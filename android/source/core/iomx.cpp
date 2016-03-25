/*****************************************************************************
 * iomx.cpp: OpenMAX interface implementation based on IOMX
 *****************************************************************************
 * Copyright (C) 2011 VLC authors and VideoLAN
 *
 * Authors: Martin Storsjo <martin@martin.st>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#include <media/stagefright/OMXClient.h>
#include <media/IOMX.h>

#include <binder/MemoryDealer.h>
#include <OMX_Component.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#if ANDROID_SDK_VERSION > 14
#include <media/hardware/HardwareAPI.h>
#include <media/hardware/OMXPluginBase.h>
#else
#include <media/stagefright/HardwareAPI.h>
#include <media/stagefright/OMXPluginBase.h>
#endif

#include "OMXMaster.h"
#include "graphics_type.h"
#include "core.cc"
#include "bug_helper.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

// Disable it, we are prefer StoreMetaDataBuffer mode on hw/encode mode.
#undef IOMX_ENABLE_SURFACE

#ifdef IOMX_ENABLE_SURFACE
#include <gui/Surface.h>
#endif

#if ANDROID_SDK_VERSION >= 11
#define HAS_USE_BUFFER
#endif
#ifdef DEBUG_RTC
class ScopedDebug{
private:
  const char *string;
  int ln;

public:
  ScopedDebug(int line, const char *s){
    string = s;
    ln = line;
    DEBUG("ScopedDebug: [%d] --> %s", ln,s);
  }
  ~ScopedDebug(){
    DEBUG("ScopedDebug: [%d] <-- %s", ln,string);
  }
};

#define SCOPEDDEBUG() ScopedDebug __ScopedDebug(__LINE__,__FUNCTION__)
#else
#define SCOPEDDEBUG()
#endif

#define iomx_xstr(x) #x
#define iomx_str(x) iomx_xstr(x)

using namespace android;

class IOMXContext {
public:
  struct ComponentsInfo{
    String8 mName;
    Vector<String8> mRoles;
  };
    bool omxv2;
    const char *build_info;
    List<IOMX::ComponentInfo> components;
    IOMXContext() {
        build_info = iomx_str(BUILD_INFO);
    }
    sp<IOMX> iomx;
    OMXPluginBase *omx;
};

static IOMXContext *ctx;

class OMXNode;

class OMXCodecObserver : public BnOMXObserver, public IBinder::DeathRecipient{
public:
    OMXCodecObserver() {
        node = NULL;
    }
    void setNode(OMXNode* n) {
        node = n;
        const sp<IBinder::DeathRecipient> binder  = this;
#if ANDROID_SDK_VERSION >= 23
        ctx->iomx->asBinder(ctx->iomx.get())->linkToDeath(binder);
#else
        ctx->iomx->asBinder()->linkToDeath(binder);
#endif
    }
#if ANDROID_SDK_VERSION >= 23
    virtual void onMessages(const std::list<omx_message> &messages) {
        for (std::list<omx_message>::const_iterator it = messages.cbegin();
                it != messages.cend(); ++it) {
            onMessage(*it);
        }    
    }    
#endif
    void onMessage(const omx_message &msg);
    void registerBuffers(const sp<IMemoryHeap> &) {}
    void binderDied(const wp<IBinder>& who);

private:
    OMXNode *node;
};

enum{
    kFlagConnectServerAPI = 1,
    kFlagEnableAllocBufferToUseBuffer = 2
};

class OMXNode {
public:
    IOMX::node_id node;
    sp<OMXCodecObserver> observer;
    OMX_CALLBACKTYPE callbacks;
    OMX_PTR app_data;
    OMX_STATETYPE state;
    List<OMX_BUFFERHEADERTYPE*> buffers;
    OMX_HANDLETYPE handle;
    uint32_t flags;
    String8 component_name;
#if (ANDROID_SDK_VERSION >= 18 ) && defined(IOMX_ENABLE_SURFACE)
    sp<IGraphicBufferProducer> producer;
    sp<Surface> surface;
#endif
    OMXNode()
    {
     flags = 0;
    }
};

typedef struct LOCAL_OMX_COMPONENTTYPE {
   OMX_COMPONENTTYPE localComp;
   uint32_t flags;
   void *local_private;
}LOCAL_OMX_COMPONENTTYPE;

class OMXBuffer {
public:
    sp<MemoryDealer> dealer;
#ifdef HAS_USE_BUFFER
    sp<GraphicBuffer> graphicBuffer;
    void *surfaceBuffer;
#endif
    IOMX::buffer_id id;
};
void OMXCodecObserver::binderDied(const wp<IBinder> &who)
{
   node->callbacks.EventHandler(node->handle,
                                node->app_data,
                                OMX_EventError,
                                -1,
                                -1,
                                (OMX_PTR)NULL);
   const sp<IBinder::DeathRecipient> binder  = this;
#if ANDROID_SDK_VERSION >= 23
        ctx->iomx->asBinder(ctx->iomx.get())->linkToDeath(binder);
#else
        ctx->iomx->asBinder()->linkToDeath(binder);
#endif
}
void OMXCodecObserver::onMessage(const omx_message &msg)
{
    if (!node)
        return;
    switch (msg.type) {
    case omx_message::EVENT:
        // TODO: Needs locking
        if (msg.u.event_data.event == OMX_EventCmdComplete && msg.u.event_data.data1 == OMX_CommandStateSet)
            node->state = (OMX_STATETYPE) msg.u.event_data.data2;
        node->callbacks.EventHandler(node->handle, node->app_data, msg.u.event_data.event, msg.u.event_data.data1, msg.u.event_data.data2, (OMX_PTR)NULL);
        break;
    case omx_message::EMPTY_BUFFER_DONE:
        for( List<OMX_BUFFERHEADERTYPE*>::iterator it = node->buffers.begin(); it != node->buffers.end(); it++ ) {
            OMXBuffer* info = (OMXBuffer*) (*it)->pPlatformPrivate;
            if (msg.u.buffer_data.buffer == info->id) {
                node->callbacks.EmptyBufferDone(node->handle, node->app_data, *it);
                break;
            }
        }
        break;
    case omx_message::FILL_BUFFER_DONE:
        for( List<OMX_BUFFERHEADERTYPE*>::iterator it = node->buffers.begin(); it != node->buffers.end(); it++ ) {
            OMXBuffer* info = (OMXBuffer*) (*it)->pPlatformPrivate;
            if (msg.u.extended_buffer_data.buffer == info->id) {
                OMX_BUFFERHEADERTYPE *buffer = *it;
                buffer->nOffset = msg.u.extended_buffer_data.range_offset;
                buffer->nFilledLen = msg.u.extended_buffer_data.range_length;
                buffer->nFlags = msg.u.extended_buffer_data.flags;
                buffer->nTimeStamp = msg.u.extended_buffer_data.timestamp;
                node->callbacks.FillBufferDone(node->handle, node->app_data, buffer);
                break;
            }
        }
        break;
    default:
        break;
    }
}
static bool AcceesUid()
{
  uid_t uid = getuid();
  bool systemuser = false;
  if ((int)uid == 0){
      systemuser = true;
    }else{
      struct passwd *pass;
      pass = getpwuid (uid);
      if (strcmp("system", pass->pw_name) == 0){
       // system user is ok ??
          systemuser = false;
        }
    }
  DEBUG("Is root ?:%d", systemuser);
  return systemuser;
}

static uint32_t FlagsForComponent(const char *name)
{
    bool is32mode = sizeof(OMX_PTR) == 4;
    std::string std_name(name);
    int pos = std_name.find("encode", 4);
    bool isEncoder = pos >=0;
    uint32_t flag = 0;
    flag |= kFlagConnectServerAPI;
    if (strncmp(name, "OMX.kedacom.", 12) == 0){

       flag |= kFlagEnableAllocBufferToUseBuffer;

       if ((!is32mode) && ctx->omxv2){
           // 64bit mode, to need the binder ???
           // I don't much want to have a 'stone_media_server'
           // that the executable program by used codec.
           flag &= ~kFlagConnectServerAPI;
       }
    }else if (strncmp(name, "OMX.qcom.", 9) == 0){
        if (ctx->omxv2 && isEncoder){
         // we are want to use the GraphicsBuffer
         // Connect local api, but only encode mode.
         // Decoder will be used useGraphicBuffer as output.
         flag &= ~kFlagConnectServerAPI;
        }
    }
    return flag;
}

static OMX_ERRORTYPE get_error(status_t err)
{
    if (err == OK)
        return OMX_ErrorNone;
    return OMX_ErrorUndefined;
}

static OMX_ERRORTYPE iomx_send_command(OMX_HANDLETYPE component, OMX_COMMANDTYPE command, OMX_U32 param1, OMX_PTR)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    return get_error(ctx->iomx->sendCommand(node->node, command, param1));
}

static OMX_ERRORTYPE iomx_get_parameter(OMX_HANDLETYPE component, OMX_INDEXTYPE param_index, OMX_PTR param)
{
    /*
     * Some QCOM OMX_getParameter implementations override the nSize element to
     * a bad value. So, save the initial nSize in order to restore it after.
     */
    OMX_U32 nSize = *(OMX_U32*)param;
    OMX_ERRORTYPE error;
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;

    error = get_error(ctx->iomx->getParameter(node->node, param_index, param, nSize));
    *(OMX_U32*)param = nSize;
    return error;
}

static OMX_ERRORTYPE iomx_set_parameter(OMX_HANDLETYPE component, OMX_INDEXTYPE param_index, OMX_PTR param)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    return get_error(ctx->iomx->setParameter(node->node, param_index, param, *(OMX_U32*)param));
}

static OMX_ERRORTYPE iomx_get_state(OMX_HANDLETYPE component, OMX_STATETYPE *ptr) {
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    *ptr = node->state;
    return OMX_ErrorNone;
}

static OMX_ERRORTYPE iomx_allocate_buffer(OMX_HANDLETYPE component, OMX_BUFFERHEADERTYPE **bufferptr, OMX_U32 port_index, OMX_PTR app_private, OMX_U32 size)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    OMXBuffer* info = new OMXBuffer;
    OMX_BUFFERHEADERTYPE *buffer  = NULL;
#ifdef HAS_USE_BUFFER
    info->graphicBuffer = NULL;
#endif
    info->dealer = new MemoryDealer(size + 4096); // Do we need to keep this around, or is it kept alive via the IMemory that references it?
    sp<IMemory> mem = info->dealer->allocate(size);
    int ret = OK;

#if ANDROID_SDK_VERSION >= 23
    if (node->flags & kFlagEnableAllocBufferToUseBuffer){
        ret = ctx->iomx->useBuffer(node->node, port_index, mem, &info->id, size);
    } else {
        ret = ctx->iomx->allocateBufferWithBackup(node->node, port_index, mem, &info->id, size);
    }
#else
    if (node->flags & kFlagEnableAllocBufferToUseBuffer){
        ret = ctx->iomx->useBuffer(node->node, port_index, mem, &info->id);
    } else {
        ret = ctx->iomx->allocateBufferWithBackup(node->node, port_index, mem, &info->id);
    }
#endif

    if (ret != OK)
        return OMX_ErrorUndefined;
    buffer = (OMX_BUFFERHEADERTYPE*) calloc(1, sizeof(OMX_BUFFERHEADERTYPE));
    *bufferptr = buffer;
    buffer->pPlatformPrivate = info;
    buffer->pAppPrivate = app_private;
    buffer->nAllocLen = size;
    buffer->pBuffer = (OMX_U8*) mem->pointer();
    node->buffers.push_back(buffer);
    return OMX_ErrorNone;
}

#ifdef HAS_USE_BUFFER
static OMX_ERRORTYPE iomx_use_buffer(OMX_HANDLETYPE component, OMX_BUFFERHEADERTYPE **bufferptr, OMX_U32 port_index, OMX_PTR app_private, OMX_U32 size, OMX_U8* data)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    OMXBuffer* info = new OMXBuffer;
    info->dealer = NULL;
    info->graphicBuffer = ((VideoBuffer *)data)->graphicBuffer;
#if 0
#if ANDROID_SDK_VERSION <= 13
    info->graphicBuffer = new GraphicBuffer((android_native_buffer_t*) data, false);
#else
    info->graphicBuffer = new GraphicBuffer((ANativeWindowBuffer*) data, false);
#endif
#endif
    int ret = ctx->iomx->useGraphicBuffer(node->node, port_index, info->graphicBuffer, &info->id);
    if (ret != OK)
        return OMX_ErrorUndefined;
    OMX_BUFFERHEADERTYPE *buffer = (OMX_BUFFERHEADERTYPE*) calloc(1, sizeof(OMX_BUFFERHEADERTYPE));
    *bufferptr = buffer;
    buffer->pPlatformPrivate = info;
    buffer->pAppPrivate = app_private;
    buffer->nAllocLen = size;
    buffer->pBuffer = data;
    node->buffers.push_back(buffer);
    return OMX_ErrorNone;
}
#endif

static OMX_ERRORTYPE iomx_free_buffer(OMX_HANDLETYPE component, OMX_U32 port, OMX_BUFFERHEADERTYPE *buffer)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    OMXBuffer* info = (OMXBuffer*) buffer->pPlatformPrivate;
    status_t ret = ctx->iomx->freeBuffer(node->node, port, info->id);
    for( List<OMX_BUFFERHEADERTYPE*>::iterator it = node->buffers.begin(); it != node->buffers.end(); it++ ) {
        if (buffer == *it) {
            node->buffers.erase(it);
            break;
        }
    }
    free(buffer);
    delete info;
    return get_error(ret);
}

static OMX_ERRORTYPE iomx_empty_this_buffer(OMX_HANDLETYPE component, OMX_BUFFERHEADERTYPE *buffer)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    OMXBuffer* info = (OMXBuffer*) buffer->pPlatformPrivate;
    return get_error(ctx->iomx->emptyBuffer(node->node, info->id, buffer->nOffset, buffer->nFilledLen, buffer->nFlags, buffer->nTimeStamp));
}

static OMX_ERRORTYPE iomx_fill_this_buffer(OMX_HANDLETYPE component, OMX_BUFFERHEADERTYPE *buffer)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    OMXBuffer* info = (OMXBuffer*) buffer->pPlatformPrivate;
    return get_error(ctx->iomx->fillBuffer(node->node, info->id));
}

static OMX_ERRORTYPE iomx_component_role_enum(OMX_HANDLETYPE component, OMX_U8 *role, OMX_U32 index)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    for( List<IOMX::ComponentInfo>::iterator it = ctx->components.begin(); it != ctx->components.end(); it++ ) {
        if (node->component_name == it->mName) {
            if (index >= it->mRoles.size())
                return OMX_ErrorNoMore;
            List<String8>::iterator it2 = it->mRoles.begin();
            for( OMX_U32 i = 0; it2 != it->mRoles.end() && i < index; i++, it2++ ) ;
            strncpy((char*)role, it2->string(), OMX_MAX_STRINGNAME_SIZE);
            if (it2->length() >= OMX_MAX_STRINGNAME_SIZE)
                role[OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
            return OMX_ErrorNone;
        }
    }
    return OMX_ErrorInvalidComponentName;
}

static OMX_ERRORTYPE iomx_get_extension_index(OMX_HANDLETYPE component, OMX_STRING parameter, OMX_INDEXTYPE *index)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    return get_error(ctx->iomx->getExtensionIndex(node->node, parameter, index));
}

static OMX_ERRORTYPE iomx_set_config(OMX_HANDLETYPE component, OMX_INDEXTYPE index, OMX_PTR param)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    return get_error(ctx->iomx->setConfig(node->node, index, param, *(OMX_U32*)param));
}

static OMX_ERRORTYPE iomx_get_config(OMX_HANDLETYPE component, OMX_INDEXTYPE index, OMX_PTR param)
{
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    return get_error(ctx->iomx->getConfig(node->node, index, param, *(OMX_U32*)param));
}

extern "C" {

bool PREFIX(JoinBinderListing)(bool noBlock)
{
  SCOPEDDEBUG();
  if (noBlock)
    android::ProcessState::self()->startThreadPool();
  return noBlock;
}


OMX_ERRORTYPE PREFIX(OMX_GetHandle)(OMX_HANDLETYPE *handle_ptr, OMX_STRING component_name, OMX_PTR app_data, OMX_CALLBACKTYPE *callbacks)
{
  SCOPEDDEBUG();
  OMX_ERRORTYPE err = OMX_ErrorNone;
  uint32_t flag = FlagsForComponent((const char  *)component_name);
  if (flag & kFlagConnectServerAPI) {
    DEBUG("[%s] From Service alloc OpenMAX component instance...", (const char *)component_name);
    OMXNode* node = new OMXNode();
    node->app_data = app_data;
    node->callbacks = *callbacks;
    node->observer = new OMXCodecObserver();
    node->observer->setNode(node);
    node->state = OMX_StateLoaded;
    node->component_name = component_name;
    node->flags = flag;

    LOCAL_OMX_COMPONENTTYPE* local_component = (LOCAL_OMX_COMPONENTTYPE*) malloc(sizeof(LOCAL_OMX_COMPONENTTYPE));
    memset(local_component, 0, sizeof(LOCAL_OMX_COMPONENTTYPE));
    local_component->local_private = NULL;
    local_component->flags = flag;
    OMX_COMPONENTTYPE * component = (OMX_COMPONENTTYPE *)local_component;
    component->nSize = sizeof(OMX_COMPONENTTYPE);
    component->nVersion.s.nVersionMajor = 1;
    component->nVersion.s.nVersionMinor = 0;
    component->nVersion.s.nRevision = 0;
    component->nVersion.s.nStep = 0;
    component->pComponentPrivate = node;
    component->SendCommand = iomx_send_command;
    component->GetParameter = iomx_get_parameter;
    component->SetParameter = iomx_set_parameter;
    component->FreeBuffer = iomx_free_buffer;
    component->EmptyThisBuffer = iomx_empty_this_buffer;
    component->FillThisBuffer = iomx_fill_this_buffer;
    component->GetState = iomx_get_state;
    component->AllocateBuffer = iomx_allocate_buffer;
#ifdef HAS_USE_BUFFER
    component->UseBuffer = iomx_use_buffer;
#else
    component->UseBuffer = NULL;
#endif
    component->ComponentRoleEnum = iomx_component_role_enum;
    component->GetExtensionIndex = iomx_get_extension_index;
    component->SetConfig = iomx_set_config;
    component->GetConfig = iomx_get_config;

    *handle_ptr = component;
    node->handle = component;
    status_t ret;
    if ((ret = ctx->iomx->allocateNode( component_name, node->observer, &node->node )) != OK){
        err = OMX_ErrorUndefined;
      }
    }else{
    OMX_HANDLETYPE phandle = NULL;
      err = ctx->omx->makeComponentInstance(component_name, callbacks , app_data , (OMX_COMPONENTTYPE **)(&phandle));
      if(err == OMX_ErrorNone){
          LOCAL_OMX_COMPONENTTYPE* component =
              (LOCAL_OMX_COMPONENTTYPE*) malloc(sizeof(LOCAL_OMX_COMPONENTTYPE));
          memset(component, 0, sizeof(LOCAL_OMX_COMPONENTTYPE));
          component->local_private = phandle;
          component->flags = flag;
          OMX_COMPONENTTYPE *comp = (OMX_COMPONENTTYPE *)component;
          *comp = *((OMX_COMPONENTTYPE *)phandle);
          *handle_ptr = comp;
      }
    }
  return err;
}

OMX_ERRORTYPE PREFIX(OMX_FreeHandle)(OMX_HANDLETYPE handle)
{
  SCOPEDDEBUG();
  OMX_ERRORTYPE err = OMX_ErrorNone;
  LOCAL_OMX_COMPONENTTYPE *local = (LOCAL_OMX_COMPONENTTYPE *)handle;
  if (local->flags & kFlagConnectServerAPI){
      OMXNode *node = (OMXNode *)(local->localComp.pComponentPrivate);
      ctx->iomx->freeNode( node->node );
      node->observer->setNode(NULL);
      delete node;
      free(handle);
    }else {
      err =  ctx->omx->destroyComponentInstance((OMX_COMPONENTTYPE *)local->local_private);
      free(handle);
    }
  return err;
}

OMX_ERRORTYPE PREFIX(OMX_InitV2)(void)
{
  SCOPEDDEBUG();
  DEBUG("Using OMX_Init V2");
    if (!ctx){
        ctx = new IOMXContext();
      }
    ctx->omx = new OMXMaster;
    ctx->omxv2 = true;
#if 0
    // we are already have these components in the list.
    for(OMX_U32 i= 0 ; i < 100 ; i++){
        char name[OMX_MAX_STRINGNAME_SIZE] = {0};
        if ( OMX_ErrorNone ==
             ctx->omx->enumerateComponents(name, OMX_MAX_STRINGNAME_SIZE, i)){
             IOMX::ComponentInfo info;
             info.mName = String8(name);
             Vector<String8> roles;
             ctx->omx->getRolesOfComponent(name, &roles);
             for( Vector<String8>::iterator it2 = roles.begin(); it2 != roles.end(); it2++ ) {
                   info.mRoles.push_back(*it2);
             }
             ctx->components.push_back(info);
             continue;
          }
        break;
      }
#endif
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PREFIX(OMX_Init)(void)
{
  SCOPEDDEBUG();
  DEBUG("OMX Init");
  {
//      OMXClient client;
      sp<IServiceManager> sm = defaultServiceManager();
      sp<IBinder> binder;
      binder = sm->checkService(String16("stone_media_server"));
      sp<IOMX> client = interface_cast<IOMX>(binder);
      if (client.get() == NULL){
          OMXClient client_omx;
          if (client_omx.connect() != OK){
              DEBUG("Connect OpenMAX API to MediaServer faiure.");
              return OMX_ErrorUndefined;
          }
          client = client_omx.interface();
          DEBUG("Connect OpenMAX API to MediaServer.");
      }else{
          DEBUG("Connect OpenMAX API to StoneServer.");
      }

      if (!ctx){
          ctx = new IOMXContext();
          ctx->omxv2 = false;
          ctx->omx = NULL;
        }
      // Default join binder listing
      (void)PREFIX(JoinBinderListing)(true);
      //ctx->iomx = client.interface();
      ctx->iomx = client;
      ctx->iomx->listNodes(&ctx->components);
    }
    // Init OpenMAX on root mode.
    if (AcceesUid()){
      (void)PREFIX(OMX_InitV2)();
    }
  return OMX_ErrorNone;
}


OMX_ERRORTYPE PREFIX(OMX_Deinit)(void)
{
  SCOPEDDEBUG();
    if (ctx){
        if (ctx->omxv2){
            delete ctx->omx;
          }else{
            ctx->iomx = NULL;
          }
        delete ctx;
        ctx = NULL;
    }
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PREFIX(OMX_ComponentNameEnum)(OMX_STRING component_name, OMX_U32 name_length, OMX_U32 index)
{
  SCOPEDDEBUG();
  if (index >= ctx->components.size())
    return OMX_ErrorNoMore;
  List<IOMX::ComponentInfo>::iterator it = ctx->components.begin();
  for( OMX_U32 i = 0; i < index; i++ )
    it++;
  strncpy(component_name, it->mName.string(), name_length);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE PREFIX(OMX_GetRolesOfComponent)(OMX_STRING component_name, OMX_U32 *num_roles, OMX_U8 **roles)
{
  SCOPEDDEBUG();
      for( List<IOMX::ComponentInfo>::iterator it = ctx->components.begin(); it != ctx->components.end(); it++ ) {
          if (!strcmp(component_name, it->mName.string())) {
              if (!roles) {
                  *num_roles = it->mRoles.size();
                  return OMX_ErrorNone;
              }
              if (*num_roles < it->mRoles.size())
                  return OMX_ErrorInsufficientResources;
              *num_roles = it->mRoles.size();
              OMX_U32 i = 0;
              for( List<String8>::iterator it2 = it->mRoles.begin(); it2 != it->mRoles.end(); i++, it2++ ) {
                  strncpy((char*)roles[i], it2->string(), OMX_MAX_STRINGNAME_SIZE);
                  roles[i][OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
              }
              return OMX_ErrorNone;
          }
        }
    return OMX_ErrorBadParameter;
}

OMX_ERRORTYPE PREFIX(OMX_GetComponentsOfRole)(OMX_STRING role, OMX_U32 *num_comps, OMX_U8 **comp_names)
{
  SCOPEDDEBUG();
  OMX_U32 i = 0;
  for( List<IOMX::ComponentInfo>::iterator it = ctx->components.begin(); it != ctx->components.end(); it++ ) {
      for( List<String8>::iterator it2 = it->mRoles.begin(); it2 != it->mRoles.end(); it2++ ) {
          if (!strcmp(it2->string(), role)) {
              if (comp_names) {
                  if (*num_comps < i)
                      return OMX_ErrorInsufficientResources;
                  strncpy((char*)comp_names[i], it->mName.string(), OMX_MAX_STRINGNAME_SIZE);
                  comp_names[i][OMX_MAX_STRINGNAME_SIZE - 1] = '\0';
              }
              i++;
              break;
          }
      }
  }
  *num_comps = i;
  return OMX_ErrorNone;
}

#ifdef HAS_USE_BUFFER
OMX_ERRORTYPE PREFIX(OMXAndroid_EnableGraphicBuffers)(OMX_HANDLETYPE component, OMX_U32 port_index, OMX_BOOL enable)
{
  SCOPEDDEBUG();
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    int ret = ctx->iomx->enableGraphicBuffers(node->node, port_index, enable);
    if (ret != OK)
        return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PREFIX(OMXAndroid_GetGraphicBufferUsage)(OMX_HANDLETYPE component, OMX_U32 port_index, OMX_U32* usage)
{
  SCOPEDDEBUG();
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    int ret = ctx->iomx->getGraphicBufferUsage(node->node, port_index, usage);
    if (ret != OK)
        return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE PREFIX(OMXAndroid_GetHalFormat)( const char *comp_name, int* hal_format )
{
    if( !strncmp( comp_name, "OMX.SEC.", 8 ) ) {
        switch( *hal_format ) {
        case OMX_COLOR_FormatYUV420SemiPlanar:
            *hal_format = 0x105; // HAL_PIXEL_FORMAT_YCbCr_420_SP
            break;
        case OMX_COLOR_FormatYUV420Planar:
            *hal_format = 0x101; // HAL_PIXEL_FORMAT_YCbCr_420_P
            break;
        }
    }
    else if( !strcmp( comp_name, "OMX.TI.720P.Decoder" ) ||
        !strcmp( comp_name, "OMX.TI.Video.Decoder" ) )
        *hal_format = 0x14; // HAL_PIXEL_FORMAT_YCbCr_422_I
#if ANDROID_SDK_VERSION <= 13 // Required on msm8660 on 3.2, not required on 4.1
    else if( !strcmp( comp_name, "OMX.qcom.video.decoder.avc" ))
        *hal_format = 0x108;
#endif

    return OMX_ErrorNone;
}

#endif
}

extern "C" {
#if (ANDROID_SDK_VERSION >= 18) && defined(IOMX_ENABLE_SURFACE)
OMX_ERRORTYPE PREFIX(OMXAndroid_CreateInputSurface)(OMX_HANDLETYPE component, OMX_U32 port_index)
{
  SCOPEDDEBUG();
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    int ret = ctx->iomx->createInputSurface(node->node, port_index, &node->producer);
    if (ret != OK || node->producer.get() == NULL)
        return OMX_ErrorUndefined;
    node->producer->setBufferCount(10);
    node->surface = new Surface(node->producer);
    return OMX_ErrorNone;
}
OMX_ERRORTYPE PREFIX(OMXAndroid_LockInputSurface)(OMX_HANDLETYPE component, OMX_U32 port_index, OMX_PTR *buffer)
{
  SCOPEDDEBUG();
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    if (!port_index && node->surface.get()){
        ANativeWindow_Buffer buff;
        if (node->surface->lock(&buff, NULL) >= 0){
             *buffer = buff.bits;
            return OMX_ErrorNone;
          }
      }else{
        return OMX_ErrorBadParameter;
      }
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE PREFIX(OMXAndroid_UnlockInputSurface)(OMX_HANDLETYPE component, OMX_U32 port_index)
{
  SCOPEDDEBUG();
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    if (!port_index && node->surface.get()){
        if (node->surface->unlockAndPost() >= 0){
            return OMX_ErrorNone;
          }
      }else{
        return OMX_ErrorBadParameter;
      }
    return OMX_ErrorUndefined;
}
OMX_ERRORTYPE PREFIX(OMXAndroid_SignalEndOfInputStream)(OMX_HANDLETYPE component)
{
  SCOPEDDEBUG()
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    int ret = ctx->iomx->signalEndOfInputStream(node->node);
    if (ret != OK)
        return OMX_ErrorUndefined;
    return OMX_ErrorNone;
}
#endif

#if ANDROID_SDK_VERSION >= 14
OMX_ERRORTYPE PREFIX(OMXAndroid_StoreMetaDataInBuffers)(OMX_HANDLETYPE component, OMX_U32 port_index, OMX_BOOL enable)
{
  SCOPEDDEBUG();
  if (ctx->omxv2){
      OMX_INDEXTYPE index;
      OMX_STRING name = const_cast<OMX_STRING>(
              "OMX.google.android.index.storeMetaDataInBuffers");

      OMX_ERRORTYPE err = OMX_GetExtensionIndex(component, name, &index);
      if (err != OMX_ErrorNone) {
          return err;
      }
      StoreMetaDataInBuffersParams params;
      memset(&params, 0, sizeof(params));
      params.nSize = sizeof(params);
      // Version: 1.0.0.0
      params.nVersion.s.nVersionMajor = 1;
      params.nPortIndex = port_index;
      params.bStoreMetaData = enable;
      return OMX_SetParameter(component, index, &params);
    }else{
#if 0
    OMXNode* node = (OMXNode*) ((OMX_COMPONENTTYPE*)component)->pComponentPrivate;
    int ret = ctx->iomx->storeMetaDataInBuffers(node->node, port_index, enable);
    if (ret != OK)
        return OMX_ErrorUndefined;
    }
    return OMX_ErrorNone;
#else
    // Current GraphicBuffer unsupport Binder IPC.
    // if we are do it, the mediaserver will be crash.
    }
    return OMX_ErrorUnsupportedIndex;
#endif
}
#endif
} // end of the extern "C"

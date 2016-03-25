#include "OmxCore.h"
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include "bug_helper.h"
#include <pthread.h>
#ifndef NULL
#define NULL 0
#endif

#define OMX_FIND_SYM(name, handle) RTC_FIND_SYM(name, handle)
#ifdef DEBUG_RTC
#define OMX_CALL(name) (true || \
  DEBUG("OMX_CALL: %s addr %p\n", __FUNCTION__, _##name) || \
  true )&& _##name == NULL ? OMX_ErrorNotImplemented :(_##name)
#else
#define OMX_CALL(name) _##name == NULL ? OMX_ErrorNotImplemented :(_##name)
#endif

class OpenMAXLibs{
private:
  void *_handle;
  OMX_ERRORTYPE (*_OMX_Init)();
  OMX_ERRORTYPE (*_OMX_Deinit)();
  pthread_mutex_t _lock;
  pthread_mutexattr_t _attr;
  bool _haveInit;
  bool _haveHandle;
public:
  OpenMAXLibs():_handle(NULL),
    _OMX_Init(NULL),
    _OMX_Deinit(NULL),
    _haveInit(false),
    _haveHandle(false)
  {
    pthread_mutexattr_init(&_attr);
    pthread_mutex_init(&_lock, &_attr);
  }
  void SetReinitPinding()
  {
    pthread_mutex_lock(&_lock);
    DEBUG("Binder server is crash.\n");
     _haveInit = true;
    pthread_mutex_unlock(&_lock);
  }

  bool Dlopen(const char *libs, void *hwd){
    pthread_mutex_lock(&_lock);
    bool ret = _handle ? true : false;
    void *handle = NULL;
    if (hwd){
     _haveHandle = true;
    }
    if ((libs && !ret) || _haveHandle){
        handle = _haveHandle ? hwd : dlopen(libs, RTLD_NOW);
        if (handle){
            OMX_FIND_SYM(OMX_Init, handle);
            OMX_FIND_SYM(OMX_Deinit, handle);
            if (_OMX_Init && _OMX_Deinit){
                if (OMX_CALL(OMX_Init)() == OMX_ErrorNone){
                    _handle = handle;
                    ret = true;
                  }
              }else{
                if (!_haveHandle)
                    dlclose(handle);
              }
          }
      }
    pthread_mutex_unlock(&_lock);
    return ret;
  }
  void *Handle(){
    DEBUG("Get Omx Handle.\n");
    pthread_mutex_lock(&_lock);
    if (_handle && _haveInit){
        DEBUG("ReInit Omx.\n");
        OMX_CALL(OMX_Deinit)();
        if (OMX_CALL(OMX_Init)() == OMX_ErrorNone)
          _haveInit = false;
      }
   pthread_mutex_unlock(&_lock);
    return _haveInit ? NULL : _handle;
  }

  ~OpenMAXLibs(){
    if(_handle){
        OMX_CALL(OMX_Deinit)();
        if (!_haveHandle)
        dlclose(_handle);
        DEBUG("~Dtor OpenMAXLibs\n");
      }
  }
};

static OpenMAXLibs _glibs;

namespace CCStone {

  OMXCore::OMXCore():
    _handle(NULL),
    _appData(NULL),
    _cb(NULL),
    _OMX_Init(NULL),
    _OMX_ComponentNameEnum(NULL),
    _OMX_Deinit(NULL),
    _OMX_FreeHandle(NULL),
    _OMX_GetComponentsOfRole(NULL),
    _OMX_GetContentPipe(NULL),
    _OMX_GetHandle(NULL),
    _OMX_GetRolesOfComponent(NULL),
    _OMX_SetupTunnel(NULL),
    _OMXAndroid_EnableGraphicBuffers(NULL),
    _OMXAndroid_GetGraphicBufferUsage(NULL),
    _OMXAndroid_GetHalFormat(NULL),
    _JoinBinderListing(NULL),
    _OMXAndroid_CreateInputSurface(NULL),
    _OMXAndroid_LockInputSurface(NULL),
    _OMXAndroid_UnlockInputSurface(NULL),
    _OMXAndroid_SignalEndOfInputStream(NULL),
    _OMXAndroid_StoreMetaDataInBuffers(NULL)
  {
    _omxcb.FillBufferDone = OMXCore::FillBufferDone;
    _omxcb.EmptyBufferDone = OMXCore::EmptyBufferDone;
    _omxcb.EventHandler = OMXCore::EventHandler;
    DEBUG("Ctor OMXCore\n");
  }
  OMXCore::~OMXCore ()
  {
    _handle = NULL;
    DEBUG("~Dtor OMXCore\n");
  }
  OMX_ERRORTYPE OMXCore::FillBufferDone (OMX_HANDLETYPE hComponent,
                                         OMX_PTR pAppData,
                                         OMX_BUFFERHEADERTYPE *pBuffer)
  {
    PrivateData_t *me = (PrivateData_t *) pAppData;
    return me->cb->FillBufferDone (hComponent, me->userPriv, pBuffer);
  }
  OMX_ERRORTYPE OMXCore::EmptyBufferDone (OMX_HANDLETYPE hComponent,
                                          OMX_PTR pAppData,
                                          OMX_BUFFERHEADERTYPE *pBuffer)
  {
    PrivateData_t *me = (PrivateData_t *) pAppData;
    return me->cb->EmptyBufferDone (hComponent, me->userPriv, pBuffer);
  }
  OMX_ERRORTYPE OMXCore::EventHandler (OMX_HANDLETYPE hComponent,
                                       OMX_PTR pAppData,
                                       OMX_EVENTTYPE eEvent,
                                       OMX_U32 nData1,
                                       OMX_U32 nData2,
                                       OMX_PTR pEventData)
  {
    PrivateData_t *me = (PrivateData_t *) pAppData;
    if (eEvent == OMX_EventError && nData1 == nData2 && nData1 == (OMX_U32)-1){
        // Binder server Crash
        _glibs.SetReinitPinding();
      }
    return me->cb->EventHandler (hComponent,
                                  me->userPriv,
                                  eEvent,
                                  nData1,
                                  nData2,
                                  pEventData);
  }

  bool OMXCore::OpenLibs (const char *libName, void *thwd)
  {
    bool ret = false;
    if (libName == NULL && thwd == NULL) return ret;
    if (_glibs.Dlopen(libName, thwd)){
        _handle = _glibs.Handle();
      }

    if (_handle != NULL){
        OMX_FIND_SYM(OMX_GetComponentsOfRole, _handle);
        OMX_FIND_SYM(OMX_GetRolesOfComponent, _handle);
        OMX_FIND_SYM(OMX_ComponentNameEnum, _handle);
        OMX_FIND_SYM(OMX_GetHandle, _handle);
        OMX_FIND_SYM(OMX_FreeHandle, _handle);
        if (_OMX_GetComponentsOfRole &&
            _OMX_GetRolesOfComponent &&
            _OMX_GetHandle && _OMX_FreeHandle){
            ret = true;
            // Not necessarily
            OMX_FIND_SYM(OMX_GetContentPipe, _handle);
            OMX_FIND_SYM(OMX_SetupTunnel, _handle);
            OMX_FIND_SYM(OMXAndroid_GetHalFormat, _handle);
            OMX_FIND_SYM(OMXAndroid_EnableGraphicBuffers, _handle);
            OMX_FIND_SYM(OMXAndroid_GetGraphicBufferUsage, _handle);
            OMX_FIND_SYM(OMXAndroid_CreateInputSurface, _handle);
            OMX_FIND_SYM(OMXAndroid_LockInputSurface, _handle);
            OMX_FIND_SYM(OMXAndroid_UnlockInputSurface, _handle);
            OMX_FIND_SYM(OMXAndroid_SignalEndOfInputStream, _handle);
            OMX_FIND_SYM(OMXAndroid_StoreMetaDataInBuffers, _handle);
            OMX_FIND_SYM(JoinBinderListing, _handle);
          }
      }else{
          if (!thwd)
              DEBUG("Open `%s` handle faild , `%s`\n", libName, dlerror());
      }
    return ret;
  }
  bool OMXCore::JoinBinderListing (bool no_block)
  {
    return OMX_CALL(JoinBinderListing)(no_block);
  }

  void OMXCore::Release ()
  {
    //int refCount = DecRef();
    //if (refCount == 1){
    //    delete this;
    //  }
  }

  OMX_ERRORTYPE OMXCore::OMX_Init()
  {
    return OMX_ErrorNone;
  }

  OMX_ERRORTYPE OMXCore::OMX_Deinit()
  {
    return OMX_ErrorNone;

  }

  OMX_ERRORTYPE OMXCore::OMX_GetHandle(
      OMX_HANDLETYPE* pHandle,
      OMX_STRING cComponentName,
      OMX_PTR pAppData,
      IOMXCallBack* pCallBacks)
  {
      PrivateData_t *data = new PrivateData_t;
      data->cb = pCallBacks;
      data->userPriv = pAppData;
    OMX_ERRORTYPE ret = OMX_CALL(OMX_GetHandle)(pHandle,
                                                cComponentName,
                                                data,
                                                &_omxcb);
    if (ret != OMX_ErrorNone){
        delete data;
        //_appData = pAppData;
        //_cb = pCallBacks;
      }
    return ret;
  }

  OMX_ERRORTYPE OMXCore::OMX_FreeHandle(
      OMX_HANDLETYPE hComponent)
  {
    return OMX_CALL(OMX_FreeHandle)(hComponent);
  }

  OMX_ERRORTYPE  OMXCore::OMX_ComponentNameEnum(
      OMX_STRING cComponentName,
      OMX_U32 nNameLength,
      OMX_U32 nIndex)
  {
    return OMX_CALL(OMX_ComponentNameEnum)(cComponentName,
                                           nNameLength,
                                           nIndex);
  }

  OMX_ERRORTYPE  OMXCore::OMX_SetupTunnel(
      OMX_HANDLETYPE hOutput,
      OMX_U32 nPortOutput,
      OMX_HANDLETYPE hInput,
      OMX_U32 nPortInput)
  {
    return OMX_CALL(OMX_SetupTunnel)(hOutput,
                                     nPortOutput,
                                     hInput,
                                     nPortInput);
  }

  /** @ingroup cp */
  OMX_ERRORTYPE   OMXCore::OMX_GetContentPipe(
      OMX_HANDLETYPE *hPipe,
      OMX_STRING szURI)
  {
    return OMX_CALL(OMX_GetContentPipe)(hPipe,
                                        szURI);
  }

  OMX_ERRORTYPE OMXCore::OMX_GetComponentsOfRole (
      OMX_STRING role,
      OMX_U32 *pNumComps,
      OMX_U8  **compNames)
  {
    return OMX_CALL(OMX_GetComponentsOfRole)(role,
                                             pNumComps,
                                             compNames);
  }

  OMX_ERRORTYPE OMXCore::OMX_GetRolesOfComponent (
      OMX_STRING compName,
      OMX_U32 *pNumRoles,
      OMX_U8 **roles)
  {
    return OMX_CALL(OMX_GetRolesOfComponent)(compName,
                                             pNumRoles,
                                             roles);
  }
  OMX_ERRORTYPE
  OMXCore::OMXAndroid_EnableGraphicBuffers(OMX_HANDLETYPE component,
                                           OMX_U32 port_index,
                                           OMX_BOOL enable)
  {
    return OMX_CALL(OMXAndroid_EnableGraphicBuffers)(component,
                                                     port_index,
                                                     enable);
  }


  OMX_ERRORTYPE
  OMXCore::OMXAndroid_GetGraphicBufferUsage(OMX_HANDLETYPE component,
                                            OMX_U32 port_index,
                                            OMX_U32* usage)
  {
    return OMX_CALL(OMXAndroid_GetGraphicBufferUsage)(component,
                                                      port_index,
                                                      usage);
  }


  OMX_ERRORTYPE OMXCore::OMXAndroid_GetHalFormat( const char *comp_name
                                                  , int* hal_format )
  {
    return OMX_CALL(OMXAndroid_GetHalFormat)(comp_name,
                                             hal_format
                                             );
  }

  OMX_ERRORTYPE OMXCore::OMXAndroid_CreateInputSurface(OMX_HANDLETYPE component,
                                                  OMX_U32 port_index)
  {
    return OMX_CALL(OMXAndroid_CreateInputSurface)(component, port_index);
  }
  OMX_ERRORTYPE OMXCore::OMXAndroid_LockInputSurface(OMX_HANDLETYPE component,
                                                  OMX_U32 port_index,
                                                OMX_PTR *buf)
  {
    return OMX_CALL(OMXAndroid_LockInputSurface)(component, port_index,buf);
  }
  OMX_ERRORTYPE OMXCore::OMXAndroid_UnlockInputSurface(OMX_HANDLETYPE component,
                                                  OMX_U32 port_index)
  {
    return OMX_CALL(OMXAndroid_UnlockInputSurface)(component, port_index);
  }

  OMX_ERRORTYPE OMXCore::OMXAndroid_SignalEndOfInputStream(
      OMX_HANDLETYPE component)
  {
    return OMX_CALL(OMXAndroid_SignalEndOfInputStream)(component);
  }

  OMX_ERRORTYPE OMXCore::OMXAndroid_StoreMetaDataInBuffers(OMX_HANDLETYPE component,
                                                      OMX_U32 port_index,
                                                    OMX_BOOL enable)
  {
     return OMX_CALL(OMXAndroid_StoreMetaDataInBuffers)(component, port_index,
                                                        enable);
  }
}
#if 0
int main(int c, char *ss[]){
  CCStone::OMXCore *omx = new CCStone::OMXCore;
  omx->OpenLibs (ss[1]);
  printf("%#x\n", omx->OMX_Init ());
  char name[1024];
  int i = 0;
  while (1){
      memset(name, 0,1024);
      if (omx->OMX_ComponentNameEnum (name,1024,i) != OMX_ErrorNone){
          break;
        }
      printf("==>%s\n", name);
      OMX_U32 roles = 10;
      char *rolesname[10] = {0};
      for (int j = 0; j< 10 ; j++){
          rolesname[j] = new char[1024];
          memset(rolesname[j], 0,1024);
        }
      omx->OMX_GetRolesOfComponent (name,&roles, (OMX_U8 **)rolesname);
      int x = 0;
      while (x < roles ){
          printf("====>%s\n", rolesname[x]);
          x++;
        }
      for (int j = 0; j< 10 ; j++){
          delete []rolesname[j];

        }
      i++;
    }
  printf("===========enum component name of the roles[%s]==="
         "========\n", ss[2]);
  if (ss[2] == NULL) return 0;

  char *compnames[3]  = {NULL};
  OMX_U32 compscc = 3;
  for (OMX_U32 j = 0; j< compscc; j++){
      compnames[j] = new char[1024];
      memset(compnames[j], 0,1024);
    }

  omx->OMX_GetComponentsOfRole (ss[2],
                                &compscc, (OMX_U8 **)compnames);
  for (OMX_U32 j = 0; j< compscc ; j++){
      printf("%s\n", compnames[j]);
    }

  for (OMX_U32 j = 0; j< compscc; j++){
      delete []compnames[j];
    }
  printf("%#x\n", omx->OMX_Deinit ());
  omx->Release ();
  return 0;
}
#endif

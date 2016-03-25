#ifndef OMXCORE_H
#define OMXCORE_H
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Types.h"
#include "graphics_type.h"
namespace CCStone {

  class IOMXCallBack{
  public:
    virtual OMX_ERRORTYPE EventHandler(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData) = 0;

    virtual OMX_ERRORTYPE EmptyBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer) = 0;

    virtual OMX_ERRORTYPE FillBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer) = 0;
    virtual ~IOMXCallBack(){}
  };

  class OMXCore{
  private:
  typedef struct PrivateData_{
      IOMXCallBack *cb;
      void *userPriv;
  }PrivateData_t;
    void *_handle;
    IOMXCallBack *_cb;
    OMX_CALLBACKTYPE _omxcb;
    OMX_PTR _appData;
    OMX_ERRORTYPE (*_OMX_Init)();
    OMX_ERRORTYPE (*_OMX_Deinit)();
    OMX_ERRORTYPE (*_OMX_GetHandle)(
        OMX_HANDLETYPE* ,
        OMX_STRING ,
        OMX_PTR ,
        OMX_CALLBACKTYPE * );

    OMX_ERRORTYPE (*_OMX_FreeHandle)(
        OMX_HANDLETYPE );

    OMX_ERRORTYPE  (*_OMX_ComponentNameEnum)(
        OMX_STRING ,
        OMX_U32 ,
        OMX_U32 );

    OMX_ERRORTYPE  (*_OMX_SetupTunnel)(
        OMX_HANDLETYPE ,
        OMX_U32 ,
        OMX_HANDLETYPE ,
        OMX_U32 );

    /** @ingroup cp */
    OMX_ERRORTYPE   (*_OMX_GetContentPipe)(
        OMX_HANDLETYPE *,
        OMX_STRING );

    OMX_ERRORTYPE (*_OMX_GetComponentsOfRole) (
        OMX_STRING role,
        OMX_U32 *,
        OMX_U8  **);

    OMX_ERRORTYPE (*_OMX_GetRolesOfComponent) (
        OMX_STRING ,
        OMX_U32 *,
        OMX_U8 **);

    OMX_ERRORTYPE (*_OMXAndroid_GetHalFormat)( const char *
                                               , int*
                                               );

    OMX_ERRORTYPE (*_OMXAndroid_GetGraphicBufferUsage)(OMX_HANDLETYPE
                                                       , OMX_U32
                                                       , OMX_U32*
                                                       );
    OMX_ERRORTYPE (*_OMXAndroid_EnableGraphicBuffers)(OMX_HANDLETYPE
                                                      , OMX_U32
                                                      , OMX_BOOL
                                                      );
    //OMX_ERRORTYPE (*_OMXAndroid_CreateOmxGraphics)(OmxGraphics_t *);

    OMX_ERRORTYPE (*_OMXAndroid_CreateInputSurface)(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index);
    OMX_ERRORTYPE (*_OMXAndroid_LockInputSurface)(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index,
                                                  OMX_PTR *buf);
    OMX_ERRORTYPE (*_OMXAndroid_UnlockInputSurface)(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index);
    OMX_ERRORTYPE (*_OMXAndroid_SignalEndOfInputStream)(OMX_HANDLETYPE component);

    OMX_ERRORTYPE (*_OMXAndroid_StoreMetaDataInBuffers)(OMX_HANDLETYPE component,
                                                        OMX_U32 port_index,
                                                        OMX_BOOL);

    bool (*_JoinBinderListing)(bool noBlock);

  private:
    static OMX_ERRORTYPE EventHandler(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_EVENTTYPE eEvent,
        OMX_U32 nData1,
        OMX_U32 nData2,
        OMX_PTR pEventData);
    static OMX_ERRORTYPE EmptyBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer);

    static OMX_ERRORTYPE FillBufferDone(
        OMX_HANDLETYPE hComponent,
        OMX_PTR pAppData,
        OMX_BUFFERHEADERTYPE* pBuffer);
  public:
    OMXCore();
    ~OMXCore();
  public:
    void Release();
    bool OpenLibs (const char *libName, void *handle = NULL);
    /*
     * The method is used to add this thread to binder threadpool with No-Block
     * /Block way. if the no_block is false then the this thread should be
     * block. It only to by using to Android platform.
    */
    bool JoinBinderListing(bool no_block = true);

    OMX_ERRORTYPE OMX_Init(void);

    OMX_ERRORTYPE OMX_Deinit(void);

    OMX_ERRORTYPE OMX_GetHandle(
        OMX_HANDLETYPE* pHandle,
        OMX_STRING cComponentName,
        OMX_PTR pAppData,
        IOMXCallBack* pCallBacks);

    OMX_ERRORTYPE OMX_FreeHandle(
        OMX_HANDLETYPE hComponent);

    OMX_ERRORTYPE  OMX_ComponentNameEnum(
        OMX_STRING cComponentName,
        OMX_U32 nNameLength,
        OMX_U32 nIndex);

    OMX_ERRORTYPE  OMX_SetupTunnel(
        OMX_HANDLETYPE hOutput,
        OMX_U32 nPortOutput,
        OMX_HANDLETYPE hInput,
        OMX_U32 nPortInput);

    /** @ingroup cp */
    OMX_ERRORTYPE   OMX_GetContentPipe(
        OMX_HANDLETYPE *hPipe,
        OMX_STRING szURI);

    OMX_ERRORTYPE OMX_GetComponentsOfRole (
        OMX_STRING role,
        OMX_U32 *pNumComps,
        OMX_U8  **compNames);

    OMX_ERRORTYPE OMX_GetRolesOfComponent (
        OMX_STRING compName,
        OMX_U32 *pNumRoles,
        OMX_U8 **roles);

    OMX_ERRORTYPE OMXAndroid_EnableGraphicBuffers(OMX_HANDLETYPE component,
                                                  OMX_U32 port_index,
                                                  OMX_BOOL enable);


    OMX_ERRORTYPE OMXAndroid_GetGraphicBufferUsage(OMX_HANDLETYPE component,
                                                   OMX_U32 port_index,
                                                   OMX_U32* usage);


    OMX_ERRORTYPE OMXAndroid_GetHalFormat( const char *comp_name
                                             , int* hal_format );

    //OMX_ERRORTYPE OMXAndroid_CreateOmxGraphics(OmxGraphics_t *g);

    OMX_ERRORTYPE OMXAndroid_CreateInputSurface(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index);
    OMX_ERRORTYPE OMXAndroid_LockInputSurface(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index,
                                                  OMX_PTR *buf);
    OMX_ERRORTYPE OMXAndroid_UnlockInputSurface(OMX_HANDLETYPE component,
                                                    OMX_U32 port_index);

    OMX_ERRORTYPE OMXAndroid_SignalEndOfInputStream(OMX_HANDLETYPE component);
    OMX_ERRORTYPE OMXAndroid_StoreMetaDataInBuffers(OMX_HANDLETYPE component,
                                                        OMX_U32 port_index,
                                                      OMX_BOOL enable);

  protected:
    /*
     * Return reference counts before add.
    */
    virtual int AddRef(){return 0;}
    /*
     * Return reference counts before dec.
    */
    virtual int DecRef(){return 1;}
  };
}
#endif // OMXCORE_H

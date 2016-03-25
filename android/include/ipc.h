#ifndef RTC_IPC_H
#define RTC_IPC_H
#include "videobuffer.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef void parcel_t;
typedef struct RTCIPCData {
 parcel_t *data;
 parcel_t *reply;
}RTCIPCData_t;

typedef struct RTCIPCParcel {
  void (*release)(parcel_t *);
  parcel_t *(*create)();

  int (*write_fd)(parcel_t *, int /*fd*/);
  int (*read_fd)(parcel_t *);

  int (*write_graphics_handle)(parcel_t *, graphics_handle *);
  graphics_handle *(*read_graphics_handle)(parcel_t *);

  int (*write32)(parcel_t *, int);
  int (*read32)(parcel_t *);

  int (*write64)(parcel_t *, long long);
  long long (*read64)(parcel_t *);

  int (*writef)(parcel_t *, float);
  float (*readf)(parcel_t *);

  int (*writed)(parcel_t *, double);
  double (*readd)(parcel_t *);

  int (*write_array)(parcel_t *, const void * /*data*/, unsigned int /*size*/);
  int (*read_array)(parcel_t *,unsigned int /*size*/, void * /*buffer*/);
  const void *(*read_array_pointer)(parcel_t *, unsigned int);

  int (*data_avail_size)(parcel_t *);
  int (*set_position)(parcel_t *, int );
  int (*position)(parcel_t *);
}RTCIPCParcel_t;

typedef struct RTCIPCNode {
    int (*transport)(struct RTCIPCNode *, const RTCIPCData_t *);
    void (*died)(struct RTCIPCNode *);
    int (*set_callback)(struct RTCIPCNode *, struct RTCIPCNode * /*cb*/);
}RTCIPCNode_t;

typedef struct RTCIPCHandleInterface {
 int (*transport)(struct RTCIPCHandleInterface *, const RTCIPCData_t *);
 RTCIPCNode_t *(*create)(struct RTCIPCHandleInterface *, int type);
}RTCIPCHandleInterface_t;

typedef struct RTCIPCHandle{
    RTCIPCParcel_t parcel;
    // By service call.
    int (*start)(const char * /*server_name*/, RTCIPCHandleInterface_t * /*cb*/, bool /*block*/);
    int (*stop)(const char * /*server_name*/);
    // By Node call.
    RTCIPCNode_t *(*create)(const char *server_name, int type);
    int (*release)(RTCIPCNode_t *);
}RTCIPCHandle_t;

int InitRTCIPC(RTCIPCHandle_t *rende, void *handle);
#ifdef __cplusplus
}
#endif

#endif

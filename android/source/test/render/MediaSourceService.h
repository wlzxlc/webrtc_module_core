#ifndef _MEDIASOURCESERVICE_H_
#define _MEDIASOURCESERVICE_H_
#include <ipc.h>
#include <string>
class MediaSourceService
{
private:
	std::string _name;
	RTCIPCHandleInterface *_ipcservice;
public:
	MediaSourceService(void);
	int init(const char *service);
	int start();
	virtual ~MediaSourceService(void);
};
#endif

#include "MediaSourceService.h"
#include <WebRTCAPI.h>
#include "IServiceNode.h"

using namespace CCStone;
class IPCMediaSourceService: public RTCIPCHandleInterface {
private:
	ServiceNode _nodeservices;
private:
	int Transport(const RTCIPCData_t *);
	RTCIPCNode_t *Creaet(int type);
public:
	IPCMediaSourceService();
	static int sTransport(RTCIPCHandleInterface *thiz,
		const RTCIPCData_t *p)
	{
		IPCMediaSourceService *me = static_cast<IPCMediaSourceService *>(thiz);
		return me->Transport(p);
	}
	static RTCIPCNode_t *sCreaet(RTCIPCHandleInterface *thiz, int type)
	{
		IPCMediaSourceService *me = static_cast<IPCMediaSourceService *>(thiz);
		return me->Creaet(type);
	}
};

int IPCMediaSourceService::Transport(const RTCIPCData_t *p)
{
	// Ingore msg
	return 0;
}

RTCIPCNode_t * IPCMediaSourceService::Creaet(int type )
{
	return _nodeservices.Create(type);
}

IPCMediaSourceService::IPCMediaSourceService()
{
	RTCIPCHandleInterface::create = IPCMediaSourceService::sCreaet;
	RTCIPCHandleInterface::transport = IPCMediaSourceService::sTransport;
}

MediaSourceService::MediaSourceService():
_ipcservice(NULL)
{
}


MediaSourceService::~MediaSourceService(void)
{
	if (_ipcservice) {
		delete _ipcservice;
	} 
}

int MediaSourceService::init( const char *service )
{
	_name = service;
	_ipcservice = new IPCMediaSourceService;
	return 0;
}

int MediaSourceService::start()
{
		RTCIPCHandle_t *ipc = WebRTCAPI::Create()->GetIPCInteraceAPI();
		return ipc->start(_name.c_str(), _ipcservice,true);
}

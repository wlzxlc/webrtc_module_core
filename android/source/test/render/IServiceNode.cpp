#include "IServiceNode.h"
#include <algorithm>
#include "IInterface.h"
#include "ISource.h"

class RTCServiceNode : public RTCIPCNode {
public:
	RTCServiceNode() {
		RTCIPCNode::died = RTCServiceNode::sDied;
		RTCIPCNode::transport = RTCServiceNode::sTransport;
		RTCIPCNode::set_callback = RTCServiceNode::sSet_callback;
	}

	virtual int Transport(const RTCIPCData_t *) {return -1;}
	virtual void Died() {}
	virtual int setCallback(struct RTCIPCNode * /*cb*/) {return -1;}

	static int sTransport(struct RTCIPCNode *thiz,const RTCIPCData_t *p)
	{
		RTCServiceNode *me = static_cast<RTCServiceNode *>(thiz);
		return me->Transport(p);
	}

	static void sDied(struct RTCIPCNode *thiz)
	{
		RTCServiceNode *me = static_cast<RTCServiceNode *>(thiz);
		me->Died();
	}

	static int sSet_callback(struct RTCIPCNode *thiz, struct RTCIPCNode * cb)
	{
		RTCServiceNode *me = static_cast<RTCServiceNode *>(thiz);
		return me->setCallback(cb);
	}
};

class RTCServiceNodeSource :public ISource, public RTCServiceNode {
private:
	int _sourcetype;
	ISource *_source;
public:
	RTCServiceNodeSource(int type ,int w, int h, int color){
		_sourcetype = type;
		_source = ISource::Create(_sourcetype, w, h, color);
	}
	virtual int width() {if (_source) return _source->width(); return -1;}
	virtual int height() {if (_source) return _source->height(); return -1;}
	virtual int color() {if (_source) return _source->color(); return -1;}
	virtual int start() {if (_source) return _source->start(); return -1;}
	virtual int stop ()  {if (_source) return _source->stop(); return -1;}
	virtual int registerDeliver(IDeliver * stream) {if (_source) return _source->registerDeliver(stream); return -1;}
	virtual int deregisterDeliver(IDeliver *stream) {if (_source) return _source->deregisterDeliver(stream); return -1;}
	virtual int sourceType() {return _sourcetype;}

	virtual int Transport(const RTCIPCData_t *p)
	{
        printf("%s:%s[L%d] data %p p->reply %p\n",__FILE__, __FUNCTION__,__LINE__, p->data, p->reply);
        Parcel arg(p->data);
        int type = arg.readInt32();
        printf("%s:%s[L%d] type = %d\n",__FILE__, __FUNCTION__,__LINE__, type);
		switch(type) {
		case ISource::kCallId_Start:
			return start();
		case ISource::kCallId_Width:
			return width();
		case ISource::kCallId_Stop:
			return stop();
		case ISource::kCallId_Height:
			return height();
		}
		return ERRNUM("Unplement");
	}
	virtual void Died() {}
	virtual int setCallback(struct RTCIPCNode * /*cb*/) {return -1;}
};

class RTCServiceNodeStream : public IInterfaceStream, public RTCServiceNode {
private:
	ServiceNode *_root;
	IRender *_render;
	RTCIPCNode *_client_cb;
	RTCServiceNodeSource * _connect_source;
public:
	RTCServiceNodeStream(ServiceNode *root):
	  _root(root),
	  _render(NULL),
	  _client_cb(NULL),
	  _connect_source(NULL){}
	virtual int SetRender(IRender *render) {_render = render; return 0;}
	virtual int ConnectSource(int source)
	{
		DEBUG("Service StreamNode source %d\n", source);
		if (_connect_source) {
			return 0;
		}
		RTCIPCNode *src = _root->FindSource(source);
		if (src) {
			RTCServiceNodeSource *s = static_cast<RTCServiceNodeSource *>(src);
			s->registerDeliver(this);
			_connect_source = s;
			return 0;
		}
		return -1;
	}
	virtual int DisConnectSource(int source)
	{
		if (_connect_source) {
			_connect_source->deregisterDeliver(this);
			_connect_source = NULL;
			return 0;
		}
		return -1;
	}

	virtual int deliver(TestVideoBuffer *buffer)
	{
		int ret = 0;
		if (_client_cb) {
            IPCArgs args;
            Parcel *in = args.data();
            in->writeInt32(kCallId_DeliverVideoBuffer);
            in->writeInt32(buffer->w);
            in->writeInt32(buffer->h);
            in->writeInt32(buffer->color);
            in->writeInt32(buffer->size);
            in->writeGraphicBuffer(const_cast<graphics_handle *>(buffer->gb));
			ret = _client_cb->transport(_client_cb, args.args());
		}
		if (_render) {
			_render->deliver(buffer);
		}
		return ret;
	}

	virtual int Transport(const RTCIPCData_t *p)
	{
        Parcel arg(p->data);
		switch (arg.readInt32()) {
		case kCallId_SetRender:
			{
				return -1;
			}
		case kCallId_ConnectSource:
			{
				return ConnectSource(arg.readInt32());
			}
		case kCallId_DisConnectSource:
			{
				return DisConnectSource(-1);
			}
		}
		return -1;
	}

	virtual void Died()
	{
		_root->EraseStream(this);
		if (_connect_source) {
			_connect_source->deregisterDeliver(this);
		}
		delete this;
	}

	virtual int setCallback(struct RTCIPCNode *cb)
	{
		_client_cb = cb;
		return 0;
	}
};

class ServiceNodeStream : public RTCIPCNode {

};

ServiceNode::ServiceNode(void)
{
	_stream.clear();
	_sources.clear();
}


ServiceNode::~ServiceNode(void)
{
}

int ServiceNode::EraseSource( RTCIPCNode_t * node)
{
	AutoLock lock(&_mutex);
	std::map<int, RTCIPCNode_t *>::iterator it = _sources.begin();
	while (it != _sources.end()) {
		if (it->second == node) {
			_sources.erase(it);
			break;
		}
		it++;
	}
	return 0;
}

int ServiceNode::EraseStream( RTCIPCNode_t * node)
{
	AutoLock lock(&_mutex);
	std::list<RTCIPCNode_t *>::iterator it = std::find(_stream.begin(),
		_stream.end(), node);
	if (it != _stream.end()) {
		_stream.erase(it);
	}
	return 0;
}

RTCIPCNode_t * ServiceNode::FindSource( int type )
{
	if (type != kNodeTypeSourceCam &&
		type != kNodeTypeSourceFile ) {
			return NULL;
	}
	std::map<int, RTCIPCNode_t *>::iterator it = _sources.find(type);
	if (it != _sources.end()) {
		return it->second;
	}
	return NULL;
}

RTCIPCNode_t * ServiceNode::Create( int type )
{
	AutoLock lock(&_mutex);
	RTCIPCNode_t* node = NULL;
	if (type != kNodeTypeSourceCam &&
		type != kNodeTypeSourceFile ) {
			node = new RTCServiceNodeStream(this);
			_stream.push_back(node);
	}else {
		// New Source node
		node = FindSource(type);
		if (node == NULL){
			DEBUG("Not found source[%d] with create it ",type);
			node = new RTCServiceNodeSource(type, 1280, 720, kTestColorNV12);
			_sources[type] = node;
		}
	}
	return node;
}


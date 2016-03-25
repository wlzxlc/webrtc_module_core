#pragma once
#include <ipc.h>
#include <map>
#include <list>
#include "Defines.h"

class ServiceNode
{
private:
	std::map<int ,RTCIPCNode_t *> _sources;
	std::list<RTCIPCNode_t *> _stream;
	Mutex _mutex;
public:
	int EraseSource(RTCIPCNode_t *);
	int EraseStream(RTCIPCNode_t *);
	RTCIPCNode_t *FindSource(int type);
public:
	ServiceNode();
	virtual RTCIPCNode_t *Create(int type);
	virtual ~ServiceNode();
};


#ifndef _DEFINES_H_
#define _DEFINES_H_
#include <pthread.h>
#include <stdio.h>
#include <list>
#include <unistd.h>
#include <linux/coda.h>
#ifndef ERRNUM
#define ERRNUM(fmt,...) -__LINE__
#endif

#ifndef DEBUG
#define DEBUG(fmt, ...) printf("%s:[%d]"fmt "\n",__FUNCTION__,__LINE__,## __VA_ARGS__);
#endif

#ifndef ALIGN
#define ALIGN(V, align) ((V + align - 1) & (~(align - 1)))
#endif

#ifndef CHECK
#include <assert.h>
#define CHECK assert
#endif
#include <ipc.h>
#include <WebRTCAPI.h>
class Parcel {
    private:
        parcel_t *_parcel;
        RTCIPCParcel_t *_handle;
        bool _have_parcel;
    public:
        Parcel() {
            _handle = &CCStone::WebRTCAPI::Create()->GetIPCInteraceAPI()->parcel;
            _parcel = _handle->create();
            _have_parcel = true;
        }

        Parcel(const parcel_t *p ) {
         _parcel = const_cast<parcel_t *>(p);
         _handle = &CCStone::WebRTCAPI::Create()->GetIPCInteraceAPI()->parcel;
         _have_parcel = false;
        }

        parcel_t *data() {
         return _parcel;
        }

        int writeInt32(int v) {
            return _handle->write32(_parcel, v);
        }
        int writeInt64(long long v) {
            return _handle->write64(_parcel, v);
        }
        int writeFloat(float v) {
            return _handle->writef(_parcel, v);
        }
        int writeDouble(double v) {
            return _handle->writed(_parcel, v);
        }
        int writeArray(const void *p, unsigned int size) {
            return _handle->write_array(_parcel, p, size);
        }
        int writeGraphicBuffer(graphics_handle *v) {
            return _handle->write_graphics_handle(_parcel, v);
        }
        int writeFd(int v) {
            return _handle->write_fd(_parcel, v);
        }
        int dataAvailSize() {
            return _handle->data_avail_size(_parcel);
        }
        int setPosition(int pos) {
            return _handle->set_position(_parcel, pos);
        }
        int position() {
            return _handle->position(_parcel);
        }
        ~Parcel() {
            if (_have_parcel)
                _handle->release(_parcel);
        }

        int readInt32() {
            return _handle->read32(_parcel);
        }
        long long readInt64() {
            return _handle->read64(_parcel);
        }
        float readFloat() {
            return _handle->readf(_parcel);
        }
        double readDouble() {
            return _handle->readd(_parcel);
        }
        int readFd() {
            return _handle->read_fd(_parcel);
        }
        int readArray(void *buffer, unsigned int size) {
            return _handle->read_array(_parcel, size, buffer);
        }
        const void * readArray(unsigned int size ) {
            return _handle->read_array_pointer(_parcel, size);
        }
        graphics_handle *readGraphicBuffer() {
            return _handle->read_graphics_handle(_parcel);
        }
};

class IPCArgs {
    private:
        Parcel _data;
        Parcel _reply;
        RTCIPCData_t _args;
    public:

        IPCArgs() {
            _args.data = _data.data();
            _args.reply = _reply.data();
        }

        IPCArgs(const RTCIPCData_t *data):
            _data(data->data),_reply(data->reply) {
        }

        Parcel *data() {
         return &_data;
        }

        Parcel *reply() {
         return &_reply;
        }

        RTCIPCData_t *args() {
            return &_args;
        }
};


class Thread {
private:
	bool _run;
	bool _exit;
	pthread_attr_t _attr;
private:
	static void *thread_loop(void *p)
	{
		Thread * me = static_cast<Thread *>(p);
		me->_run = true;
		while ( me->_run &&  me->run());
		me->_exit = true;
		me->_run = false;
		return NULL;
	}
public:
	virtual bool run(){return false;}
	Thread()
	{
		_exit = false;
		pthread_attr_init(&_attr);
	}
	unsigned int startLoop()
	{
		if (_run) {
			return 0;
		}

		pthread_t tid;
		pthread_create(&tid, &_attr, Thread::thread_loop, this);
		while (!_run) usleep(1000 * 5);
		return (unsigned int)(tid);
	}

	void stopLoop(bool async = false)
	{
		_run = false;
		if (async){
			while(!_exit) usleep(1000 * 10);
		}
	}

	~Thread()
	{
		stopLoop();
	}
};

class Condition;
class Mutex {
private:
    friend class Condition;
	pthread_mutex_t _mutex;
	pthread_mutexattr_t _attr; 
public:
	Mutex(){
		pthread_mutexattr_init(&_attr);
		pthread_mutex_init(&_mutex, &_attr);
	}

	void lock()
	{
		pthread_mutex_lock(&_mutex);
	}
	void unlock()
	{
		pthread_mutex_unlock(&_mutex);
	}
};

class AutoLock {
private:
	Mutex *_mutex;
public:
	AutoLock(Mutex *lock):_mutex(lock)
	{
		_mutex->lock();
	}
	~AutoLock()
	{
		_mutex->unlock();
	}
};

class Condition {
    private:
        pthread_condattr_t _attr;
        pthread_cond_t _cond;
    public:
        Condition() {
         pthread_condattr_init(&_attr);
         pthread_cond_init(&_cond, &_attr);
        }

        int wait(Mutex *mutex) {
            return pthread_cond_wait(&_cond, &mutex->_mutex);
        }

        int signal() {
           return pthread_cond_signal(&_cond);
        }

        int broadcast() {
          return pthread_cond_broadcast(&_cond);
        }

        int timedWait(Mutex *mutex, long long ms) {
            struct timespec spec;
            spec.tv_sec = ms / 1000;
            spec.tv_nsec = (ms % 1000) * 1E9;
            return pthread_cond_timedwait(&_cond,&mutex->_mutex, &spec);
        }

        ~Condition() {
        }
};
#endif

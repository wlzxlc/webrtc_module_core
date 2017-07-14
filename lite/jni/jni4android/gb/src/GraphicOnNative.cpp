/*
 * GraphicOnNative.cpp
 *
 *  Created on: Jul 5, 2017
 *      Author: lichao
 */
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include "GraphicOnNative.h"
#include <graphics_type.h>
#include <videobuffer.h>
#include "j4a_GraphicOnNative.h"
#include "j4a_Parcel.h"
#include "j4a_SystemGraphicBuffer.h"
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#define GL_GLEXT_PROTOTYPES 1
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#define PACKAGE_NAME "com/snail/gb/GraphicBuffer"

#ifndef ERRNUM
#define ERRNUM -__LINE__
#endif

#ifndef PRINTI
#define PRINTI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,"GraphicOnNative","[%s:%d] " fmt "\n",__FUNCTION__,__LINE__, ##__VA_ARGS__)
#endif

#ifndef PRINTE
#define PRINTE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "GraphicOnNative","[%s:%d] " fmt "\n",__FUNCTION__,__LINE__, ##__VA_ARGS__)
#endif

#ifndef PRINTW
#define PRINTW(fmt, ...) __android_log_print(ANDROID_LOG_WARN, "GraphicOnNative","[%s:%d] " fmt "\n",__FUNCTION__,__LINE__, ##__VA_ARGS__)
#endif

typedef struct NativeFd {
	int sdk;
	JavaVM *vm;
	OmxGraphics_t hwd;
	void *handle;
} NativeFd;

typedef struct GraphicBufferOnNative {
	int w;
	int h;
	graphics_pixformat src_color;
	graphics_handle *gb;
	EGLDisplay dpy;
	EGLImageKHR img;
	GLuint texId;
} GraphicBufferOnNative;
//////////////////////////////////////////////////////////////////////////////////////////////
#define ANDROID_NATIVE_MAKE_CONSTANT(a,b,c,d) \
    (((unsigned)(a)<<24)|((unsigned)(b)<<16)|((unsigned)(c)<<8)|(unsigned)(d))

#define ANDROID_NATIVE_WINDOW_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','w','n','d')

#define ANDROID_NATIVE_BUFFER_MAGIC \
    ANDROID_NATIVE_MAKE_CONSTANT('_','b','f','r')
typedef struct native_handle {
	int version; /* sizeof(native_handle_t) */
	int numFds; /* number of file-descriptors at &data[0] */
	int numInts; /* number of ints at &data[numFds] */
	int data[0]; /* numFds + numInts ints */
} native_handle_t;
typedef const native_handle_t* buffer_handle_t;
typedef struct android_native_base_t {
	/* a magic value defined by the actual EGL native type */
	int magic;

	/* the sizeof() of the actual EGL native type */
	int version;

	void* reserved[4];

	/* reference-counting interface */
	void (*incRef)(struct android_native_base_t* base);
	void (*decRef)(struct android_native_base_t* base);
} android_native_base_t;
typedef struct ANativeWindowBuffer {
	ANativeWindowBuffer() {
		common.magic = ANDROID_NATIVE_BUFFER_MAGIC;
		common.version = sizeof(ANativeWindowBuffer);
		memset(common.reserved, 0, sizeof(common.reserved));
	}

	// Implement the methods that sp<ANativeWindowBuffer> expects so that it
	// can be used to automatically refcount ANativeWindowBuffer's.
	void incStrong(const void* /*id*/) const {
		common.incRef(const_cast<android_native_base_t*>(&common));
	}
	void decStrong(const void* /*id*/) const {
		common.decRef(const_cast<android_native_base_t*>(&common));
	}

	struct android_native_base_t common;

	int width;
	int height;
	int stride;
	int format;
	int usage;

	void* reserved[2];

	buffer_handle_t handle;

	void* reserved_proc[8];
} ANativeWindowBuffer_t;
//////////////////////////////////////////////////////////////////////////////////
static NativeFd _gfd;

typedef struct NativeGraphicBufferFromJava {
	jobject gb;
	ANativeWindowBuffer_t * awb;
	int fdIdx;
	void *base;
	jobject mCanvas;
} NativeObjectOnJava;

inline JNIEnv *getEnv() {
	JNIEnv *env = NULL;
	_gfd.vm->GetEnv((void **) &env, JNI_VERSION_1_6);
	return env;
}

inline int getPixformatSize(int w, int h, int pixformat) {
	int size = 0;
	switch (pixformat) {
	case IOMX_HAL_PIXEL_FORMAT_RGB_888: {
		size = w * h * 3;
		break;
	}
	case IOMX_HAL_PIXEL_FORMAT_RGBA_8888: {
		size = w * h * 4;
		break;
	}
	case IOMX_HAL_PIXEL_FORMAT_RGB_565: {
		size = w * h * 2;
		break;
	}
	default:
		break;
	}
	return size;
}

static void dumpAWB(ANativeWindowBuffer_t *t) {
	PRINTI("====================DUMP awb ===========================");
	PRINTI("awb: %p", t);
	if (t) {
		PRINTI("common.magic 0x%x VS 0x%x", t->common.magic,
				(int)ANDROID_NATIVE_BUFFER_MAGIC);
		PRINTI("common.version %d VS %d", t->common.version,
				(int )sizeof(ANativeWindowBuffer));
		PRINTI("W:%d H:%d Format:%d Stride:%d Usage: 0x%x", t->width, t->height,
				t->format, t->stride, t->usage);
		PRINTI("handle.version %d VS %d", t->handle->version,
				(int )sizeof(native_handle_t));
		PRINTI("handle.numFds %d", t->handle->numFds);
		PRINTI("handle.numInts %d", t->handle->numInts);

		int idx = 0;
		for (; idx < t->handle->numFds; idx++) {
			PRINTI("data[%d] = %d (fd)", idx, t->handle->data[idx]);
		}

		for (int ints = 0; ints < t->handle->numInts; ints++) {
			PRINTI("data[%d] = %d", idx + ints, t->handle->data[idx + ints]);
		}
	}
	PRINTI("===============================================---------=");
}

static void destroyEGLImageKHR(GraphicBufferOnNative &gb) {
	if (gb.img) {
		eglDestroyImageKHR(gb.dpy, gb.img);
		if (gb.texId) {
			glDeleteTextures(1, &gb.texId);
			gb.texId = 0;
		}
		gb.img = NULL;
	}
}

static int createEGLImageKHR(GraphicBufferOnNative &gb, int fb) {
	GLuint sharedTextureID = 0;

	if (gb.img) {
		destroyEGLImageKHR(gb);
	}
	// Create the EGLImageKHR from the native buffer
	EGLint eglImgAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
	EGL_NONE };

	gb.dpy = eglGetCurrentDisplay();
	if (gb.dpy == NULL) {
		PRINTE("eglGetCurrentDisplay return null");
		return ERRNUM;
	}

	void *anw = _gfd.hwd.winbuffer(gb.gb);
	if (!anw) {
		PRINTE("anw is null");
		return ERRNUM;
	}

	ANativeWindowBuffer_t *t = (ANativeWindowBuffer_t *) anw;
	if (t->common.magic != ANDROID_NATIVE_BUFFER_MAGIC) {
		PRINTE("BUG !!! Invalid anw !!!");
		const char *cc = (char *) t;
		for (int i = 0; i < 32; i++) {
			PRINTE("cc[%d] = %c ", i, cc[i]);
		}
	}

	EGLImageKHR gImage = eglCreateImageKHR(gb.dpy, EGL_NO_CONTEXT,
	EGL_NATIVE_BUFFER_ANDROID, (EGLClientBuffer) anw, eglImgAttrs);
	if (gImage == EGL_NO_IMAGE_KHR) {
		PRINTE("create EGLImageKHR faild");
		return ERRNUM;
	}

	// bind an off-screen frame buffer from input.
	glBindFramebuffer(GL_FRAMEBUFFER, fb);

	// Create GL texture, bind to GL_TEXTURE_2D, etc.
	glGenTextures(1, &sharedTextureID);
	glBindTexture(GL_TEXTURE_2D, sharedTextureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	// Attach the EGLImage to whatever texture is bound to GL_TEXTURE_2D
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, gImage);
	gb.img = gImage;
	gb.texId = sharedTextureID;
	return gb.texId;
}

static int jlock(graphics_handle *hwd, uint32_t usage, void** vaddr) {
	JNIEnv *env = getEnv();
	if (!hwd || env == NULL)
		return ERRNUM;
	NativeGraphicBufferFromJava * o = (NativeGraphicBufferFromJava *) hwd;

	if (o->base) {
		*vaddr = o->base;
		return 0;
	}

	if (o->fdIdx >= o->awb->handle->numFds) {
		PRINTE("Invalid mmap fdIDX.");
		return ERRNUM;
	}

	int fd = o->awb->handle->data[o->fdIdx];
	int offset = 0;

	if (fd <= 0)
		return ERRNUM;

	int size = getPixformatSize(o->awb->stride, o->awb->height, o->awb->format);

	if (!size) return ERRNUM;

	o->mCanvas = J4AC_android_view_GraphicBuffer__lockCanvas(env, o->gb);

	if (o->mCanvas == NULL) {
		PRINTE("Lock canvas faild");
		return ERRNUM;
	}


	void *addr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd,
			offset);
	if (!addr || addr == MAP_FAILED) {
		PRINTE("mmap vaddr faild. fdIDX %d", o->fdIdx);
		while (++o->fdIdx < o->awb->handle->numFds) {
			fd = o->awb->handle->data[o->fdIdx];
			addr = mmap(NULL, size, PROT_WRITE | PROT_READ, MAP_SHARED, fd,
					offset);
			if (!addr || addr == MAP_FAILED) {
				PRINTE("mmap vaddr faild. fd %d", o->fdIdx);
			} else {
				break;
			}
		}
	}

	if (!addr || addr == MAP_FAILED) {
		J4AC_android_view_GraphicBuffer__unlockCanvasAndPost(env, o->gb, o->mCanvas);
		o->mCanvas = NULL;
		return ERRNUM;
	}else {
		o->mCanvas = J4A_NewGlobalRef__catchAll(env, o->mCanvas);
	}

	o->base = addr;
	*vaddr = o->base;
	return 0;
}

static int junlock(graphics_handle *hwd) {
	if (!hwd)
		return ERRNUM;
	NativeGraphicBufferFromJava * o = (NativeGraphicBufferFromJava *) hwd;
	JNIEnv * env = getEnv();
	if (o->base) {
		munmap(o->base,
				getPixformatSize(o->awb->stride, o->awb->height,
						o->awb->format));
		o->base = NULL;
		J4AC_android_view_GraphicBuffer__unlockCanvasAndPost(env, o->gb, o->mCanvas);
		J4A_DeleteGlobalRef(env, o->mCanvas);
		o->mCanvas = NULL;
	}
	return 0;
}

static graphics_handle *jalloc(int w, int h, graphics_pixformat pixfmt,
		int usage) {
	JNIEnv *env = getEnv();
	jobject gb = J4AC_android_view_GraphicBuffer__create__asGlobalRef__catchAll(
			env, w, h, pixfmt, usage);
	if (!gb) {
		PRINTE("Create object GraphicBuffer failed.");
		return NULL;
	}

	jlong gbwrapper =
			J4AC_android_view_GraphicBuffer__mNativeObject__get__catchAll(env,
					gb);
	ANativeWindowBuffer_t ** ptr = (ANativeWindowBuffer_t **) gbwrapper;

	char *tp = (char *) (*ptr);
	int found = 0;
	for (int i = 0; i < 32; i++) {
		if (strncmp(tp + i, "rfb_", 4) == 0) {
			tp = tp + i;
			found = 1;
			break;
		}
	}

	if (!found) {
		PRINTE("Not found magic 'rfb_'");
		J4A_DeleteGlobalRef(env, gb);
		return NULL;
	}
	ANativeWindowBuffer_t *nb = reinterpret_cast<ANativeWindowBuffer_t *>(tp);
	if (nb->width != w || nb->height != h || nb->format != pixfmt
			|| nb->usage != usage) {
		PRINTE("Unknown ANativeWindowBuffer_t struct.!!!");
		J4A_DeleteGlobalRef(env, gb);
		return NULL;
	} else {
		dumpAWB(nb);
	}
	NativeGraphicBufferFromJava * o = new NativeGraphicBufferFromJava;
	memset(o, 0, sizeof(NativeGraphicBufferFromJava));
	o->gb = gb;
	o->awb = nb;
	o->fdIdx = 0;
	PRINTI("create gb %p", o);
	return o;
}

static unsigned int jstride(graphics_handle *hwd) {
	NativeGraphicBufferFromJava *o = (NativeGraphicBufferFromJava *) hwd;
	if (o) {
		return o->awb->stride;
	}
	return 0;
}

static void jdestory(graphics_handle *hwd) {
	JNIEnv *env = getEnv();
	if (!hwd || env == NULL)
		return;

	NativeGraphicBufferFromJava *o = (NativeGraphicBufferFromJava *) hwd;
	PRINTI("destroy gb %p", o);
	junlock(hwd);
	J4A_DeleteGlobalRef(env, o->gb);
	delete o;
}

static void *jwinbuffer(graphics_handle *hwd) {
	NativeGraphicBufferFromJava *o = (NativeGraphicBufferFromJava *) hwd;
	if (o) {
		return o->awb;
	}
	return NULL;
}

static int InitVideoBufferFromJava(OmxGraphics_t *hgb, JNIEnv *env) {
#if 0
	jobject gb = J4AC_android_view_GraphicBuffer__create__asGlobalRef__catchAll(env, 1280, 720,
			IOMX_HAL_PIXEL_FORMAT_RGB_888,
			IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN |
			IOMX_GRALLOC_USAGE_SW_READ_OFTEN |
			IOMX_GRALLOC_USAGE_HW_TEXTURE);
	if (!gb) {
		PRINTE("Create object GraphicBuffer failed.");
		return ERRNUM;
	} else {
		jobject parcel = J4AC_android_os_Parcel__obtain__asGlobalRef__catchAll(env);
		if (!parcel) {
			PRINTE("Create object Parcel failed.");
			J4A_DeleteGlobalRef(env, gb);
			return ERRNUM;
		}
		J4AC_android_view_GraphicBuffer__writeToParcel__catchAll(env, gb, parcel, 0);

		PRINTI("Ready read 4 byte from parcel, total %d byte", J4AC_android_os_Parcel__dataSize(env, parcel));
		J4AC_android_os_Parcel__setDataPosition(env, parcel, 0);
		PRINTI(" w %d h %d format %d usage %d",
				J4AC_android_os_Parcel__readInt(env, parcel),
				J4AC_android_os_Parcel__readInt(env, parcel),
				J4AC_android_os_Parcel__readInt(env, parcel),
				J4AC_android_os_Parcel__readInt(env, parcel));
		jlong gbwrapper =J4AC_android_view_GraphicBuffer__mNativeObject__get__catchAll(env, gb);
		ANativeWindowBuffer_t ** ptr = (ANativeWindowBuffer_t **)gbwrapper;

		PRINTI("ptr　%p", ptr);
		PRINTI("*ptr　%p", *ptr);

		char *tp = (char *)(*ptr);
		int found = 0;
		for (int i = 0;i < 32; i++) {
			if (strncmp(tp + i, "rfb_", 4) == 0 ) {
				tp = tp + i;
				found = 1;
				break;
			}
		}
		if (!found) {
			PRINTE("Not found magic 'rfb_'");
			J4A_DeleteGlobalRef(env, parcel);
			J4A_DeleteGlobalRef(env, gb);
			return ERRNUM;
		}
		ANativeWindowBuffer_t &nb = *reinterpret_cast<ANativeWindowBuffer_t *>(tp);
		PRINTI("w %d h %d format %d usage %d handle　%p fds %d ints %d", nb.width, nb.height,nb.format,nb.usage, nb.handle, nb.handle->numFds, nb.handle->numInts);
		for (int idx = 0; idx < nb.handle->numFds + nb.handle->numInts - 1; idx++) {
			PRINTI(" idx %d value %d", idx, nb.handle->data[idx]);
		}

		void *addr = mmap(NULL ,1280 * 720 * 3, PROT_WRITE,MAP_SHARED, nb.handle->data[0], 0);
		if (addr) {
			memset(addr, 0, 1280 * 720 * 3);
			munmap(addr, 1280 * 720 * 3);
		} else {
			PRINTI("mmap addr failed.");
		}
		J4A_DeleteGlobalRef(env, parcel);
		J4A_DeleteGlobalRef(env, gb);
		PRINTI("Test done.");
	}
#endif
	memset(hgb, 0, sizeof(OmxGraphics_t));
	hgb->alloc = jalloc;
	hgb->destory = jdestory;
	hgb->lock = jlock;
	hgb->unlock = junlock;
	hgb->winbuffer = jwinbuffer;
	hgb->stride = jstride;
	return 0;
}

static jint load(JNIEnv *env, jobject obj, jint sdk, jstring path) {
	int ret = ERRNUM;
	long ptr = 0;
	void *handle = NULL;
	if (_gfd.handle == NULL) {
		int tsdk = sdk;
		OmxGraphics_t gbhand;
		if (sdk > 1) {
			const char* dir;
			dir = env->GetStringUTFChars(path, (jboolean *)NULL);
			PRINTI("Using Native GraphicBuffer module, path %s", dir);
			while (tsdk > 10) {
				char buffer[512] = { 0 };
				if (dir == NULL)
				 sprintf(buffer, "libgb_%d.so", tsdk);
				else
				 sprintf(buffer, "%s/libgb_%d.so", dir, tsdk);
				PRINTI("Dlopen %s...", buffer);
				handle = dlopen(buffer, RTLD_NOW);
				if (handle) {
					PRINTI("Loaded gb module succeed with sdk %d", tsdk);
					break;
				}
				tsdk--;
			}

			if (dir)
			env->ReleaseStringUTFChars(path, dir);

			if (!handle) {
				PRINTE("Loading gb module faild (%s).", dlerror());
				ret = ERRNUM;
				goto faild;
			}
			ret = InitVideoBuffer(&gbhand, handle);

		} else {
			PRINTI("Using Java GraphicBuffer module.");
			ret = InitVideoBufferFromJava(&gbhand, env);
		}
		if (ret) {
			PRINTE("Init gb method faild (%d).", ret);
			ret = ERRNUM;
			goto faild;
		} else if (gbhand.alloc == NULL || gbhand.lock == NULL
				|| gbhand.unlock == NULL || gbhand.destory == NULL) {
			PRINTE("GraphicBuffer method is imperfect.");
			dlclose(_gfd.handle);
			_gfd.handle = NULL;
			ret = ERRNUM;
			goto faild;
		} else {
			PRINTI("Hook graphic method succeed, alloc %p, destroy %p",
					gbhand.alloc, gbhand.destory);
		}
		_gfd.handle = handle;
		_gfd.hwd = gbhand;
	}

	ret = 0;
	ptr = J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(env, obj);
	if (!ptr) {
		GraphicBufferOnNative * ngb = new GraphicBufferOnNative;
		memset(ngb, 0, sizeof(*ngb));
		J4AC_com_snail_gb_GraphicBuffer__mNativeObject__set__catchAll(env, obj,
				(long) ngb);
	} else {
		PRINTI("already exist native object %ld", ptr);
	}
	faild: return ret;
}

static jint createFrameBufferAndBind(JNIEnv *env, jobject obj, jint w, jint h,
		jint color, int fd) {
	GraphicBufferOnNative *ngb =
			(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
					env, obj);

	if (!ngb)
		return ERRNUM;

	switch (color) {
	case IOMX_HAL_PIXEL_FORMAT_RGBA_8888:
	case IOMX_HAL_PIXEL_FORMAT_RGBX_8888:
	case IOMX_HAL_PIXEL_FORMAT_RGB_888:
	case IOMX_HAL_PIXEL_FORMAT_RGB_565:
		break;
	default:
		return ERRNUM;
	}

	if (w < 1 || h < 1)
		return ERRNUM;

	graphics_handle *gb = _gfd.hwd.alloc(w, h, color,
			IOMX_GRALLOC_USAGE_HW_TEXTURE |
			IOMX_GRALLOC_USAGE_HW_FB |
			IOMX_GRALLOC_USAGE_SW_READ_OFTEN |
			IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN
			);

	void * addr = NULL;
	_gfd.hwd.lock(gb, IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN, &addr);
	if (addr) {
		int size = getPixformatSize(w, h, ngb->src_color);
		PRINTI("Test write memory size %d", size);
		memset(addr, 0, size);
		PRINTI("Test write memory done.");
		_gfd.hwd.unlock(gb);
	}

	if (!gb)
		return ERRNUM;

	ngb->src_color = color;
	ngb->gb = gb;
	ngb->w = w;
	ngb->h = h;
	int tex = createEGLImageKHR(*ngb, fd);
	if (tex <= 0) {
		PRINTE("createEGLImageKHR create!");
		_gfd.hwd.destory(gb);
		ngb->gb = NULL;
		return ERRNUM;
	}
	return tex;
}

static jint destroyFrameBuffer(JNIEnv *env, jobject obj) {
	GraphicBufferOnNative *ngb =
			(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
					env, obj);
	if (ngb) {
		destroyEGLImageKHR(*ngb);
		if (ngb->gb) {
			_gfd.hwd.destory(ngb->gb);
		}
		delete ngb;
		J4AC_com_snail_gb_GraphicBuffer__mNativeObject__set__catchAll(env, obj,
				0);
	}
	return 0;
}

static jlong lockAddr(JNIEnv *env, jobject obj) {
	int ret = ERRNUM;
	void *addr = NULL;
	GraphicBufferOnNative *ngb =
			(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
					env, obj);
	if (ngb) {
		if (ngb->gb) {

			ret = _gfd.hwd.lock(ngb->gb,
					IOMX_GRALLOC_USAGE_SW_READ_OFTEN
							| IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN, &addr);
		}
	}
	return (jlong) addr;
}

static jobject lockBuffer(JNIEnv *env, jobject obj) {
	jbyteArray array = NULL;
	void *addr = (void *) lockAddr(env, obj);
	if (addr) {
		GraphicBufferOnNative *ngb =
				(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
						env, obj);
		int size = getPixformatSize(ngb->w, ngb->h, ngb->src_color);
		array = env->NewByteArray(size);

		char *p = (char *) env->GetByteArrayElements(array, 0);
		const char *src = (const char *) addr;
		int stride = _gfd.hwd.stride(ngb->gb);
		for (int row = 0; row < ngb->h; row++) {
			memcpy(p, src, ngb->w * 4);
			p += ngb->w * 4;
			src += stride * 4;
		}
		return array;
	}
	return NULL;
}

static jint unlockBuffer(JNIEnv *env, jobject obj) {
	GraphicBufferOnNative *ngb =
			(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
					env, obj);
	if (ngb) {
		if (ngb->gb)
			_gfd.hwd.unlock(ngb->gb);
	}
	return 0;
}

static jint stride(JNIEnv *env, jobject obj) {
	GraphicBufferOnNative *ngb =
			(GraphicBufferOnNative *) J4AC_com_snail_gb_GraphicBuffer__mNativeObject__get(
					env, obj);
	int stride = 0;
	if (ngb && ngb->gb) {
		stride = _gfd.hwd.stride(ngb->gb);
	}
	return stride;
}

static JNINativeMethod gMethods[] = { { "_load", "(ILjava/lang/String;)I", (void *) load }, {
		"_createFrameBufferAndBind", "(IIII)I",
		(void *) createFrameBufferAndBind }, { "_destroyFrameBuffer", "()I",
		(void *) destroyFrameBuffer }, { "_lockBuffer", "()[B",
		(void *) lockBuffer }, { "_unlock", "()I", (void *) unlockBuffer }, {
		"_stride", "()I", (void *) stride }, { "_lock", "()J",
		(void *) lockAddr } };

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv *env = NULL;
	_gfd.vm = vm;

	PRINTI("JNI_Onload! on %d bit", (int )(sizeof(long) * 8));
	vm->GetEnv((void **) &env, JNI_VERSION_1_6);

	if (!env) {
		PRINTE("Get env faild.");
		return ERRNUM;
	}

	jclass clazz = env->FindClass(PACKAGE_NAME);
	if (!clazz) {
		PRINTE("Not found class %s", PACKAGE_NAME);
		return ERRNUM;
	}

	if (env->RegisterNatives(clazz, gMethods,
			sizeof(gMethods) / sizeof(gMethods[0]))) {
		PRINTE("Register native methods faild.");
		return ERRNUM;
	}

	if (J4A_loadClass__J4AC_com_snail_gb_GraphicBuffer(env)) {
		PRINTE("Loading class GraphicBuffer error.");
		return ERRNUM;
	}

	if (J4A_loadClass__J4AC_android_view_GraphicBuffer(env)) {
		PRINTW("Loading class android_view_GraphicBuffer error.");
		return JNI_VERSION_1_6;
	}

	if (J4A_loadClass__J4AC_android_os_Parcel(env)) {
		PRINTW("Loading class android_os_Parcel error.");
		return JNI_VERSION_1_6;
	}
	return JNI_VERSION_1_6;
}

void JNI_OnUnload(JavaVM* vm, void* reserved) {
	if (_gfd.handle) {
		dlclose(_gfd.handle);
	}
}


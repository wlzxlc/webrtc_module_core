/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/resource.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <assert.h>
#include <dlfcn.h>
#include "WebRTCAPI.h"

#undef NDEBUG

using namespace CCStone;

static void printGLString(const char *name, GLenum s) {
    // fprintf(stderr, "printGLString %s, %d\n", name, s);
    const char *v = (const char *) glGetString(s);
    // int error = glGetError();
    // fprintf(stderr, "glGetError() = %d, result of glGetString = %x\n", error,
    //        (unsigned int) v);
    // if ((v < (const char*) 0) || (v > (const char*) 0x10000))
    //    fprintf(stderr, "GL %s = %s\n", name, v);
    // else
    //    fprintf(stderr, "GL %s = (null) 0x%08x\n", name, (unsigned int) v);
    fprintf(stderr, "GL %s = %s\n", name, v);
}

static void checkEglError(const char* op, EGLBoolean returnVal = EGL_TRUE) {
    if (returnVal != EGL_TRUE) {
        fprintf(stderr, "%s() returned %d\n", op, returnVal);
    }

    for (EGLint error = eglGetError(); error != EGL_SUCCESS; error
            = eglGetError()) {
        fprintf(stderr, "after %s() (0x%x)\n", op,
                error);
    }
}

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        fprintf(stderr, "after %s() glError (0x%x)\n", op, error);
    }
}

#if 0
static const char gVertexShader[] = "attribute vec4 vPosition;\n"
    "varying vec2 yuvTexCoords;\n"
    "void main() {\n"
    "  yuvTexCoords = vPosition.xy + vec2(0.5, 0.5);\n"
    "  gl_Position = vPosition;\n"
    "}\n";
#else

    // varying [vec2, yuvTexCoords] shared by vertex/fragment
    // attribute [vec4, vPosition] only used by vertex. 
static const char gVertexShader[] = "attribute vec4 vPosition;\n"
    "varying vec2 yuvTexCoords;\n"
    "varying vec2 overlayTexCoords;\n"
    "void main() {\n"
    "float nx,ny;\n"
    "nx = vPosition.x + 1.0;\n"
    "ny = vPosition.y - 1.0;\n"
    "nx = nx / 2.0;\n"
    "ny = ny / 2.0;\n"
    "  yuvTexCoords = vec2(abs(nx), abs(ny));\n"
    "  overlayTexCoords = yuvTexCoords;\n"
    "  gl_Position = vPosition;\n"
    "}\n";
#endif

static const char gFragmentShader[] = "#extension GL_OES_EGL_image_external : require\n"
    "precision mediump float;\n"
    "uniform samplerExternalOES yuvTexSampler;\n"
    "uniform samplerExternalOES overlayTexSampler;\n"
    "varying vec2 yuvTexCoords;\n"
    "varying vec2 overlayTexCoords;\n"
    "void main() {\n"
    "vec4 temp_fragColor;\n"
    "  temp_fragColor = texture2D(overlayTexSampler, overlayTexCoords);\n"
    " /*gl_FragColor = texture2D(yuvTexSampler, yuvTexCoords) + temp_fragColor;\n*/"
    " gl_FragColor = temp_fragColor;\n"
    "}\n";

GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLuint shader = glCreateShader(shaderType);
    printf("!!! shader %d\n", shader);
    if (shader) {
        glShaderSource(shader, 1, &pSource, NULL);
        glCompileShader(shader);
        GLint compiled = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (!compiled) {
            GLint infoLen = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
            if (infoLen) {
                char* buf = (char*) malloc(infoLen);
                if (buf) {
                    glGetShaderInfoLog(shader, infoLen, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            } else {
                fprintf(stderr, "Guessing at GL_INFO_LOG_LENGTH size\n");
                char* buf = (char*) malloc(0x1000);
                if (buf) {
                    glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                    fprintf(stderr, "Could not compile shader %d:\n%s\n",
                            shaderType, buf);
                    free(buf);
                }
            }
            glDeleteShader(shader);
            shader = 0;
        }
    }
    return shader;
}

GLuint createProgram(const char* pVertexSource, const char* pFragmentSource) {
    GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
    if (!vertexShader) {
        return 0;
    }

    GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
    if (!pixelShader) {
        return 0;
    }

    GLuint program = glCreateProgram();
    if (program) {
        glAttachShader(program, vertexShader);
        checkGlError("glAttachShader");
        glAttachShader(program, pixelShader);
        checkGlError("glAttachShader");
        glLinkProgram(program);
        GLint linkStatus = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
        if (linkStatus != GL_TRUE) {
            GLint bufLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
            if (bufLength) {
                char* buf = (char*) malloc(bufLength);
                if (buf) {
                    glGetProgramInfoLog(program, bufLength, NULL, buf);
                    fprintf(stderr, "Could not link program:\n%s\n", buf);
                    free(buf);
                }
            }
            glDeleteProgram(program);
            program = 0;
        }
    }
    return program;
}

GLuint gProgram;
GLint gvPositionHandle;
GLint gYuvTexSamplerHandle;
GLint gOverlayTexSamplerHandle;

bool setupGraphics(int w, int h) {
    gProgram = createProgram(gVertexShader, gFragmentShader);
    if (!gProgram) {
        return false;
    }
    gvPositionHandle = glGetAttribLocation(gProgram, "vPosition");
    checkGlError("glGetAttribLocation");
    fprintf(stderr, "glGetAttribLocation(\"vPosition\") = %d\n",
            gvPositionHandle);
    gYuvTexSamplerHandle = glGetUniformLocation(gProgram, "yuvTexSampler");
    checkGlError("glGetUniformLocation");
    fprintf(stderr, "glGetUniformLocation(\"yuvTexSampler\") = %d\n",
            gYuvTexSamplerHandle);


    gOverlayTexSamplerHandle = glGetUniformLocation(gProgram, "overlayTexSampler");
    fprintf(stderr, "glGetUniformLocation(\"overlayTexSampler\") = %d\n",
            gOverlayTexSamplerHandle);
    checkGlError("glGetUniformLocation");

    int x = 0;
    int y = 0;
    printf("Set view %d, %d, %d, %d\n", x, y, w, h);
    glViewport(x, y, w, h);
    checkGlError("glViewport");
    return true;
}

int align(int x, int a) {
    return (x + (a-1)) & (~(a-1));
}

const int yuvTexWidth = 1920;
const int yuvTexHeight = 1080;
const int yuvTexUsage = IOMX_GRALLOC_USAGE_HW_TEXTURE; 
const int yuvTexFormat = 0x32315659;
const int overlayFormat = IOMX_HAL_PIXEL_FORMAT_RGBA_8888;
//const int yuvTexFormat = 0x23;
const int yuvTexOffsetY = 0;
const bool yuvTexSameUV = false;
static RTCSurface * targetSurface;
static OmxGraphics_t *api;
static graphics_handle *yuvTexBuffer;
static graphics_handle *backTexBuffer;
static graphics_handle *frontTexBuffer;
static GLuint yuvTex[3]; /*yuv stream, back frame, front frame*/

#define TEX_YUV 0
#define TEX_BACK 1
#define TEX_FRONT 2

bool setupYuvTexSurface(EGLDisplay dpy, EGLContext context) {
 if (!api) {
  api = WebRTCAPI::Create()->GetGraphicsAPI();
 } 

 if (!yuvTexBuffer) {
  yuvTexBuffer = api->alloc(yuvTexWidth, yuvTexHeight, yuvTexFormat , IOMX_GRALLOC_USAGE_HW_TEXTURE);
  backTexBuffer = api->alloc(yuvTexWidth, yuvTexHeight, overlayFormat, 
          IOMX_GRALLOC_USAGE_HW_TEXTURE);
  frontTexBuffer = api->alloc(yuvTexWidth, yuvTexHeight, overlayFormat, 
          IOMX_GRALLOC_USAGE_HW_TEXTURE);
  assert(yuvTexBuffer && backTexBuffer && frontTexBuffer);
  void *backbuffer = NULL;
  api->lock(backTexBuffer, IOMX_GRALLOC_USAGE_SW_WRITE_RARELY, &backbuffer);
  for (int height = 0; height <= 1080; height++) {
   memset(backbuffer, rand(), 1920 * 4);
  }
  api->unlock(backTexBuffer);
 }

   const char *filename = "1080p_nv21.yuv";
   FILE *yuv = fopen(filename, "r");
   if (!yuv){
    printf("Open %s failed. \n", filename);
   }else {
    void *buffer = NULL;
    api->lock(yuvTexBuffer,IOMX_GRALLOC_USAGE_SW_WRITE_RARELY, &buffer);
    assert(buffer);
    fread(buffer, yuvTexWidth * yuvTexHeight * 3 / 2, 1, yuv);
    fclose(yuv);
    api->unlock(yuvTexBuffer);
   }

// Create textures and contain the three texure.
    glGenTextures(3, yuvTex);
    checkGlError("glGenTextures");
//////////////////////////Setup yuv texture//////////////////
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, yuvTex[TEX_YUV]);
    checkGlError("glBindTexture");
    EGLClientBuffer clientBuffer = (EGLClientBuffer) api->winbuffer(yuvTexBuffer);
    EGLImageKHR img = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer, 0);
    checkEglError("eglCreateImageKHR");
    if (img == EGL_NO_IMAGE_KHR) {
        return false;
    }
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img);
    checkGlError("glEGLImageTargetTexture2DOES");

/////////////////////////Setup back texture///////////////////
    glBindTexture(GL_TEXTURE_2D, yuvTex[TEX_BACK]);
    checkGlError("glBindTexture");
    clientBuffer = (EGLClientBuffer) api->winbuffer(backTexBuffer);
    img = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer, 0);
    checkEglError("eglCreateImageKHR");
    if (img == EGL_NO_IMAGE_KHR) {
        return false;
    }
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img);
    checkGlError("glEGLImageTargetTexture2DOES");

/////////////////////////Setup front texture///////////////////
    glBindTexture(GL_TEXTURE_2D, yuvTex[TEX_FRONT]);
    checkGlError("glBindTexture");
    clientBuffer = (EGLClientBuffer) api->winbuffer(frontTexBuffer);
    img = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer, 0);
    checkEglError("eglCreateImageKHR");
    if (img == EGL_NO_IMAGE_KHR) {
        return false;
    }
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)img);
    checkGlError("glEGLImageTargetTexture2DOES");

    return true;
}

#if 0
bool setupYuvTexSurface(EGLDisplay dpy, EGLContext context) {
    int blockWidth = yuvTexWidth > 16 ? yuvTexWidth / 16 : 1;
    int blockHeight = yuvTexHeight > 16 ? yuvTexHeight / 16 : 1;
    yuvTexBuffer =  new GraphicBuffer(yuvTexWidth, yuvTexHeight, yuvTexFormat, yuvTexUsage);
    int yuvTexStrideY = yuvTexBuffer->getStride();
    int yuvTexOffsetV = yuvTexStrideY * yuvTexHeight;
    int yuvTexStrideV = (yuvTexStrideY/2 + 0xf) & ~0xf;
    int yuvTexOffsetU = yuvTexOffsetV + yuvTexStrideV * yuvTexHeight/2;
    int yuvTexStrideU = yuvTexStrideV;
    char* buf = NULL;
    int err = yuvTexBuffer->lock(GRALLOC_USAGE_SW_WRITE_OFTEN, (void**)(&buf));
    if (err != 0) {
        fprintf(stderr, "yuvTexBuffer->lock(...) failed: %d\n", err);
        return false;
    }
    for (int x = 0; x < yuvTexWidth; x++) {
        for (int y = 0; y < yuvTexHeight; y++) {
            int parityX = (x / blockWidth) & 1;
            int parityY = (y / blockHeight) & 1;
            unsigned char intensity = (parityX ^ parityY) ? 63 : 191;
            buf[yuvTexOffsetY + (y * yuvTexStrideY) + x] = intensity;
            if (x < yuvTexWidth / 2 && y < yuvTexHeight / 2) {
                buf[yuvTexOffsetU + (y * yuvTexStrideU) + x] = intensity;
                if (yuvTexSameUV) {
                    buf[yuvTexOffsetV + (y * yuvTexStrideV) + x] = intensity;
                } else if (x < yuvTexWidth / 4 && y < yuvTexHeight / 4) {
                    buf[yuvTexOffsetV + (y*2 * yuvTexStrideV) + x*2 + 0] =
                    buf[yuvTexOffsetV + (y*2 * yuvTexStrideV) + x*2 + 1] =
                    buf[yuvTexOffsetV + ((y*2+1) * yuvTexStrideV) + x*2 + 0] =
                    buf[yuvTexOffsetV + ((y*2+1) * yuvTexStrideV) + x*2 + 1] = intensity;
                }
            }
        }
    }

    err = yuvTexBuffer->unlock();
    if (err != 0) {
        fprintf(stderr, "yuvTexBuffer->unlock() failed: %d\n", err);
        return false;
    }
    EGLClientBuffer clientBuffer = (EGLClientBuffer)yuvTexBuffer->getNativeBuffer();
    EGLImageKHR img = eglCreateImageKHR(dpy, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID,
            clientBuffer, 0);
    checkEglError("eglCreateImageKHR");
    if (img == EGL_NO_IMAGE_KHR) {
        return false;
    }

    glGenTextures(1, &yuvTex);
    checkGlError("glGenTextures");
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, yuvTex);
    checkGlError("glBindTexture");
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)img);
    checkGlError("glEGLImageTargetTexture2DOES");

    return true;
}
#endif
#if 0
const GLfloat gTriangleVertices[] = {
    -0.5f, 0.5f,
    -0.5f, -0.5f,
    0.5f, -0.5f,
    0.5f, 0.5f,
};
#else


const GLfloat gTriangleVertices[] = {
    -1.0f, 1.0f,
    1.0f, 1.0f,
    1.0f, -1.0f,
    -1.0f, -1.0f,
};
#endif

void renderFrame() {
    glClearColor(0.0f, 0.0f, 1.0f, 1.0f);
    checkGlError("glClearColor");
    glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    checkGlError("glClear");

    glUseProgram(gProgram);
    checkGlError("glUseProgram");

    glVertexAttribPointer(gvPositionHandle, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), gTriangleVertices);
    checkGlError("glVertexAttribPointer");
    glEnableVertexAttribArray(gvPositionHandle);
    checkGlError("glEnableVertexAttribArray");

    glUniform1i(gYuvTexSamplerHandle, 0);
    checkGlError("glUniform1i");
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, yuvTex[TEX_YUV]);
    checkGlError("glBindTexture");

    glUniform1i(gOverlayTexSamplerHandle, 1);
    checkGlError("glUniform1i");
    glBindTexture(GL_TEXTURE_2D, yuvTex[TEX_BACK]);
    checkGlError("glBindTexture");

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    checkGlError("glDrawArrays");
}

void printEGLConfiguration(EGLDisplay dpy, EGLConfig config) {

#define X(VAL) {VAL, #VAL}
    struct {EGLint attribute; const char* name;} names[] = {
    X(EGL_BUFFER_SIZE),
    X(EGL_ALPHA_SIZE),
    X(EGL_BLUE_SIZE),
    X(EGL_GREEN_SIZE),
    X(EGL_RED_SIZE),
    X(EGL_DEPTH_SIZE),
    X(EGL_STENCIL_SIZE),
    X(EGL_CONFIG_CAVEAT),
    X(EGL_CONFIG_ID),
    X(EGL_LEVEL),
    X(EGL_MAX_PBUFFER_HEIGHT),
    X(EGL_MAX_PBUFFER_PIXELS),
    X(EGL_MAX_PBUFFER_WIDTH),
    X(EGL_NATIVE_RENDERABLE),
    X(EGL_NATIVE_VISUAL_ID),
    X(EGL_NATIVE_VISUAL_TYPE),
    X(EGL_SAMPLES),
    X(EGL_SAMPLE_BUFFERS),
    X(EGL_SURFACE_TYPE),
    X(EGL_TRANSPARENT_TYPE),
    X(EGL_TRANSPARENT_RED_VALUE),
    X(EGL_TRANSPARENT_GREEN_VALUE),
    X(EGL_TRANSPARENT_BLUE_VALUE),
    X(EGL_BIND_TO_TEXTURE_RGB),
    X(EGL_BIND_TO_TEXTURE_RGBA),
    X(EGL_MIN_SWAP_INTERVAL),
    X(EGL_MAX_SWAP_INTERVAL),
    X(EGL_LUMINANCE_SIZE),
    X(EGL_ALPHA_MASK_SIZE),
    X(EGL_COLOR_BUFFER_TYPE),
    X(EGL_RENDERABLE_TYPE),
    X(EGL_CONFORMANT),
   };
#undef X

    for (size_t j = 0; j < sizeof(names) / sizeof(names[0]); j++) {
        EGLint value = -1;
        EGLint returnVal = eglGetConfigAttrib(dpy, config, names[j].attribute, &value);
        EGLint error = eglGetError();
        if (returnVal && error == EGL_SUCCESS) {
            printf(" %s: ", names[j].name);
            printf("%d (0x%x)", value, value);
        }
    }
    printf("\n");
}

int getPixelSize(int color)
{
    switch(color) {
     case IOMX_HAL_PIXEL_FORMAT_RGBA_8888:
         return 4;
     case IOMX_HAL_PIXEL_FORMAT_RGB_565:
         return 2;
     defalut:
        assert(!"Unknow color format."); 
        break;
    }
    return 0;
}

static void sRTCSurfaceCallback (RTCSurface *surface,
                                   graphics_handle *graphics,
                                   void *priv_data)
{

   RTCSurfaceHandle_t *sapi = (RTCSurfaceHandle_t *)(priv_data);
   int w = api->width(graphics);
   int h = api->height(graphics);
   int color = api->pixelFormat(graphics);
   int usage = api->usage(graphics);
   printf("RTCSurface callback...w %d h %d color %d usage %#x\n", w, h, color, usage);

   const char *filename = "rtc_gl_overlay.rgb";
   FILE *save = fopen(filename, "w");
   void *buffer = NULL;
   int write_size = w * h * getPixelSize(color);
   api->lock(graphics, IOMX_GRALLOC_USAGE_SW_READ_RARELY, &buffer);
   fwrite(buffer, write_size, 1, save);
   fclose(save);
   printf("Writed %d bytes in the %s\n",write_size, filename);
   api->unlock(graphics);
   sapi->releaseBuffer(surface, graphics);
   api = NULL;
}

EGLNativeWindowType createRTCSurface(int target_w, int target_h)
{
    RTCSurfaceHandle_t *api = WebRTCAPI::Create()->GetSurfaceAPI();
    if (!targetSurface) {
        assert (api);
        api->create(&targetSurface, target_w, target_h, 
                IOMX_GRALLOC_USAGE_HW_TEXTURE,
                sRTCSurfaceCallback, 20, api);
        assert(targetSurface);
    }
    void *wind = NULL;
    api->window(targetSurface, &wind);
    assert(wind);
    return (EGLNativeWindowType)wind;
}

int main(int argc, char** argv) {
    EGLBoolean returnValue;
    EGLConfig config = {0};
    EGLint numConfigs = 1;

    EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    EGLint s_configAttribs[] = {
#if 0
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL_NONE
#else
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_ALPHA_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
#endif
    };
    EGLint majorVersion;
    EGLint minorVersion;
    EGLContext context;
    EGLSurface surface;
    EGLint w, h;

    EGLDisplay dpy;

    checkEglError("<init>");
    dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    checkEglError("eglGetDisplay");
    if (dpy == EGL_NO_DISPLAY) {
        printf("eglGetDisplay returned EGL_NO_DISPLAY.\n");
        return 0;
    }

    returnValue = eglInitialize(dpy, &majorVersion, &minorVersion);
    checkEglError("eglInitialize", returnValue);
    fprintf(stderr, "EGL version %d.%d\n", majorVersion, minorVersion);
    if (returnValue != EGL_TRUE) {
        printf("eglInitialize failed\n");
        return 0;
    }

    EGLNativeWindowType window = createRTCSurface(1920, 1080);
    EGLint format; 
    eglChooseConfig(dpy, s_configAttribs, &config, 1, &numConfigs);
    eglGetConfigAttrib(dpy, config, EGL_NATIVE_VISUAL_ID, &format);
    //returnValue = EGLUtils::selectConfigForNativeWindow(dpy, s_configAttribs, window, &myConfig);
    //if (returnValue) {
    //    printf("EGLUtils::selectConfigForNativeWindow() returned %d", returnValue);
    //    return 1;
    //}
    //checkEglError("EGLUtils::selectConfigForNativeWindow");

    printf("Chose this configuration:\n");
    printEGLConfiguration(dpy, config);

    surface = eglCreateWindowSurface(dpy, config, window, NULL);
    checkEglError("eglCreateWindowSurface");
    if (surface == EGL_NO_SURFACE) {
        printf("gelCreateWindowSurface failed.\n");
        return 1;
    }

    context = eglCreateContext(dpy, config, EGL_NO_CONTEXT, context_attribs);
    checkEglError("eglCreateContext");
    if (context == EGL_NO_CONTEXT) {
        printf("eglCreateContext failed\n");
        return 1;
    }
    returnValue = eglMakeCurrent(dpy, surface, surface, context);
    checkEglError("eglMakeCurrent", returnValue);
    if (returnValue != EGL_TRUE) {
        return 1;
    }
    eglQuerySurface(dpy, surface, EGL_WIDTH, &w);
    checkEglError("eglQuerySurface");
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &h);
    checkEglError("eglQuerySurface");
    //GLint dim = w < h ? w : h;

    fprintf(stderr, "Window dimensions: %d x %d\n", w, h);

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    if(!setupYuvTexSurface(dpy, context)) {
        fprintf(stderr, "Could not set up texture surface.\n");
        return 1;
    }

    if(!setupGraphics(w, h)) {
        fprintf(stderr, "Could not set up graphics.\n");
        return 1;
    }
    int cc = 1;
    {
        printf("Rending %d\n",cc++);
        struct timeval end,start;
        gettimeofday(&start,NULL);
        renderFrame();
        eglSwapBuffers(dpy, surface);
        checkEglError("eglSwapBuffers");
        gettimeofday(&end,NULL);
        int interval = 0;
        interval = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);
        printf("time= %f(ms)\n", interval/1000.0);
    }
    printf("Stop render.\n");

    while(api);
    RTCSurfaceHandle_t *sapi = WebRTCAPI::Create()->GetSurfaceAPI();
    sapi->destroy(targetSurface);
    return 0;
}

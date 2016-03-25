#ifndef CCSTONE_OPENGL_UTIL_H_
#define CCSTONE_OPENGL_UTIL_H_
#include <error.h>

namespace CCStone {

#define NO_ERROR 0
#define NO_MEMORY -22
#define BAD_VALUE -23
#define NAME_NOT_FOUND -24

#ifndef CC_UNLIKELY
#define CC_UNLIKELY(exper) exper
#endif

#ifndef ALOGD
#define ALOGD(fmt,...)
#endif

#ifndef ALOGE
#define ALOGE(fmt,...)
#endif

#ifndef ALOGI
#define ALOGI(fmt,...)
#endif

#ifndef ALOGW
#define ALOGW(fmt,...)
#endif

#ifndef ALOGE_IF
#define ALOGE_IF(exper, fmt, ...)
#endif

#ifndef LOG_ALWAYS_FATAL
#define LOG_ALWAYS_FATAL
#endif

#ifndef LOG_ALWAYS_FATAL_IF
#define LOG_ALWAYS_FATAL_IF(exper,...)
#endif



#ifdef __cplusplus
    template<bool> struct CompileTimeAssert;
    template<> struct CompileTimeAssert<true> {}; 
#define COMPILE_TIME_ASSERT(_exp) \
    template class CompileTimeAssert< (_exp) >;
#endif
#define COMPILE_TIME_ASSERT_FUNCTION_SCOPE(_exp) \
    CompileTimeAssert<( _exp )>();

    // ---------------------------------------------------------------------------

#ifdef __cplusplus
    template<bool C, typename LSH, typename RHS> struct CompileTimeIfElse;
    template<typename LHS, typename RHS> 
        struct CompileTimeIfElse<true,  LHS, RHS> { typedef LHS TYPE; };
    template<typename LHS, typename RHS> 
        struct CompileTimeIfElse<false, LHS, RHS> { typedef RHS TYPE; };
#endif

    enum {
        /* flip source image horizontally (around the vertical axis) */
        HAL_TRANSFORM_FLIP_H    = 0x01,
        /* flip source image vertically (around the horizontal axis)*/
        HAL_TRANSFORM_FLIP_V    = 0x02,
        /* rotate source image 90 degrees clockwise */
        HAL_TRANSFORM_ROT_90    = 0x04,
        /* rotate source image 180 degrees */
        HAL_TRANSFORM_ROT_180   = 0x03,
        /* rotate source image 270 degrees clockwise */
        HAL_TRANSFORM_ROT_270   = 0x07,
        /* don't use. see system/window.h */
        HAL_TRANSFORM_RESERVED  = 0x08,
    };

template <class T>
class Singleton : public T {
  static T *Instence() {
      return new Singleton;
  } 
};

}
#endif

#ifndef _OMXGRAPHICS_H_
#define _OMXGRAPHICS_H_
#include <stdint.h>
#include <android/rect.h>

typedef struct graphics_ycbcr {
    void *y;
    void *cb;
    void *cr;
    size_t ystride;
    size_t cstride;
    size_t chroma_step;
}graphics_ycbcr_t;


// Point to VideoBuffer
//class VideoBuffer{
//    public:
//        sp<GraphicBuffer> graphicBuffer;
//}
typedef void  graphics_handle;
typedef const void * graphics_native_handle;
typedef int graphics_pixformat;

enum {
    IOMX_HAL_PIXEL_FORMAT_RGBA_8888          = 1,
    IOMX_HAL_PIXEL_FORMAT_RGBX_8888          = 2,
    IOMX_HAL_PIXEL_FORMAT_RGB_888            = 3,
    IOMX_HAL_PIXEL_FORMAT_RGB_565            = 4,
    IOMX_HAL_PIXEL_FORMAT_BGRA_8888          = 5,


    /*
     * Android YUV format:
     *
     * This format is exposed outside of the HAL to software decoders and
     * applications.  EGLImageKHR must support it in conjunction with the
     * OES_EGL_image_external extension.
     *
     * YV12 is a 4:2:0 YCrCb planar format comprised of a WxH Y plane followed
     * by (W/2) x (H/2) Cr and Cb planes.
     *
     * This format assumes
     * - an even width
     * - an even height
     * - a horizontal stride multiple of 16 pixels
     * - a vertical stride equal to the height
     *
     *   y_size = stride * height
     *   c_stride = ALIGN(stride/2, 16)
     *   c_size = c_stride * height/2
     *   size = y_size + c_size * 2
     *   cr_offset = y_size
     *   cb_offset = y_size + c_size
     *
     */
    IOMX_HAL_PIXEL_FORMAT_YV12   = 0x32315659,


    IOMX_HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED = 0x22,

    /*
     * Android flexible YCbCr formats
     *
     * This format allows platforms to use an efficient YCbCr/YCrCb buffer
     * layout, while still describing the buffer layout in a way accessible to
     * the CPU in a device-independent manner.  While called YCbCr, it can be
     * used to describe formats with either chromatic ordering, as well as
     * whole planar or semiplanar layouts.
     *
     * struct android_ycbcr (below) is the the struct used to describe it.
     *
     * This format must be accepted by the gralloc module when
     * USAGE_HW_CAMERA_WRITE and USAGE_SW_READ_* are set.
     *
     * This format is locked for use by gralloc's (*lock_ycbcr) method, and
     * locking with the (*lock) method will return an error.
     */
    IOMX_HAL_PIXEL_FORMAT_YCbCr_420_888 = 0x23,

    /* Legacy formats (deprecated), used by ImageFormat.java */
    IOMX_HAL_PIXEL_FORMAT_YCbCr_422_SP       = 0x10, // NV16
    IOMX_HAL_PIXEL_FORMAT_YCrCb_420_SP       = 0x11, // NV21
    IOMX_HAL_PIXEL_FORMAT_YCbCr_422_I        = 0x14,  // YUY2

    // Qcom platform for encode mode
    IOMX_QCOM_HAL_PIXEL_FORMAT_NV12_ENCODEABLE = 0x102 //NV12
};

enum {
    /* buffer is never read in software */
    IOMX_GRALLOC_USAGE_SW_READ_NEVER         = 0x00000000,
    /* buffer is rarely read in software */
    IOMX_GRALLOC_USAGE_SW_READ_RARELY        = 0x00000002,
    /* buffer is often read in software */
    IOMX_GRALLOC_USAGE_SW_READ_OFTEN         = 0x00000003,
    /* mask for the software read values */
    IOMX_GRALLOC_USAGE_SW_READ_MASK          = 0x0000000F,

    /* buffer is never written in software */
    IOMX_GRALLOC_USAGE_SW_WRITE_NEVER        = 0x00000000,
    /* buffer is rarely written in software */
    IOMX_GRALLOC_USAGE_SW_WRITE_RARELY       = 0x00000020,
    /* buffer is often written in software */
    IOMX_GRALLOC_USAGE_SW_WRITE_OFTEN        = 0x00000030,
    /* mask for the software write values */
    IOMX_GRALLOC_USAGE_SW_WRITE_MASK         = 0x000000F0,

    /* buffer will be used as an OpenGL ES texture */
    IOMX_GRALLOC_USAGE_HW_TEXTURE            = 0x00000100,
    /* buffer will be used as an OpenGL ES render target */
    IOMX_GRALLOC_USAGE_HW_RENDER             = 0x00000200,
    /* buffer will be used by the 2D hardware blitter */
    IOMX_GRALLOC_USAGE_HW_2D                 = 0x00000400,
    /* buffer will be used by the HWComposer HAL module */
    IOMX_GRALLOC_USAGE_HW_COMPOSER           = 0x00000800,
    /* buffer will be used with the framebuffer device */
    IOMX_GRALLOC_USAGE_HW_FB                 = 0x00001000,
    /* buffer will be used with the HW video encoder */
    IOMX_GRALLOC_USAGE_HW_VIDEO_ENCODER      = 0x00010000,
    /* buffer will be written by the HW camera pipeline */
    IOMX_GRALLOC_USAGE_HW_CAMERA_WRITE       = 0x00020000,
    /* buffer will be read by the HW camera pipeline */
    IOMX_GRALLOC_USAGE_HW_CAMERA_READ        = 0x00040000,
    /* buffer will be used as part of zero-shutter-lag queue */
    IOMX_GRALLOC_USAGE_HW_CAMERA_ZSL         = 0x00060000,
    /* mask for the camera access values */
    IOMX_GRALLOC_USAGE_HW_CAMERA_MASK        = 0x00060000,
    /* mask for the software usage bit-mask */
    IOMX_GRALLOC_USAGE_HW_MASK               = 0x00071F00,

    /* buffer will be used as a RenderScript Allocation */
    IOMX_GRALLOC_RENDERSCRIPT          = 0x00100000,

    /* buffer should be displayed full-screen on an external display when
     *      * possible
     *           */
    IOMX_GRALLOC_EXTERNAL_DISP         = 0x00002000,

    /* Must have a hardware-protected path to external display sink for
     *      * this buffer.  If a hardware-protected path is not available, then
     *           * either don't composite only this buffer (preferred) to the
     *                * external sink, or (less desirable) do not route the entire
     *                     * composition to the external sink.
     *                          */
    IOMX_GRALLOC_PROTECTED             = 0x00004000,

    /* implementation-specific private usage flags */
    IOMX_GRALLOC_PRIVATE_0             = 0x10000000,
    IOMX_GRALLOC_PRIVATE_1             = 0x20000000,
    IOMX_GRALLOC_PRIVATE_2             = 0x40000000,
    IOMX_GRALLOC_PRIVATE_3             = 0x80000000,
    IOMX_GRALLOC_PRIVATE_MASK          = 0xF0000000,
};

#endif

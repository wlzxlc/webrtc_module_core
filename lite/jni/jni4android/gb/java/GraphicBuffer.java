package com.snail.gb;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.os.Build;

import java.io.File;
import java.nio.ByteBuffer;

import dalvik.system.BaseDexClassLoader;

/**
 * Created by lichao on 17-7-5.
 */

public class GraphicBuffer {
    private static final String TAG = GraphicBuffer.class.getName();
    private long mNativeObject;
    private Context mCtx;
    private int mColor;
    private int mWidth;
    private int mHeight;
    private int mTexId;
    private int mFrameBufferId;
    private int mInstallSdk;
    private Canvas mCanvas;
    static {
        System.loadLibrary("snail_graphic");
    }

    private int loadwithsdk(){
        synchronized (GraphicBuffer.class) {
            int sdk = 0;
            BaseDexClassLoader baseDexClassLoader = (BaseDexClassLoader)mCtx.getClassLoader();
            File file = new File(baseDexClassLoader.findLibrary("snail_graphic"));
            String path = file.getParent();
            Log.d(TAG, "Install shared library path [" + path + "] ...");
            // Using dlopen when system sdk less than 19
            if (Build.VERSION.SDK_INT < 19) {
                sdk = Build.VERSION.SDK_INT;

                // GraphicBuffer is not supports if the Application configure the target sdk greater than 24 and System SDK less than 19
                if (mCtx.getApplicationInfo().targetSdkVersion >= 24) {
                    Log.w(TAG, "GraphicBuffer is not supports on devices " + Build.MODEL);
                    return -1;
                }
            }
            mInstallSdk = sdk;
            return _load(sdk, path);
        }
    }

    public byte[] lockAndCopy() {
        return _lockBuffer();
    }

    public int unlock() {
        return _unlock();
    }

    public int getSDK() {
        return mInstallSdk;
    }

    private GraphicBuffer() {
    }

    private GraphicBuffer(Context ctx, int w, int h, int color) {
        mWidth = w;
        mHeight = h;
        mColor = color;
        mCtx = ctx;
    }

    public static GraphicBuffer create(Context ctx, int w, int h, int color) {
        GraphicBuffer buffer = new GraphicBuffer(ctx, w, h, color);
        int rev = buffer.loadwithsdk();
        if (rev < 0) {
            buffer = null;
        }
        return buffer;
    }

    public int createTexture(int framebuffer) {
        int tex = _createFrameBufferAndBind(mWidth, mHeight, mColor, framebuffer);
        if (tex > 0) {
            mTexId = tex;
            mFrameBufferId = framebuffer;
        }
        return mTexId;
    }

    public int textureId() {
        return mTexId;
    }

    public int framebufferId() {
        return mFrameBufferId;
    }

    public int stride() {
        return _stride();
    }

    public int width() {
        return mWidth;
    }

    public int height() {
        return mHeight;
    }

    public long lock() {
        return _lock();
    }

    public void destroy() {
        if (mTexId > 0) {
            _destroyFrameBuffer();
            mTexId = -1;
            mFrameBufferId = -1;
            Log.d(TAG, "destroy GraphicBufer");
        }
    }

    @Override
    protected void finalize() throws Throwable {
        super.finalize();
        destroy();
    }

    private native int _load(int sdk, String path);
    private native int _createFrameBufferAndBind(int w, int h, int color, int fb);
    private native int _destroyFrameBuffer();
    private native byte[] _lockBuffer();
    private native long _lock();
    private native int _unlock();
    private native int _stride();
}

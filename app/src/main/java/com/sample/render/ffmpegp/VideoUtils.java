package com.sample.render.ffmpegp;

import android.view.Surface;

/**
 * Description :
 * <p>
 * Created : TIAN FENG
 * Date : 2019/3/20
 * Email : 27674569@qq.com
 * Version : 1.0
 */
public class VideoUtils {

    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("postproc-53");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("ffmpegp_render");
        System.loadLibrary("yuv");
    }


    public static native void render(String path, Surface suface);
}

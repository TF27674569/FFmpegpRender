#include <jni.h>
#include <string>
#include <android/log.h>
#include <stdio.h>
#include <unistd.h>

// native 绘制需要的头文件
#include <android/native_window_jni.h>
#include <android/native_window.h>

#define TAG "FFmpegp"

#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO,TAG,FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR,TAG,FORMAT,##__VA_ARGS__);

extern "C"{

//封装视频格式
#include "libavformat/avformat.h"
// 解码库
#include "libavcodec/avcodec.h"
// 缩放
#include "libswscale/swscale.h"

#include "libyuv.h"

}

using namespace libyuv;



extern "C"
JNIEXPORT void JNICALL
Java_com_sample_render_ffmpegp_VideoUtils_render(JNIEnv *env, jclass type, jstring path_,
                                                 jobject suface) {
    const char *path = env->GetStringUTFChars(path_, 0);
    /**
     * 思路基本与解码过程一致
     * 循环帧的时候不需要保存直接转为rgba后操作缓冲区即可
     *
     * 思路：
     *  1 注册组建 av_register_all
     *  2 打开输入视频文件 avformat_open_input
     *  3 获取视频信息 avformat_find_stream_info
     *  4 获取视频解码器 avcodec_find_decoder (在此之前需要先获取到视频流信息)
     *  5 打开视频解码器 avcodec_open2
     *  6 一帧一帧读取压缩视频数据AVPacket 一帧一帧的视频包
     *   6.1 压缩视频包转YUV420P的包
     *   6.2 native绘制 lock fix unlock
     *  7 关闭释放资源
     */


    // 1 注册组建
    av_register_all();


    // 2 打开输入视频文件
    //  需要参数 1 AVFormatContext 封装格式上下文
    //          2 const char * 路径
    //          3 AVInputFormat 指定输入的封装格式。一般传NULL，由FFmpeg自行探测
    //          4 AVDictionary 其它参数设置。它是一个字典，用于参数传递，不传则写NULL。
    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if((avformat_open_input(&pFormatCtx,path,NULL,NULL))!=0){
        LOGE("打开文件失败  %s \n",path);
        return;
    }

    // 3 获取视频信息
    if (avformat_find_stream_info(pFormatCtx,NULL)<0){
        LOGE("获取视频信息失败\n");
        return;
    }

    // 4 获取视频解码器 for 循环找到视频流
    int videoStreamId = -1;

    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        // 判断流的类型是否是视频流
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStreamId = i;
            goto CHECK;
        }
    }

    CHECK:
    // 判断是否获取到了视频流
    if (videoStreamId == -1){
        LOGE("没有获取到视频流\n");
        return;
    }


    // 视频编码器上下文 包含了众多编解码器需要的参数信息
    AVCodecContext *pCodecCtx = pFormatCtx->streams[videoStreamId]->codec;

    // 拿到流获取解码器
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);


    if (pCodec == NULL){
        LOGE("无法解码，视频是否加密\n");
        return;
    }


    // 5 打开解码器
    if (avcodec_open2(pCodecCtx,pCodec,NULL)<0){
        LOGE("解码器无法打开\n");
        return;
    }

    // 6 一帧一帧读取压缩视频数据AVPacket 一帧一帧的视频包
    // 需要一个 avpacket
    AVPacket *avPacket = (AVPacket*)(av_malloc(sizeof(AVPacket)));

    AVFrame *yuvFrame = av_frame_alloc();
    // yuv为输出文件frame
    AVFrame *rgbaFrame = av_frame_alloc();

    // 初始化native窗口
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,suface);
    ANativeWindow_Buffer outBuffer;

    // 用于判断是否结束
    int got_picture_ptr = -1,frameCount = 0;



    while (av_read_frame(pFormatCtx,avPacket)>=0){

        // 只解码视频文件
        if (avPacket->stream_index == videoStreamId){
            // AVPacket - > AVFrame
            // AVCodecContext 编码器上下文
            // AVFrame 编码后一帧的数据
            // int *got_picture_ptr 为0 表示没有数据了
            // AVPacket  读取到的数据包
            avcodec_decode_video2(pCodecCtx,yuvFrame,&got_picture_ptr,avPacket);

            // 不为0 正在解码
            if (got_picture_ptr){
                // 锁住屏幕
                // ANativeWindow* window  native窗口
                // ANativeWindow_Buffer* outBuffer 缓冲区
                // ARect* inOutDirtyBounds 矩阵 NULL
                // 需要先设置缓冲区的属性
                ANativeWindow_setBuffersGeometry(nativeWindow,pCodecCtx->width, pCodecCtx->height,WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_lock(nativeWindow,&outBuffer,NULL);

                // 设置缓冲区为outBuffer数据
                avpicture_fill((AVPicture *)(rgbaFrame), (uint8_t* )outBuffer.bits, AV_PIX_FMT_ARGB, pCodecCtx->width, pCodecCtx->height);


                // 将yuv数据转为argb8888 与sufaceview绘制的一致 这里使用libyuv库转换
                I420ToARGB(yuvFrame->data[0],yuvFrame->linesize[0],// Y
                           yuvFrame->data[2],yuvFrame->linesize[2],// V
                           yuvFrame->data[1],yuvFrame->linesize[1],// U
                           rgbaFrame->data[0],rgbaFrame->linesize[0],// OUT  rgba 只有一块 没有与yuv一样4 1 1 区分
                           pCodecCtx->width,pCodecCtx->height// w h
                );


                // 解锁屏幕
                ANativeWindow_unlockAndPost(nativeWindow);

                // 防止绘制过快 16每帧
                usleep(1000 * 16);

                LOGI("解码第%d帧",frameCount++);
            }

        }

        av_free_packet(avPacket);
    }

    ANativeWindow_release(nativeWindow);


    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);


    env->ReleaseStringUTFChars(path_, path);
}
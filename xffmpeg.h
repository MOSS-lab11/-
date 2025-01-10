#pragma once
#include<string>
#include<qmutex.h>
extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>//解码器
#include <libavcodec/avcodec.h>
#include<libswresample/swresample.h>
}
class xffmpeg {
protected:
    
xffmpeg();
xffmpeg(const xffmpeg&) = delete;
xffmpeg& operator=(const xffmpeg&) = delete;
SwsContext* cCtx = NULL;
SwrContext* aCtx = NULL;

protected:
    char errorbuf[1024];
    
    
    



public:
    QMutex mutex;
    int totalSec=0;  //总时长
    int pts = 0;  //当前播放时间 ms//只有音频才处理pts
    AVFormatContext* ic = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVCodecContext* acodec_ctx = NULL;
    int videostream = 0;
    int audiostream = 44444;    
    //******** int audiostream = 1;   导致两个流编号一样，废了
    int fps = 0;
    AVFrame* yuv = NULL;
    AVFrame* pcm = NULL;
    const AVCodec* codec = NULL;
    const AVCodec* acodec = NULL;
    bool isplay = false;
    int sampleRate = 48000;
    int sampleSize = 16;
    int channel = 2;
    // 获取单例实例的静态方法
    static xffmpeg* Get() {
        static xffmpeg ff;
        return &ff;
    }
    std::string Geterror();
    bool Open(const char *path);
    void Close();
    ///用户需要自己清理
    AVPacket Read();

   int Getpts(const AVPacket *pkt);
  int decode(const AVPacket *pkt);
  int ToPCM(char* out);
    bool ToRGB(char* out, int outwidth, int outheight); 
        //pos为百分比0~1
    bool seek(float pos);

    virtual ~xffmpeg();
};


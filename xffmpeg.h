#pragma once
#include<string>
#include<qmutex.h>
extern "C"
{
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>//������
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
    int totalSec=0;  //��ʱ��
    int pts = 0;  //��ǰ����ʱ�� ms//ֻ����Ƶ�Ŵ���pts
    AVFormatContext* ic = NULL;
    AVCodecContext* codec_ctx = NULL;
    AVCodecContext* acodec_ctx = NULL;
    int videostream = 0;
    int audiostream = 44444;    
    //******** int audiostream = 1;   �������������һ��������
    int fps = 0;
    AVFrame* yuv = NULL;
    AVFrame* pcm = NULL;
    const AVCodec* codec = NULL;
    const AVCodec* acodec = NULL;
    bool isplay = false;
    int sampleRate = 48000;
    int sampleSize = 16;
    int channel = 2;
    // ��ȡ����ʵ���ľ�̬����
    static xffmpeg* Get() {
        static xffmpeg ff;
        return &ff;
    }
    std::string Geterror();
    bool Open(const char *path);
    void Close();
    ///�û���Ҫ�Լ�����
    AVPacket Read();

   int Getpts(const AVPacket *pkt);
  int decode(const AVPacket *pkt);
  int ToPCM(char* out);
    bool ToRGB(char* out, int outwidth, int outheight); 
        //posΪ�ٷֱ�0~1
    bool seek(float pos);

    virtual ~xffmpeg();
};


#include "xffmpeg.h"
#include "videoWidget.h"
#include <QPainter>
#include <iostream>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

static double r2d(AVRational r) {//avrational��ʾ������
    return r.num == 0 || r.den == 0 ? 0 : (double)r.num / (double)r.den;
}
std::string xffmpeg::Geterror() {
	mutex.lock();
	std::string str = this->errorbuf;
	mutex.unlock();
	return str;

}


bool xffmpeg::Open(const char* path) {
    Close();
    mutex.lock();

    int res = avformat_open_input(&ic, path, NULL, NULL);
    if (res != 0) { // ��ʧ��
        av_strerror(res, errorbuf, sizeof(errorbuf)); // �Ὣ���صĴ���str������buf��
        printf("open %s failed:%s\n", path, errorbuf);
        mutex.unlock();
        return false;
    }

    res = avformat_find_stream_info(ic, NULL); // info����Ϣ����˼
    if (res < 0) {
        printf("Could not find stream information\n");
        mutex.unlock();
        return false;
    }
    
     totalSec = (ic->duration / AV_TIME_BASE) * 1000;
     av_dump_format(ic, 0, path, 0);
    for (unsigned int i = 0; i < ic->nb_streams; i++) {
        AVStream* stream = ic->streams[i];
        //videostream = av_find_best_stream(ic,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//��Ƶ��
            videostream = i;
            // �򿪱�����
            std::cout << i << "��Ƶ��Ϣ" << std::endl;
            std::cout << "������" << av_get_media_type_string(stream->codecpar->codec_type) << std::endl;
        
            fps = r2d(ic->streams[videostream]->avg_frame_rate);

             codec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (codec == NULL) {
                printf("video codec not found!\n");
                mutex.unlock();
                return false;
            }

            // �������������
          
            codec_ctx = avcodec_alloc_context3(codec);
            if (!codec_ctx) {
                printf("Failed to allocate codec context\n");
                mutex.unlock();
                return false;
            }

            // ������������������������
            if (avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0) {
                printf("Failed to copy codec parameters to context\n");
                avcodec_free_context(&codec_ctx);
                mutex.unlock();
                return false;
            }
            codec_ctx->thread_count = 2;  //�������߳̽���

            // �򿪽�����
            int err = avcodec_open2(codec_ctx, codec, NULL);
            if (err != 0) {
                char buf[1024];
                av_strerror(err, buf, sizeof(buf));
                printf("Failed to open codec: %s\n", buf);
                avcodec_free_context(&codec_ctx);
                mutex.unlock();
                return false;
            }
            printf("open video codec success!\n");


        }//��Ƶ
      else if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) 
        {
           audiostream = i;
         acodec = avcodec_find_decoder(stream->codecpar->codec_id);
         acodec_ctx = avcodec_alloc_context3(acodec);
         if (!acodec_ctx) {
             printf("Failed to allocate codec context\n");
             mutex.unlock();
             return false;
            }

         // ������������������������
         if (avcodec_parameters_to_context(acodec_ctx, stream->codecpar) < 0) {
             printf("Failed to copy codec parameters to context\n");
             avcodec_free_context(&acodec_ctx);
             mutex.unlock();
             return false;
            }
        int re=avcodec_open2(acodec_ctx, acodec, NULL);
        if (re < 0) {
            mutex.unlock();
            printf("��Ƶ��������ʧ�ܣ�");
            return false;
            } 
  
        this->sampleRate = acodec_ctx->sample_rate;
        this->channel = acodec_ctx->ch_layout.nb_channels;
        switch (acodec_ctx->sample_fmt)
            {
        case AV_SAMPLE_FMT_S16:
        this->sampleSize = 16;
        break;
        case AV_SAMPLE_FMT_S32:
            this->sampleSize = 32;
            break;
        default:
            break;
         }
        printf("audio samplerate:%d\n size:%d\n  channel:%d\n", this->sampleRate, this->sampleSize, this->channel);
        }
   


    }

    mutex.unlock();
    return true;
}





void xffmpeg::Close() {
	mutex.lock();
	if (ic != NULL) {
		avformat_close_input(&ic);
	}
    if (yuv)av_frame_free(&yuv);
    if (cCtx) {
        sws_freeContext(cCtx);
        cCtx = NULL;
    }
    if (aCtx) {
        swr_free(&aCtx);
    }

	mutex.unlock();

}



AVPacket xffmpeg::Read()
{

	AVPacket pkt;
	memset(&pkt, 0, sizeof(AVPacket));
	mutex.lock();
	if (!ic) {
		mutex.unlock();
		return pkt;
	}
	int err = av_read_frame(ic, &pkt);
	if (err != 0) {
		av_strerror(err,errorbuf,sizeof(errorbuf));
	}
	mutex.unlock();
	return pkt;
}

int xffmpeg::Getpts(const AVPacket* pkt)
{
    mutex.lock();
    if (!ic) {
        mutex.unlock();
        return -1;
    }
    int64_t p = (pkt->pts * r2d(ic->streams[pkt->stream_index]->time_base)) * 1000;
    mutex.unlock();
    return p;
}



int xffmpeg::decode(const AVPacket* pkt) {//���뺯��
    mutex.lock();
    if (yuv != NULL)av_frame_free(&yuv);
    if (pcm != NULL)av_frame_free(&pcm);
    if (!ic) {
        mutex.unlock();
        return NULL;
    }

    if (yuv == NULL) {
        yuv = av_frame_alloc();
        if (!yuv) {
            printf("Could not allocate frame\n");
            mutex.unlock();
            return 0;
        }
    }
    if (pcm == NULL) {
       pcm = av_frame_alloc();
        if (!pcm) {
            printf("Could not audio frame\n");
            mutex.unlock();
            return 0;
        }
    }
    AVFrame* frame = yuv;
    if (pkt->stream_index == audiostream) {
      
        frame = pcm;
        int re = avcodec_send_packet(acodec_ctx, pkt); //������Ƶ�ģ�������     
            std::cout << "��packetΪ��Ƶ" << std::endl;
        if (re != 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(re, errbuf, sizeof(errbuf));
            printf("Error sending a packet for decoding: %s\n", errbuf);
            mutex.unlock();
            return NULL;
        }
     //   av_packet_unref((AVPacket*)pkt);
        //��ռ��cpu��ֻ�Ǵ��߳��л�ȡ����ӿ�,һ��send���ܶ�Ӧ���receive

        re = avcodec_receive_frame(acodec_ctx, frame);
        if (re != 0) {
            if (re != AVERROR(EAGAIN) && re != AVERROR_EOF) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(re, errbuf, sizeof(errbuf));
                printf("Error during decoding: %s\n", errbuf);
            }
 
            mutex.unlock();
            return NULL;
        }
    }
    //����packet�������߳�,send��null���ε���receiveȡ�����л���֡
    else if (pkt->stream_index == videostream) {
    //    
        int re = avcodec_send_packet(codec_ctx, pkt);
        std::cout << "��packetΪ��Ƶ" << std::endl;
        if (re != 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(re, errbuf, sizeof(errbuf));
            printf("Error sending a packet for decoding: %s\n", errbuf);
            mutex.unlock();
            return NULL;
        }
       // av_packet_unref((AVPacket*)pkt);
        //��ռ��cpu��ֻ�Ǵ��߳��л�ȡ����ӿ�,һ��send���ܶ�Ӧ���receive

        re = avcodec_receive_frame(codec_ctx, frame);
        if (re != 0) {
            if (re != AVERROR(EAGAIN) && re != AVERROR_EOF) {
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_strerror(re, errbuf, sizeof(errbuf));
                printf("Error during decoding: %s\n", errbuf);
            }

            mutex.unlock();
            return NULL;
        }
    }
    mutex.unlock();
    int64_t p = (frame->pts * r2d(ic->streams[pkt->stream_index]->time_base)) * 1000;
    //printf("c==%d pkt->stream_index==%d\n ", audiostream, pkt->stream_index);
    if(audiostream==pkt->stream_index)
    {
       
        xffmpeg::Get()->pts = p;//ֻ��¼��Ƶ
    }
    av_packet_unref((AVPacket*)pkt);//��Ҫpkt�����Ȳ�Ҫ��ǰ��ʹ���������������������������������������Ѫ�Ľ�ѵ��
    return p;
}

int xffmpeg::ToPCM(char* out)
{
    mutex.lock();
    if (!ic || !pcm || !out) {
        mutex.unlock();
        return 0;
    }

    //�����任

    AVCodecContext* ctx = acodec_ctx;
    if (aCtx == NULL) {
        aCtx = swr_alloc();
    }
    swr_alloc_set_opts2(&aCtx,&ctx->ch_layout,
        AV_SAMPLE_FMT_S16,
        ctx->sample_rate,&ctx->ch_layout,
        ctx->sample_fmt,
        ctx->sample_rate,
        0,0);
    swr_init(aCtx);
    uint8_t* data[1] = { NULL };
    data[0] = (uint8_t*)out;
    int len = swr_convert(aCtx, data, 10000,
        pcm->data, pcm->nb_samples);//����д�������������������������������󣬷��ظ�ֵ��
    if (len <= 0) {
        mutex.unlock();
        return 0;
    }
    int outsize = av_samples_get_buffer_size(NULL,ctx->ch_layout.nb_channels,
        pcm->nb_samples,AV_SAMPLE_FMT_S16,0);//�õ���������Ҫ�����Ǽ���洢ָ�������͸�ʽ����Ƶ��������
    //�Ļ�������С��������˵�����������������ÿ����������������������ʽ�Ͷ�����������������Ļ�������С
    mutex.unlock();
    return outsize;
}

 bool xffmpeg::ToRGB(char* out, int outwidth, int outheight) {
     mutex.lock();
     if (!ic || !yuv) {
         mutex.unlock();
         return false;
     }
     // ���֡����
     if (yuv->flags & AV_FRAME_FLAG_KEY) {
         printf("Processing ssssssssssssssssssssssssssskey frame (I frame)\n");
     }
     else {
         printf("Processing non-key wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwframe (P or B frame)\n");
     }
     AVCodecContext* videoctx = codec_ctx;

     // �������ȡת��������
     cCtx = sws_getCachedContext(cCtx,
         codec_ctx->width,
         codec_ctx->height,
         codec_ctx->pix_fmt,
         outwidth, outheight,
         AV_PIX_FMT_BGRA, // Ŀ�����ظ�ʽ
         SWS_BICUBIC,     // ѡ���㷨
         NULL, NULL, NULL);

     if (!cCtx) {
         printf("Failed to get sws context\n");
         mutex.unlock();
         return false; // ������
     }
     printf("sws_get success!\n");

     uint8_t* data[AV_NUM_DATA_POINTERS] = { 0 };
     data[0] = (uint8_t*)out;
     int linesize[AV_NUM_DATA_POINTERS] = { 0 };
     linesize[0] = outwidth * 4; // BGRA ��ʽ
     // ȷ�� yuv->data �� yuv->linesize ��Ч
     if (!yuv->data || !yuv->data[0]) {
         printf("Invalid YUV data\n");
         mutex.unlock();
         return false;
     }
     
     int h = sws_scale(cCtx, yuv->data, yuv->linesize, 0, videoctx->height, data, linesize);
     if (h < 0) {
         printf("Error during scaling\n");
         mutex.unlock();
         return false; // ������
     }

     printf("fps==(%d)\n", fps);
     mutex.unlock();
     return true;
 }





xffmpeg::xffmpeg() {
	errorbuf[0] = '\0';
	avdevice_register_all();
}


bool xffmpeg::seek(float pos)
{
    mutex.lock();
    if (ic==NULL) {
        mutex.unlock();
        return false;
    }
    int64_t stamp = 0;
    stamp = pos * ic->streams[videostream]->duration;
    
    int re = av_seek_frame(ic, videostream, stamp, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    avcodec_flush_buffers(codec_ctx);
    pts = (stamp * r2d(ic->streams[videostream]->time_base)) * 1000;
    mutex.unlock();
    if (re >=0) { 
        return true; }
    printf("errrrrrrrrrrrrrrrrrrrrrrrr");
    return false;
}
xffmpeg::~xffmpeg() {

}
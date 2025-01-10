#include "xffmpeg.h"
#include "videoWidget.h"
#include <QPainter>
#include <iostream>

#pragma comment(lib, "avformat.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"swresample.lib")

static double r2d(AVRational r) {//avrational表示有理数
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
    if (res != 0) { // 打开失败
        av_strerror(res, errorbuf, sizeof(errorbuf)); // 会将返回的错误str储存在buf中
        printf("open %s failed:%s\n", path, errorbuf);
        mutex.unlock();
        return false;
    }

    res = avformat_find_stream_info(ic, NULL); // info：信息的意思
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
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {//视频流
            videostream = i;
            // 打开编码器
            std::cout << i << "视频信息" << std::endl;
            std::cout << "流类型" << av_get_media_type_string(stream->codecpar->codec_type) << std::endl;
        
            fps = r2d(ic->streams[videostream]->avg_frame_rate);

             codec = avcodec_find_decoder(stream->codecpar->codec_id);
            if (codec == NULL) {
                printf("video codec not found!\n");
                mutex.unlock();
                return false;
            }

            // 分配解码器上下
          
            codec_ctx = avcodec_alloc_context3(codec);
            if (!codec_ctx) {
                printf("Failed to allocate codec context\n");
                mutex.unlock();
                return false;
            }

            // 复制流参数到解码器上下文
            if (avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0) {
                printf("Failed to copy codec parameters to context\n");
                avcodec_free_context(&codec_ctx);
                mutex.unlock();
                return false;
            }
            codec_ctx->thread_count = 2;  //用两个线程解码

            // 打开解码器
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


        }//音频
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

         // 复制流参数到解码器上下文
         if (avcodec_parameters_to_context(acodec_ctx, stream->codecpar) < 0) {
             printf("Failed to copy codec parameters to context\n");
             avcodec_free_context(&acodec_ctx);
             mutex.unlock();
             return false;
            }
        int re=avcodec_open2(acodec_ctx, acodec, NULL);
        if (re < 0) {
            mutex.unlock();
            printf("音频解码器打开失败！");
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



int xffmpeg::decode(const AVPacket* pkt) {//解码函数
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
        int re = avcodec_send_packet(acodec_ctx, pkt); //这是音频的！！！！     
            std::cout << "此packet为音频" << std::endl;
        if (re != 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(re, errbuf, sizeof(errbuf));
            printf("Error sending a packet for decoding: %s\n", errbuf);
            mutex.unlock();
            return NULL;
        }
     //   av_packet_unref((AVPacket*)pkt);
        //不占用cpu，只是从线程中获取解码接口,一次send可能对应多次receive

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
    //发送packet到解码线程,send传null后多次调用receive取出所有缓冲帧
    else if (pkt->stream_index == videostream) {
    //    
        int re = avcodec_send_packet(codec_ctx, pkt);
        std::cout << "此packet为视频" << std::endl;
        if (re != 0) {
            char errbuf[AV_ERROR_MAX_STRING_SIZE];
            av_strerror(re, errbuf, sizeof(errbuf));
            printf("Error sending a packet for decoding: %s\n", errbuf);
            mutex.unlock();
            return NULL;
        }
       // av_packet_unref((AVPacket*)pkt);
        //不占用cpu，只是从线程中获取解码接口,一次send可能对应多次receive

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
       
        xffmpeg::Get()->pts = p;//只记录音频
    }
    av_packet_unref((AVPacket*)pkt);//你要pkt，就先不要在前面使用这个！！！！！！！！！！！！！！！！（血的教训）
    return p;
}

int xffmpeg::ToPCM(char* out)
{
    mutex.lock();
    if (!ic || !pcm || !out) {
        mutex.unlock();
        return 0;
    }

    //描述变换

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
        pcm->data, pcm->nb_samples);//返回写入输出缓冲区的样本数。如果发生错误，返回负值。
    if (len <= 0) {
        mutex.unlock();
        return 0;
    }
    int outsize = av_samples_get_buffer_size(NULL,ctx->ch_layout.nb_channels,
        pcm->nb_samples,AV_SAMPLE_FMT_S16,0);//得到函数的主要功能是计算存储指定数量和格式的音频样本所需
    //的缓冲区大小。具体来说，它会根据声道数、每个声道的样本数、采样格式和对齐参数，计算出所需的缓冲区大小
    mutex.unlock();
    return outsize;
}

 bool xffmpeg::ToRGB(char* out, int outwidth, int outheight) {
     mutex.lock();
     if (!ic || !yuv) {
         mutex.unlock();
         return false;
     }
     // 检查帧类型
     if (yuv->flags & AV_FRAME_FLAG_KEY) {
         printf("Processing ssssssssssssssssssssssssssskey frame (I frame)\n");
     }
     else {
         printf("Processing non-key wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwframe (P or B frame)\n");
     }
     AVCodecContext* videoctx = codec_ctx;

     // 创建或获取转换上下文
     cCtx = sws_getCachedContext(cCtx,
         codec_ctx->width,
         codec_ctx->height,
         codec_ctx->pix_fmt,
         outwidth, outheight,
         AV_PIX_FMT_BGRA, // 目标像素格式
         SWS_BICUBIC,     // 选择算法
         NULL, NULL, NULL);

     if (!cCtx) {
         printf("Failed to get sws context\n");
         mutex.unlock();
         return false; // 错误处理
     }
     printf("sws_get success!\n");

     uint8_t* data[AV_NUM_DATA_POINTERS] = { 0 };
     data[0] = (uint8_t*)out;
     int linesize[AV_NUM_DATA_POINTERS] = { 0 };
     linesize[0] = outwidth * 4; // BGRA 格式
     // 确保 yuv->data 和 yuv->linesize 有效
     if (!yuv->data || !yuv->data[0]) {
         printf("Invalid YUV data\n");
         mutex.unlock();
         return false;
     }
     
     int h = sws_scale(cCtx, yuv->data, yuv->linesize, 0, videoctx->height, data, linesize);
     if (h < 0) {
         printf("Error during scaling\n");
         mutex.unlock();
         return false; // 错误处理
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
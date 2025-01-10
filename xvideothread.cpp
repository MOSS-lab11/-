#include "xvideothread.h"
#include"xffmpeg.h"
#include <Qthread>
#include "QtWidgetsApplication2.h"
#include "xAudioplay.h"
#include<list>
static bool isexit = false;
static int apts = -1; 
static std::list<AVPacket>videos;//储存的视频帧
void xvideothread::task_decode() 
{
	char out[10000] = { 0 };//栈上分配
	while (!isexit) {
		if (xffmpeg::Get()->isplay) {
			QThread::msleep(10);
			continue;
		}
		while (videos.size() > 0) {
			AVPacket pack= videos.front();
		int pts=xffmpeg::Get()->Getpts(&pack);
		if (pts > apts) {
			break;
		}
		xffmpeg::Get()->decode(&pack);
		av_packet_unref(&pack);
		videos.pop_front();
		}

		int free = xAudioplay::Get()->Getfree();
		printf("buffer size==%d", free);
		if (free < 10000) {
			QThread::msleep(1);
			continue;
		}
		AVPacket pkt = xffmpeg::Get()->Read();
		if (pkt.size <= 0) {
			QThread::sleep(10);
			continue;
		}
		if (pkt.stream_index == xffmpeg::Get()->audiostream) {
			apts= xffmpeg::Get()->decode(&pkt);
		
			av_packet_unref(&pkt);
			int len = xffmpeg::Get()->ToPCM(out);
			xAudioplay::Get()->Write(out, len);
			continue;
		}

		else if (pkt.stream_index == xffmpeg::Get()->videostream) {
			/*xffmpeg::Get()->decode(&pkt);
			av_packet_unref(&pkt);*/

			videos.push_back(pkt);
			//if (xffmpeg::Get()->fps > 0)
			//	QThread::msleep(1000 / (xffmpeg::Get()->fps));//这段代码的主要目的是为了在视频处理或播放过程中，使线程按照指定的帧率进行处理，

			////从而控制视频的播放速度。通过使线程在每帧之间暂停适当的时间，可以模拟出流畅的视频播放效果
		}
	}
}

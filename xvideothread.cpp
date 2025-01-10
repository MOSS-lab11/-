#include "xvideothread.h"
#include"xffmpeg.h"
#include <Qthread>
#include "QtWidgetsApplication2.h"
#include "xAudioplay.h"
#include<list>
static bool isexit = false;
static int apts = -1; 
static std::list<AVPacket>videos;//�������Ƶ֡
void xvideothread::task_decode() 
{
	char out[10000] = { 0 };//ջ�Ϸ���
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
			//	QThread::msleep(1000 / (xffmpeg::Get()->fps));//��δ������ҪĿ����Ϊ������Ƶ����򲥷Ź����У�ʹ�̰߳���ָ����֡�ʽ��д���

			////�Ӷ�������Ƶ�Ĳ����ٶȡ�ͨ��ʹ�߳���ÿ֮֡����ͣ�ʵ���ʱ�䣬����ģ�����������Ƶ����Ч��
		}
	}
}

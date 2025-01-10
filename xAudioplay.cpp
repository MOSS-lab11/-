#include "xAudioplay.h"
#include <QAudioFormat>
#include <QMediaDevices>
#include <QIODevice>
#include <QAudioSink>
#include<QThread>
#include <QMutex>
class cxAudioPlay :public xAudioplay {
public:
	QMutex mutex;

	QAudioDevice audioDevice = QMediaDevices::defaultAudioOutput();
	QAudioFormat fmt;
	QAudioSink* audio = NULL;
	QIODevice* audioIO = NULL;

	bool Start() {
		Stop();
		mutex.lock();

		fmt.setSampleRate(this->sampleRate); // ������
		fmt.setChannelCount(this->channel);    // ������
		fmt.setSampleFormat(QAudioFormat::Int16); // λ��
	 audio = new QAudioSink(audioDevice, fmt);  //ԭ��д��QAudioSink* audio= new QAudioSink(audioDevice, fmt)
	 //�Ǵ����˸��ֲ��������ͽ�ȫ�ֱ���������
		audio->setBufferSize(40000);
		audioIO = audio->start();
		/*audioIO->write();*/
		mutex.unlock();
		return true;
	}
	bool Stop() {
		mutex.lock();
		if (audio != NULL) {
			audio->stop();
			delete audio; // �����ڴ�
			audio = NULL;
			audioIO = NULL;
			
		}
		mutex.unlock();
		return true;
	}

	int Getfree() {
		mutex.lock();
		if (!audio)
		{
			mutex.unlock();
			return 0;
		}
	
		int free = audio->bytesFree();
		mutex.unlock();
		return free;
	}


	void Play(bool isplay) {
		mutex.lock();
		if (!audio)
		{
			mutex.unlock();
			return;
		}
		if (isplay) {
			audio->resume();
		}
		else {
			audio->suspend();
		}
		mutex.unlock();
	}
	bool Write(const char* data, int datasize) {//��ffmpeg���������д�뵽QAudioSink���в���
		mutex.lock();
		if(audioIO)
			audioIO->write(data, datasize);
		mutex.unlock();
		return true;
	}
};
xAudioplay* xAudioplay::Get()
{
	static cxAudioPlay ap;

	return &ap;
}




xAudioplay::~xAudioplay()
{

}

xAudioplay::xAudioplay()
{

}

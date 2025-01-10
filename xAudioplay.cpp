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

		fmt.setSampleRate(this->sampleRate); // 采样率
		fmt.setChannelCount(this->channel);    // 声道数
		fmt.setSampleFormat(QAudioFormat::Int16); // 位深
	 audio = new QAudioSink(audioDevice, fmt);  //原本写的QAudioSink* audio= new QAudioSink(audioDevice, fmt)
	 //是创造了个局部变量，就将全局变量覆盖了
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
			delete audio; // 清理内存
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
	bool Write(const char* data, int datasize) {//由ffmpeg处理的数据写入到QAudioSink进行播放
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

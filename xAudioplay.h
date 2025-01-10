#pragma once
class xAudioplay
{
public:
	int sampleRate = 48000;
	int sampleSize = 16;
	int channel = 2;


	static xAudioplay *Get();
	virtual bool Start()=0;
	virtual bool Stop() = 0;
	virtual void Play(bool isplay)=0;
	virtual bool Write(const char*data,int datasize)=0;
	virtual int Getfree() = 0;
	virtual ~xAudioplay();
protected:
	xAudioplay();
};


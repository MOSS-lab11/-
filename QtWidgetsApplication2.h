#pragma once

#include <QtWidgets/QWidget>
#include "ui_QtWidgetsApplication2.h"
#include <qthread.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
#include <libavutil/imgutils.h>
}

class QtWidgetsApplication2 : public QWidget
{
    Q_OBJECT

public:
    

    QtWidgetsApplication2(QWidget *parent = nullptr);
    ~QtWidgetsApplication2();
    void resizeEvent(QResizeEvent *e);
    void timerEvent(QTimerEvent* e);//�����麯��  timerevent����   ����ʱ������ʱ���������������
public slots:
    void open();
    void sliderPress();
    void sliderRelease();
    void play();
private:
    Ui::QtWidgetsApplication2Class ui;
    QThread* recvs;
};


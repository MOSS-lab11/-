#pragma once
#include<QtWidgets/qwidget.h>
#include<QOpenGLWidget>
#include<qthread.h>
class videoWidget:public QOpenGLWidget
{
public:
	QThread* thread_1=NULL;
	videoWidget(QWidget* p = NULL);
	void paintEvent(QPaintEvent*e);
	void timerEvent(QTimerEvent* e);
	virtual ~videoWidget();
};


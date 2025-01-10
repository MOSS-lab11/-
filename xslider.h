#pragma once
#include<qslider.h>

class xslider:public QSlider
{
	Q_OBJECT
public:
	xslider(QWidget*p=NULL);
	~xslider();
	void mousePressEvent(QMouseEvent*e);
};


#include "xslider.h"
#include "xslider.h"
#include<QMouseEvent>

void xslider::mousePressEvent(QMouseEvent* e)
{
	int value = ((float)e->pos().x() / (float)this->width()) * (this->maximum() + 1);
	this->setValue(value);
	QSlider::mousePressEvent(e);

}
xslider::xslider(QWidget* p) :QSlider(p)
{
}

xslider::~xslider()
{

}


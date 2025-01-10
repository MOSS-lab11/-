#include "QtWidgetsApplication2.h"
#include"qthread.h"
#include<qfiledialog.h>
#include"xffmpeg.h"
#include<qmessagebox.h>
#include "xAudioplay.h"
static bool ispressslider = false;
static bool isplay=true;

QtWidgetsApplication2::QtWidgetsApplication2(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    startTimer(40);
}
void QtWidgetsApplication2::open() {//槽函数
    QString name = QFileDialog::getOpenFileName(
        this, QString::fromLocal8Bit("选择视频文件")
    );
    if (name.isEmpty())return;
    this->setWindowTitle(name);
    if (xffmpeg::Get()->Open(name.toLocal8Bit())==false) {
        QMessageBox::information(this, "err", "file open failed!");
        return;
    }
    if (xffmpeg::Get()->totalSec <= 0) {
        QMessageBox::information(this, "err", "file open failed!");
        return;
    }

    xAudioplay::Get()->sampleRate = xffmpeg::Get()->sampleRate;
    xAudioplay::Get()->channel = xffmpeg::Get()->channel;
    xAudioplay::Get()->sampleSize = 16;
    xAudioplay::Get()->Start();
        char buf[1024] = { 0 };
        int min= (xffmpeg::Get()->totalSec / 1000) / 60;
        int sec = (xffmpeg::Get()->totalSec / 1000)%60;
        snprintf(buf, sizeof(buf), "%d:%d", min, sec);
        ui.totaltime->setText(buf);
        isplay = false;
        play();
    
}
QtWidgetsApplication2::~QtWidgetsApplication2()
{}

void QtWidgetsApplication2::resizeEvent(QResizeEvent * e)
{
    ui.openGLWidget->resize(size());
  
    ui.openbutton->move(this->width()/2-50,this->height()-80);
    ui.playbutton->move(this->width()/2+50,this->height()-80);
    ui.playSlider->move(25, this->height() - 120);
    ui.playSlider->resize(this->width() - 50, ui.playSlider->height());
    ui.playtime->move(25, ui.playbutton->y());
    ui.totaltime ->move(100, ui.playbutton->y());
    ui.sprt->move(ui.playtime->x()+ui.playtime->width()+1, ui.playtime->y());
    
}

void QtWidgetsApplication2::timerEvent(QTimerEvent * e)
{
   
    int sec = (xffmpeg::Get()->pts / 1000)%60;
    int min = (xffmpeg::Get()->pts / 1000)/60;
    char buf[1024] = { 0 };
    printf("pts==%d sec==%d,min==%d\n", xffmpeg::Get()->pts, sec, min);
    printf("pts==%d sec==%d,min==%d\n", xffmpeg::Get()->pts, sec, min);
    printf("pts==%d sec==%d,min==%d\n", xffmpeg::Get()->pts, sec, min);
    printf("pts==%d sec==%d,min==%d\n", xffmpeg::Get()->pts, sec, min);
    snprintf(buf,sizeof(buf), "%03d:%02d", min, sec);
    ui.playtime->setText(buf);

    if (xffmpeg::Get()->totalSec > 0) {
        float rate = (float)xffmpeg::Get()->pts / (float)xffmpeg::Get()->totalSec;
        if (!ispressslider) {//按下的时候不去刷新，松开seek函数
            ui.playSlider->setValue(rate * 1000);
        }
    }
}

void QtWidgetsApplication2::sliderPress()
{
    ispressslider = true;

}

void QtWidgetsApplication2::sliderRelease()
{
    ispressslider = false;
    float pos = (float)(ui.playSlider->value() / (float)(ui.playSlider->maximum() + 1));
    xffmpeg::Get()->seek(pos);
}

void QtWidgetsApplication2::play()
{
    isplay = !isplay;
    xffmpeg::Get()->isplay = isplay;
    if (isplay) {
        //暂停状态
        const QString sta_1 = "stop";
    
        ui.playbutton->setText(sta_1);
    }
    else if (!isplay) {
        const QString sta_2 = "start";

        ui.playbutton->setText(sta_2);
    }
}

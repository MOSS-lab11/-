#include "videoWidget.h"
#include <QPainter>
#include <QPoint>
#include "xffmpeg.h"
#include "xvideothread.h"

videoWidget::videoWidget(QWidget* p) : QOpenGLWidget(p) {
    thread_1 = new QThread;
    xvideothread* task_class_1 = new xvideothread;
    task_class_1->moveToThread(thread_1);
    QObject::connect(thread_1, &QThread::started, task_class_1, &xvideothread::task_decode);
    startTimer(20); // 50 FPS
    thread_1->start();
}

void videoWidget::paintEvent(QPaintEvent* e) {
    static QImage image;
    static int w = 0;
    static int h = 0;

    if (w != width() || h != height()) {
        // Only reallocate if the size has changed
        image = QImage(width(), height(), QImage::Format_ARGB32);
        w = width();
        h = height();
    }

    // Convert video frame to RGB and update the image
    if (xffmpeg::Get()->ToRGB((char*)image.bits(), width(), height())) {
        QPainter painter(this);
        painter.drawImage(QPoint(0, 0), image);
    }
}

void videoWidget::timerEvent(QTimerEvent* e) {
    update(); // Trigger a repaint
}

videoWidget::~videoWidget() {
    // Ensure the thread is properly stopped and resources are cleaned up
    thread_1->quit();
    thread_1->wait();
    delete thread_1;
}


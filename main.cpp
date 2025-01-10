#include "QtWidgetsApplication2.h"
#include <QtWidgets/QApplication>
#include <stdio.h>
#include<QAudioOutput>
#include <QAudioFormat>
#include"xffmpeg.h"

int outwidth = 640;
int outhight = 480;

SwsContext* cCtx = NULL;


//static double r2d(AVRational r) {//avrational表示有理数
//    return r.num == 0 || r.den == 0?0:(double)r.num/(double)r.den;
//}



int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QtWidgetsApplication2 w;
    w.show();
    app.exec();
}

#pragma once

#include <QObject>
#include <QString>
#include <QtNetwork>
#include <QStringList>
#include <QDateTime>
#include <QTimer>
#include <QRegExp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>
#include "ComFunction.h"
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;
using std::placeholders::_5;


#define SafeDelete(p) {delete p; p = 0;}
// #define MyDebug() RTC_LOG(LS_INFO)
#define CONVERT_TO_STRING(...) QString(#__VA_ARGS__)

#define QT_TO_STD(qt) (qt.toLatin1().data())
#define STD_TO_QT(std) (QString::fromStdString(std))

#include "rtc_base/logging.h"



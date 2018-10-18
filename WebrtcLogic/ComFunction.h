#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "media/base/videocapturer.h"
#include "modules\video_capture\video_capture.h"
#include "modules\video_capture\video_capture_factory.h"
#include "media\engine\webrtcvideocapturerfactory.h"

bool ConvertToJson(const QString &msg, QJsonArray &ret);
bool ConvertToJson(const QString &msg, QJsonObject &ret);

#define __GET_VALUE_FROM_OBJ__(obj, key, type, dst)\
	if (obj.contains(key))\
	{\
		QJsonValue value = obj.value(key); \
		if (value.is##type())\
		{\
			dst = value.to##type();\
		}\
	}

// template <class FUNCTION, class DST>
// class JsonConvert
// {
// 	static convert(QJsonObject obj, char *key, FUNCTION type, DST dst)
// 	{
// 		if (obj.contains(key))
// 		{
// 			QJsonValue value = obj.value(key);
// 			if (value.is##type())
// 			{
// 				dst = value.to##type();
// 			}
// 		}
// 	}
// };

void GetCameraDevices(std::vector<std::string> &device_names);
QString GetRandomString(int len);

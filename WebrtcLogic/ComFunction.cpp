#include "stdafx.h"
#include "ComFunction.h"

bool ConvertToJson(const QString &msg, QJsonArray &ret)
{
	QJsonParseError parseError;
	QJsonDocument document = QJsonDocument::fromJson(msg.toLatin1(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
// 		qDebug() << parseError.errorString();
		return false;
	}

	if (!document.isArray())
	{
		return false;
	}

	ret = document.array();
	return true;
}

bool ConvertToJson(const QString &msg, QJsonObject &ret)
{
	QJsonParseError parseError;
	QJsonDocument document = QJsonDocument::fromJson(msg.toLatin1(), &parseError);
	if (parseError.error != QJsonParseError::NoError)
	{
// 		qDebug() << parseError.errorString();
		return false;
	}

	if (!document.isObject())
	{
		return false;
	}

	ret = document.object();
	return true;
}

void GetCameraDevices(std::vector<std::string> &device_names)
{
	device_names.clear();
	std::unique_ptr<webrtc::VideoCaptureModule::DeviceInfo> info(
		webrtc::VideoCaptureFactory::CreateDeviceInfo());
	if (!info) {
		return;
	}
	int num_devices = info->NumberOfDevices();
	for (int i = 0; i < num_devices; ++i) {
		const uint32_t kSize = 256;
		char name[kSize] = { 0 };
		char id[kSize] = { 0 };
		if (info->GetDeviceName(i, name, kSize, id, kSize) != -1) {
			device_names.push_back(name);
		}
	}
}


QString GetRandomString(int len)
{
	static const QString charSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	static int strLen = charSet.length();

	QString randomString;
	for (int i = 0; i < len; i++)
	{
		randomString += charSet.at(qrand() % strLen);
	}
	return randomString;
}

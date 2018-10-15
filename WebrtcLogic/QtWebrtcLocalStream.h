#pragma once

#include "modules/video_capture/video_capture_factory.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "api/peerconnectioninterface.h"
#include "QtWebrtcStream.h"
#include "modules/desktop_capture/desktop_geometry.h"

class QtWebrtcLocalStream : public QObject, public QtWebrtcStream
{
	Q_OBJECT

public:
	QtWebrtcLocalStream();
	virtual ~QtWebrtcLocalStream();
	static QtWebrtcLocalStream *GetInstance()
	{
		static QtWebrtcLocalStream *instance = new QtWebrtcLocalStream();
		instance->Init();
		return instance;
	};

	bool Init();

	bool CloseVideo();
	bool OpenCamrea(QString &name);
	bool OpenScreen(int id);
	bool OpenScreen(int id, webrtc::DesktopRect rect);
	bool OpenWindow(int id);

	static webrtc::DesktopRect GetScreenRect(int id);
	static bool GetDeviceById(int id, std::string *device_name, std::wstring *device_key);

Q_SIGNALS:
	void VideoTrackChanged();

private:
	bool SetCapture(std::unique_ptr<cricket::VideoCapturer> video_device);

private:
	std::once_flag			of;
};

#define _WebrtcLocalStream QtWebrtcLocalStream::GetInstance()

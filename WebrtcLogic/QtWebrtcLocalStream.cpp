#include "stdafx.h"
#include "QtWebrtcLocalStream.h"
#include "ConnecttionFactory.h"
#include "modules\desktop_capture\win_video_capture.h"
#include "modules\desktop_capture\win\screen_capture_utils.h"
#include "modules\desktop_capture\win\window_capture_utils.h"

const char kAudioLabel[] = "audio_label";
const char kVideoLabel[] = "video_label";

QtWebrtcLocalStream::QtWebrtcLocalStream()
{
}

QtWebrtcLocalStream::~QtWebrtcLocalStream()
{
	StopView();
}

bool QtWebrtcLocalStream::Init()
{
	try
	{
		std::call_once(of, [&]
		{
 			_audioTrack = _ConnecttionFactory->CreateAudioTrack(kAudioLabel, _ConnecttionFactory->CreateAudioSource(cricket::AudioOptions()));

			std::vector<std::string> device_names;
			GetCameraDevices(device_names);
			if (device_names.size() == 0)
			{
				throw "no device";
			}

			OpenCamrea(STD_TO_QT(device_names[0]));
		});

	}
	catch (...)
	{
		return false;
	}

	return _videoTrack != NULL;
}

bool QtWebrtcLocalStream::CloseVideo()
{
	StopView();
	_videoTrack = NULL;
	emit VideoTrackChanged();
	return true;
}

bool QtWebrtcLocalStream::SetCapture(std::unique_ptr<cricket::VideoCapturer> video_device)
{
	if (!video_device)
		return false;

	rtc::scoped_refptr<webrtc::VideoTrackInterface> videoTrack =
		_ConnecttionFactory->CreateVideoTrack(kVideoLabel,
			_ConnecttionFactory->CreateVideoSource(std::move(video_device), NULL));

	if (!videoTrack)
		return false;

	_videoTrack = std::move(videoTrack);

	StartView();
	emit VideoTrackChanged();
	return true;
}

bool QtWebrtcLocalStream::OpenCamrea(QString &name)
{
	std::string deviceName = QT_TO_STD(name);

	std::vector<std::string> device_names;
	GetCameraDevices(device_names);

	cricket::WebRtcVideoDeviceCapturerFactory factory;
	std::unique_ptr<cricket::VideoCapturer> capturer;

	for (const auto& name : device_names)
	{
		if (deviceName == name)
		{
			capturer = factory.Create(cricket::Device(name, name));
			if (capturer)
			{
				break;
			}
		}
	}

	return SetCapture(std::move(capturer));
}

webrtc::DesktopRect QtWebrtcLocalStream::GetScreenRect(int id)
{
	std::wstring device_key;

	if (!QtWebrtcLocalStream::GetDeviceById(id, NULL, &device_key))
	{
		return webrtc::DesktopRect();
	}

	return webrtc::GetScreenRect(id, device_key);
}

bool QtWebrtcLocalStream::GetDeviceById(int id, std::string *device_name, std::wstring *device_key)
{
	std::vector<int> device_id;
	std::vector<std::string> device_names;
	std::vector<std::string> device_keys;
	webrtc::GetScreenList(&device_id, &device_names, &device_keys);

	for (int index = 0; index < device_id.size(); index++)
	{
		if (device_id[index] == id)
		{
			if (device_name)
			{
				*device_name = device_names[index];
			}
			if (device_key)
			{
				device_key->assign(device_keys[index].begin(), device_keys[index].end());
			}

			return true;
		}
	}

	return false;
}

bool QtWebrtcLocalStream::OpenScreen(int id)
{
	return SetCapture(webrtc::WinVideoCapture::CreateScreenVideoCapturer(GetScreenRect(id), id));
}

bool QtWebrtcLocalStream::OpenScreen(int id, webrtc::DesktopRect rect)
{
	if (!GetScreenRect(id).ContainsRect(rect))
	{
		return false;
	}

	return SetCapture(webrtc::WinVideoCapture::CreateScreenVideoCapturer(rect, id));
}

bool QtWebrtcLocalStream::OpenWindow(int id)
{
	return SetCapture(webrtc::WinVideoCapture::CreateWindowVideoCapturer(id));
}

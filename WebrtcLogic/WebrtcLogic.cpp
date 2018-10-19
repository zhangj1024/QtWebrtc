#include "stdafx.h"
#include "WebrtcLogic.h"
#include "QConferenceManager.h"
#include "QtWebrtcLocalStream.h"

#include "modules\desktop_capture\win\screen_capture_utils.h"
#include "modules\desktop_capture\win\window_capture_utils.h"

WebrtcLogic::WebrtcLogic()
{

}

void WebrtcLogic::Login(QString server)
{
	std::string stdServer = QT_TO_STD(server);
	_QConferenceManager->Login(stdServer);
}

void WebrtcLogic::Logout()
{
	_QConferenceManager->Logout();
}

void WebrtcLogic::ConnectToPeer(qint64 peer_id)
{
	_QConferenceManager->ConnectToPeer(peer_id, true, true);
}

void WebrtcLogic::DisConnectToPeer()
{
	_QConferenceManager->DisConnectToPeer();
}

void WebrtcLogic::SetWnds(HWND local, QVector<HWND>remote)
{
	_QConferenceManager->SetWnds(local, remote);
}

void WebrtcLogic::Init()
{
	_QConferenceManager->Init();
}

void WebrtcLogic::Uninit()
{
	_QConferenceManager->Uninit();
}

void WebrtcLogic::StartPreview()
{
	_QConferenceManager->StartLocalPreview();
}

void WebrtcLogic::StopPreview()
{
	_QConferenceManager->StopLocalPreview();
}


void WebrtcLogic::CloseVideo()
{
	_WebrtcLocalStream->CloseVideo();
}

void WebrtcLogic::GetCameraList(QVector<QString> &devices)
{
	std::vector<std::string> device_names;
	GetCameraDevices(device_names);

	for (auto device : device_names)
	{
		devices.push_back(STD_TO_QT(device));
	}
}

void WebrtcLogic::OpenCamrea(QString &deviceName)
{
	_WebrtcLocalStream->OpenCamrea(deviceName);
}

void WebrtcLogic::GetScreenList(QVector<QPair<QString, int> > &devices)
{
	webrtc::DesktopCapturer::SourceList screens;
	webrtc::GetScreenList(&screens);

	for (auto screen : screens)
	{
		devices.push_back(QPair<QString, int>(STD_TO_QT(screen.title), screen.id));
	}
}

QRect WebrtcLogic::GetScreenRect(int id)
{
	webrtc::DesktopRect rect = QtWebrtcLocalStream::GetScreenRect(id);
	return QRect(rect.left(), rect.top(), rect.width(), rect.height());
}

void WebrtcLogic::OpenScreen(int id)
{
	_WebrtcLocalStream->OpenScreen(id);
}

void WebrtcLogic::OpenScreen(int id, QRect rect)
{
	_WebrtcLocalStream->OpenScreen(id, webrtc::DesktopRect::MakeXYWH(rect.left(), rect.top(), rect.width(), rect.height()));
}

void WebrtcLogic::GetWindowList(QVector<QPair<QString, int> > &devices)
{
	webrtc::DesktopCapturer::SourceList windows;
	webrtc::GetWindowList(&windows);

	for (auto window : windows)
	{
		devices.push_back(QPair<QString, int>(STD_TO_QT(window.title), window.id));
	}
}

void WebrtcLogic::OpenWindow(int id)
{
	_WebrtcLocalStream->OpenWindow(id);
}

void WebrtcLogic::EnableSend(bool enable)
{
	_QConferenceManager->EnableSend(enable);
}
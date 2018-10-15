#pragma once

#include "webrtclogic_global.h"

class WEBRTCLOGIC_EXPORT WebrtcLogic
{
public:
	WebrtcLogic();

	void Login(QString server);
	void Logout();

	void ConnectToPeer(qint64 peer_id);
	void DisConnectToPeer();

	void SetWnds(HWND local, QVector<HWND>remote);

	void Init();
	void Uninit();

	void StartPreview();
	void StopPreview();

	void CloseVideo();

	void GetCameraList(QVector<QString> &devices);
	void OpenCamrea(QString &deviceName);

	void GetScreenList(QVector<QPair<QString, int> > &devices);
	QRect GetScreenRect(int id);

	void OpenScreen(int id);
	void OpenScreen(int id, QRect rect);

	void GetWindowList(QVector<QPair<QString, int> > &devices);
	void OpenWindow(int id);

	void EnableSend(bool enable);

};

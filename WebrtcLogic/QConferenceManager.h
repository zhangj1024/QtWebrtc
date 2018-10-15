#pragma once

#include <QObject>

#include "rtc_base/checks.h"
#include "rtc_base/ssladapter.h"
#include "rtc_base/win32socketinit.h"
#include "rtc_base/win32socketserver.h"
#include "peer_connection_client.h"
#include "QtWebrtcRemoteStream.h"
#include "QtLicodeSignalling.h"

#define SIGNALING_LICODE

class QConferenceManager : public QObject
{
	Q_OBJECT

public:
	QConferenceManager(QObject *parent = NULL);
	~QConferenceManager();

	static QConferenceManager *getInstance()
	{
		static QConferenceManager *instance = new QConferenceManager();
		return instance;
	};

	bool Init();
	void Uninit();

	void StartLocalPreview();
	void StopLocalPreview();


	void Login(std::string &server);
	void Logout();

	void DisConnectToPeer();

	void SetWnds(HWND local, QVector<HWND>remote);

	void EnableSend(bool enable);
Q_SIGNALS:
	void initError(QString error);

public slots :
	void ConnectToPeer(qint64 peer_id, bool remote);

private slots:
// 	void onRemoteStreamAdd(qint64 streamId, bool show);
	void onRemoteStreamRemove(qint64 streamId);
#ifdef SIGNALING_LICODE
	void onStreamStarted(qint64 streamId);
#endif
	void onLocalSDP(qint64 id, QString sdp, QString type);
	void onLocalIceCandidate(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void onIceGatheringComplete(qint64 id);

	void onRetmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void onRetmoeSDP(qint64 id, QString type, QString sdp);

private:
	struct iceCandidate
	{
		QString sdp_mid;
		int sdp_mline_index;
		QString sdp;
	};

	void sendICEs(qint64 id, QVector<iceCandidate> &iceCandidateList);
	void AddStreamInfo(rtc::scoped_refptr<QtWebrtcRemoteStream> remoteStream);

private:
	rtc::Win32SocketServer *w32_ss;
	rtc::Win32Thread *w32_thread;

	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _peer_connection_factory;

	struct RemoteStreamInfo
	{
	public:
		rtc::scoped_refptr<QtWebrtcRemoteStream> stream;

		QString sdp;
		QString sdpType;

		QVector<iceCandidate> iceCandidateList;

// 		bool remote = false;

		bool canSendSDP;
		bool canSendCandidate;
	};
	QHash<qint64, RemoteStreamInfo> _remoteStreamInfos;
#ifdef SIGNALING_LICODE
	QtLicodeSignalling _signalling;
#else
	PeerConnectionClient _signalling;
#endif

	HWND _localHwnd;
	QVector<QPair<HWND, qint64> > _remoteHwnds;

// 	rtc::scoped_refptr<QtWebrtcRemoteStream> _conduct;
};

#define _QConferenceManager QConferenceManager::getInstance()

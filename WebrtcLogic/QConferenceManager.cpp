#include "stdafx.h"
#include "QConferenceManager.h"
#include <functional>
#include "ConnecttionFactory.h"
#include "QtWebrtcLocalStream.h"

QConferenceManager::QConferenceManager(QObject *parent)
	: QObject(parent)
{
	rtc::EnsureWinsockInit();
	w32_ss = new rtc::Win32SocketServer();
	w32_thread = new rtc::Win32Thread(w32_ss);
	rtc::ThreadManager::Instance()->SetCurrentThread(w32_thread);
	rtc::InitializeSSL();

#ifdef SIGNALING_LICODE
	QObject::connect(&_signalling, SIGNAL(signal_StreamStarted(qint64)), this, SLOT(onStreamStarted(qint64)));
#endif
	QObject::connect(&_signalling, SIGNAL(signal_PeerConnected(qint64, bool)), this, SLOT(ConnectToPeer(qint64, bool)));
	QObject::connect(&_signalling, SIGNAL(signal_PeerDisconnected(qint64)), this, SLOT(onRemoteStreamRemove(qint64)));

	QObject::connect(&_signalling, SIGNAL(retmoeIce(qint64, QString, int, QString)), this, SLOT(onRetmoeIce(qint64, QString, int, QString)));
	QObject::connect(&_signalling, SIGNAL(retmoeSDP(qint64, QString, QString)), this, SLOT(onRetmoeSDP(qint64, QString, QString)));
}

QConferenceManager::~QConferenceManager()
{
	ConnecttionFactory::Get() = nullptr;
	w32_thread->Stop();
	delete w32_thread;
	rtc::CleanupSSL();
}

bool QConferenceManager::Init()
{

	return true;
}

void QConferenceManager::StartLocalPreview()
{
	_WebrtcLocalStream->SetHwnd(_localHwnd);
	_WebrtcLocalStream->StartView();
}

void QConferenceManager::StopLocalPreview()
{
	_WebrtcLocalStream->StopView();
}

void QConferenceManager::Login(std::string &server)
{
	_signalling.ConnectServer(server);
}

void QConferenceManager::Logout()
{
	for (auto &hwnd : _remoteHwnds)
	{
		hwnd.second = 0;
	}
	DisConnectToPeer();
	_signalling.DisconnectServer();
}

void QConferenceManager::ConnectToPeer(qint64 peer_id, bool remote)
{
	_WebrtcLocalStream->Init();
	StartLocalPreview();

	rtc::scoped_refptr<QtWebrtcRemoteStream> remoteStream;

	remoteStream = new rtc::RefCountedObject<QtWebrtcRemoteStream>(peer_id);
	QObject::connect(remoteStream, SIGNAL(LocalIceCandidate(qint64, QString, int, QString)), this, SLOT(onLocalIceCandidate(qint64, QString, int, QString)));
	QObject::connect(remoteStream, SIGNAL(LocalSDP(qint64, QString, QString)), this, SLOT(onLocalSDP(qint64, QString, QString)));
	QObject::connect(remoteStream, SIGNAL(IceGatheringComplete(qint64)), this, SLOT(onIceGatheringComplete(qint64)));

	AddStreamInfo(remoteStream);

	if (remote)
	{
		for (auto &hwnd : _remoteHwnds)
		{
			if (hwnd.second == 0)
			{
				RTC_LOG(LS_INFO) << "set peer hwnd:" << peer_id << " " << hwnd.first;
				remoteStream->SetHwnd(hwnd.first);
				hwnd.second = peer_id;
				break;
			}
		}
	}

	remoteStream->ConnectToPeer();
#ifdef SIGNALING_LICODE
	remoteStream->EnableSend(!remote);
#endif
}

void QConferenceManager::AddStreamInfo(rtc::scoped_refptr<QtWebrtcRemoteStream> remoteStream)
{
	RemoteStreamInfo stream;
	stream.stream = remoteStream;

#ifdef SIGNALING_LICODE
	stream.canSendSDP = false;
	stream.canSendCandidate = false;
#else 
	stream.canSendSDP = true;
	stream.canSendCandidate = true;
#endif

	qint64 peerId = remoteStream->id();

	RTC_LOG(LS_INFO) << __FUNCTION__ << peerId;
	_remoteStreamInfos[peerId] = stream;
}

void QConferenceManager::DisConnectToPeer()
{
	for (const auto remoteStream : _remoteStreamInfos)
	{
		_signalling.DisconnectPeer(remoteStream.stream->id());
		remoteStream.stream->DeletePeerConnection();
// 		_remoteStreamInfos.remove(remoteStream.stream->id());
	}
	_remoteStreamInfos.clear();
}

void QConferenceManager::SetWnds(HWND local, QVector<HWND>remote)
{
	_localHwnd = local;
	_remoteHwnds.clear();
	for (auto hwnd : remote)
	{
		_remoteHwnds.push_back(QPair<HWND, qint64>(hwnd, 0));
	}
}

// void QConferenceManager::onRemoteStreamAdd(qint64 streamId, bool remote)
// {
// 	ConnectToPeer(streamId, true);
// }

void QConferenceManager::onRemoteStreamRemove(qint64 id)
{
	auto itr = _remoteStreamInfos.find(id);
	if (itr != _remoteStreamInfos.end())
	{
		_signalling.DisconnectPeer(itr->stream->id());
		itr->stream->DeletePeerConnection();

		_remoteStreamInfos.erase(itr);
	}

	for (auto &hwnd : _remoteHwnds)
	{
		if (hwnd.second == id)
		{
			RTC_LOG(LS_INFO) << "remove peer widget:" << id << " " << hwnd.first;
			hwnd.second = 0;
			break;
		}
	}
}

#ifdef SIGNALING_LICODE
void QConferenceManager::onStreamStarted(qint64 streamId)
{
	auto itr = _remoteStreamInfos.find(streamId);
	if (itr == _remoteStreamInfos.end())
	{
		return;
	}

	auto &remoteStreamState = *itr;
	auto &remoteStream = remoteStreamState.stream;

	remoteStreamState.canSendSDP = true;

	if (!remoteStreamState.sdp.isEmpty() && !remoteStreamState.sdpType.isEmpty())
	{
		_signalling.SendSDP(streamId, remoteStreamState.sdp, remoteStreamState.sdpType);
		remoteStreamState.sdp.clear();
		remoteStreamState.sdpType.clear();
	}
}
#endif

void QConferenceManager::onLocalSDP(qint64 id, QString sdp, QString type)
{
	auto itr = _remoteStreamInfos.find(id);
	if (itr == _remoteStreamInfos.end())
	{
		return;
	}

	auto &remoteStreamState = *itr;
	auto &remoteStream = remoteStreamState.stream;

	remoteStreamState.sdp = sdp;
	remoteStreamState.sdpType = type;

	if (remoteStreamState.canSendSDP)
	{
		_signalling.SendSDP(id, sdp, type);
	}
}

void QConferenceManager::sendICEs(qint64 id, QVector<iceCandidate> &iceCandidateList)
{
	while (iceCandidateList.count() > 0)
	{
		auto candidate = iceCandidateList.front();
		iceCandidateList.pop_front();
		_signalling.SendCandidate(id, candidate.sdp_mid, candidate.sdp_mline_index, candidate.sdp);
	}
}

void QConferenceManager::onLocalIceCandidate(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate)
{
	auto itr = _remoteStreamInfos.find(id);
	if (itr == _remoteStreamInfos.end())
	{
		return;
	}

	auto &remoteStreamState = *itr;
	auto &remoteStream = remoteStreamState.stream;


#ifdef SIGNALING_LICODE
	if (!candidate.startsWith("end"))
	{
		candidate = "a=" + candidate;
	}
#endif

	QVector<iceCandidate> &iceCandidateList = remoteStreamState.iceCandidateList;
	iceCandidateList.push_back(iceCandidate{ sdp_mid, sdp_mlineindex, candidate });

	if (remoteStreamState.canSendCandidate)
	{
		sendICEs(remoteStream->id(), iceCandidateList);
	}
}

void QConferenceManager::onIceGatheringComplete(qint64 id)
{
#if (defined (SIGNALING_LICODE)) || (defined (SIGNALING_JANUS))
	onLocalIceCandidate(id, "end", -1, "end");
#endif
}

void QConferenceManager::onRetmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate)
{
	auto itr = _remoteStreamInfos.find(id);
	if (itr == _remoteStreamInfos.end())
	{
#if 0
		ConnectToPeer(id, true, false);
		itr = _remoteStreamInfos.find(id);
		if (itr == _remoteStreamInfos.end())
#endif
		{
			return;
		}
	}

	auto &remoteStreamState = *itr;
	auto &remoteStream = remoteStreamState.stream;
	remoteStream->AddPeerIceCandidate(sdp_mid, sdp_mlineindex, candidate);
}

void QConferenceManager::onRetmoeSDP(qint64 id, QString type, QString sdp)
{
	auto itr = _remoteStreamInfos.find(id);
	if (itr == _remoteStreamInfos.end())
	{
#if 0
		ConnectToPeer(id, true, false);
		itr = _remoteStreamInfos.find(id);
		if (itr == _remoteStreamInfos.end())
#endif
		{
			return;
		}
	}

	auto &remoteStreamState = *itr;
	auto &remoteStream = remoteStreamState.stream;
	remoteStream->SetPeerSDP(type, sdp);

#ifdef SIGNALING_LICODE
	remoteStreamState.canSendCandidate = true;
	sendICEs(remoteStream->id(), remoteStreamState.iceCandidateList);
#endif
}

void QConferenceManager::Uninit()
{
	for (auto &hwnd : _remoteHwnds)
	{
		hwnd.second = 0;
	}

	DisConnectToPeer();
	_signalling.DisconnectServer();

	delete _WebrtcLocalStream;
	ConnecttionFactory::UnInit();
}

void QConferenceManager::EnableSend(bool enable)
{
	for (auto remote : _remoteStreamInfos)
	{
		remote.stream->EnableSend(enable);
	}
}
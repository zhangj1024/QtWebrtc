#pragma once
#include <QObject>
#include "JanusWebsocket.h"

class JanusSignalling : public QObject
{
	Q_OBJECT
public:
	explicit JanusSignalling(QObject *parent = Q_NULLPTR);
	virtual ~JanusSignalling();

	void ConnectServer(const std::string& server);
	void DisconnectServer();
	bool DisconnectPeer(qint64 peer_id);

public slots:
	void SendSDP(qint64 id, QString sdp, QString type);
	void SendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate);
	void SendCandidateCompleted(qint64 id);

Q_SIGNALS:
	void signal_SignedIn();  // Called when we're logged on.
	void signal_Disconnected();
	void signal_PeerConnected(qint64 id, bool remote);
	void signal_PeerDisconnected(qint64 id);
	void signal_ServerConnectionFailure();

	void signal_StreamStarted(qint64 id);

	void retmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void retmoeSDP(qint64 id, QString type, QString sdp);

private slots:
	void on_socket_connected();
	void on_socket_disconnected(QString endpoint);
	void on_socket_errorReceived(QString reason, QString advice);

private:
	void CreateSessionId();
	void OnCreateSessionId(const QJsonObject &recvmsg);

	void AttachVideoRoom();
	void OnAttachVideoRoom(const QJsonObject &recvmsg);

	void Join(qint64 id);
	void OnJoin(const QJsonObject &recvmsg, qint64 id);
	void OnEventJoin(const QJsonObject &recvdata);

	void OnSendSDP(const QJsonObject &recvmsg);
	void OnEventRemoteSDP(const QJsonObject &recvdata);

	void OnSendCandidate(const QJsonObject &recvmsg);
	void Subscribe(qint64 streamId);
	void onSubscribe(qint64 streamId, QString msg);

private:
	JanusWebsocket _socket;
	QUrl m_requestUrl;

	qint64 _sessionId;
};



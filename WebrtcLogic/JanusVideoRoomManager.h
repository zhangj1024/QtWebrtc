#pragma once

#include <QObject>
#include "JanusWebsocket.h"
#include "JanusPeerconnection.h"

class JanusVideoRoomManager : public QObject
{
	Q_OBJECT

public:
	JanusVideoRoomManager(QObject *parent = NULL);
	~JanusVideoRoomManager();

	void ConnectServer(const std::string& server);
	void DisconnectServer();

	void Join();
	bool DisconnectPeer(qint64 peer_id);

public slots:
	void SendSDP(qint64 &id, QString &sdp, QString &type);
	void SendCandidate(qint64 &id, QString &sdpMid, int &sdpMLineIndex, QString &candidate);

Q_SIGNALS:
	void signal_Login();
	void signal_Logout();
	void signal_Error(QString error);

	void signal_PeerConnected(qint64 id, bool show, bool connect);
	void signal_PeerDisconnected(qint64 id);

	void signal_RetmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void signal_RetmoeSDP(qint64 id, QString type, QString sdp);

private slots:
	void on_socket_connected();
	void on_socket_disconnected();

private:
	void CreateSessionId();
	void OnCreateSessionId(const QJsonObject &recvmsg);

	void OnEventJoin(const QJsonObject &recvdata);
	void OnEventEvent(const QJsonObject &recvdata);
	void OnEventAttached(const QJsonObject &recvdata);
	void OnEventUnpublish(const QJsonObject &recvdata);

	void OnEventSdp(const QJsonObject &recvdata);
	void OnEventPublishers(const QJsonObject &recvdata);

private:
	JanusWebsocket _socket;
	qint64 _sessionId = 0;
	QString _opaqueId;
	qint64 _privateId = 0;

	std::list<JanusPeerconnection *> peers;
};

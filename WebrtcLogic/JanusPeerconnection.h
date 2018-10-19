#pragma once

#include <QObject>
#include "JanusWebsocket.h"
#define JANUS "janus"

class JanusPeerconnection : public QObject
{
	Q_OBJECT

public:
	JanusPeerconnection(JanusWebsocket &socket, QString opaqueId, QObject *parent);
	~JanusPeerconnection();

	void AttachVideoRoom();
	void OnAttachVideoRoom(const QJsonObject &recvmsg);

	void Join();
	void OnJoin(const QJsonObject &recvmsg);

	//base method
	void SendSDP(QString sdp, QString type);
	void OnSendSDP(const QJsonObject &recvmsg);

	void SendCandidate(QString sdpMid, int sdpMLineIndex, QString candidate);
	void OnSendCandidate(const QJsonObject &recvmsg);

	inline qint64 GetHandleId() { return _handleId; };

	inline bool Subscribe() { return _subscribeId != 0; };
	inline void SetSubscribe(qint64 id) { _subscribeId = id; };
	inline qint64 GetSubscribe() { return _subscribeId; };

	inline void SetPrivateId(qint64 id) { _privateId = id; };

Q_SIGNALS:
	void signal_PeerConnected(qint64 id, bool subscribe);

private:
	void DetachVideoRoom();
	void OnDetachVideoRoom(const QJsonObject &recvdata);


private:
	JanusWebsocket &_socket;
	qint64 _privateId = 0;
	qint64 _handleId = 0;
	QString _opaqueId;
	qint64 _subscribeId = 0;
};

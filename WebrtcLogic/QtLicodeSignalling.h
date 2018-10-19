#pragma once
#include <QObject>
#include "SocketIo/QtSocketIoClient.h"
class QtLicodeSignalling : public QObject
{
	Q_OBJECT
public:
	QtLicodeSignalling();
	~QtLicodeSignalling();

	void ConnectServer(const std::string& server);
	void DisconnectServer();
	bool DisconnectPeer(qint64 peer_id);

public slots:
	void SendSDP(qint64 id, QString sdp, QString type);
	void SendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate);

Q_SIGNALS:
	void signal_SignedIn();  // Called when we're logged on.
 	void signal_Disconnected();
	void signal_PeerConnected(qint64 id, bool remote);
	void signal_PeerDisconnected(qint64 id);
	void signal_ServerConnectionFailure();

	void signal_StreamStarted(qint64 id);

	void signal_RetmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void signal_RetmoeSDP(qint64 id, QString type, QString sdp);

private slots:
	void on_socket_connected();
	void on_socket_disconnected(QString endpoint);
	void on_socket_errorReceived(QString reason, QString advice);

private:
	void connectWebsocket();
	void on_connectServer_response(int errorCode, QString errorInfo, QByteArray bytes);

	void onRoomConnectResult(QString msg);
	void publish();
	void onPublishResult(QString msg);
	void Subscribe(qint64 streamId);
	void onSubscribe(qint64 streamId, QString msg);

	//event
	void onAddStream(QString message);
	void onSignalingMsgErizo(QString message);
	void onRemoveStream(QString message);

	bool getEventObj(QString &message, QJsonObject &obj);
private:
	QString tokenId;
	QString host = "192.168.1.164:3001";
	bool secure;
	QString signature;

	QtSocketIoClient _socket;

	QString _roomid;
	qint64 _streamId = 0;
};

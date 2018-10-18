#pragma once
#include <QObject>
#include <QtWebSockets>

typedef std::function<void(const QJsonObject &recvdata)> janus_event_listener;

class JanusWebsocket : public QObject
{
	Q_OBJECT
public:

	explicit JanusWebsocket(QObject *parent = Q_NULLPTR);
	virtual ~JanusWebsocket();

	bool open(const QString &url);
	void close();

	void emitMessage(QJsonObject &message);
	void emitMessage(QJsonObject &message, const janus_event_listener &func);

	void setEventCallBack(const QString &event_name, const janus_event_listener &func);
	void setNotifyCallBack(const QString &event_name, const janus_event_listener &func);

	inline void setSessionId(const qint64 &id) { _sessionId = id; };
Q_SIGNALS:
	void connected();
	void disconnected();
	void errorReceived(QString reason);

private Q_SLOTS:
	void onWsError(QAbstractSocket::SocketError error);
	void onWsMessage(QString textMessage);

	void onPingTimer();
	void onPingTimeout();

private:
	QString doEmitMessage(QJsonObject &message);
	void onPingAck();

	void onEvent(const QString &event, const QJsonObject &object);
	void onNotify(const QString &notify, const QJsonObject &object);
	void onMsgAck(const QString &transaction, const QJsonObject &object);
	void onError(const QString &message);

private:
	QWebSocket *m_pWebSocket;
	QUrl m_requestUrl;
	QMap<QString, janus_event_listener> m_messageCallbacks;
	QMap<QString, janus_event_listener> m_eventCallbacks;
	QMap<QString, janus_event_listener> m_notifyCallbacks;


	QString m_sid;
	uint m_iPingInterval = 25000;
	uint m_iPingTimeout = 60000;
	QTimer *m_pPingTimer;
	QTimer *m_pPingTimeOutTimer;

	qint64 _sessionId = 0;
};


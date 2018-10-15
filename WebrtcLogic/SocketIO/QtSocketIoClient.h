#ifndef QSOCKETIOCLIENT_H
#define QSOCKETIOCLIENT_H

#include <QObject>
#include <QtWebSockets/QWebSocket>
#include <QTimer>

QT_BEGIN_NAMESPACE

enum FRAME_TYPE
{
	FRAME_OPEN		= 0,
	FRAME_CLOSE		= 1,
	FRAME_PING		= 2,
	FRAME_PONG		= 3,
	FRAME_MESSAGE	= 4,
	FRAME_UPGRADE	= 5,
	FRAME_NOOP		= 6,
};

enum PACKET_TYPE
{
	PACKET_CONNECT			= 0,
	PACKET_DISCONNECT		= 1,
	PACKET_EVENT			= 2,
	PACKET_ACK				= 3,
	PACKET_ERROR			= 4,
	PACKET_BINARY_EVENT		= 5,
	PACKET_BINARY_ACK		= 6,
};

typedef std::function<void(const QString& msg)> event_listener;
typedef QList<QVariant> valueList;


class QtSocketIoClient : public QObject
{
    Q_OBJECT
public:

    explicit QtSocketIoClient(QObject *parent = Q_NULLPTR);
    virtual ~QtSocketIoClient();

    bool open(const QString &url);
	void close();

    void emitMessage(const QString &message, const QVariant &value);
    void emitMessage(const QString &message, const QVariant &value, event_listener const& func);
	void emitMessage(const QString &message, const valueList &values);
	void emitMessage(const QString &message, const valueList &values, event_listener const& func);

	void on(QString const& event_name, event_listener const& func);

Q_SIGNALS:
	void connected();
	void disconnected(QString reason);
	void errorReceived(QString reason);

private Q_SLOTS:
	void onWsError(QAbstractSocket::SocketError error);
	void onWsMessage(QString textMessage);

    void onPingTimer();
	void onPingTimeout();

private:
	QString package(const QVariant &value);
	int doEmitMessage(const QString &name, const QString &msg);
	int doEmitMessage(const QString &name, const valueList &values);

	void parseMessage(const QString &message);
	void onHandshake(const QString &data);
	void onPong();
	void onMessage(const PACKET_TYPE ePacketType, const int messageId, const QString &message);

	void onEvent(const QString &message);
	void onMsgAck(const int messageId, const QString &message);
	void onError(const QString &message);

private:
    QWebSocket *m_pWebSocket;
    QUrl m_requestUrl;
	QMap<int, event_listener> m_messageCallbacks;
    QMap<QString, event_listener> m_eventCallbacks;


	QString m_sid;
	uint m_iPingInterval = 25000;
	uint m_iPingTimeout = 60000;
	QTimer *m_pPingTimer;
	QTimer *m_pPingTimeOutTimer;
};

QT_END_NAMESPACE

#endif // QSOCKETIOCLIENT_H

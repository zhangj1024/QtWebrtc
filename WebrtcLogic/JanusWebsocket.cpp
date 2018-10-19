#include "stdafx.h"
#include "JanusWebsocket.h"

JanusWebsocket::JanusWebsocket(QObject *parent) :
	QObject(parent),
	m_pWebSocket(new QWebSocket()),
	m_requestUrl(),
	m_pPingTimer(new QTimer()),
	m_pPingTimeOutTimer(new QTimer())
{
	connect(m_pPingTimer, SIGNAL(timeout()), this, SLOT(onPingTimer()));
	connect(m_pPingTimeOutTimer, SIGNAL(timeout()), this, SLOT(onPingTimeout()));

	connect(m_pWebSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWsError(QAbstractSocket::SocketError)));
	connect(m_pWebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onWsMessage(QString)));

	connect(m_pWebSocket, SIGNAL(connected()), this, SIGNAL(connected()));
	connect(m_pWebSocket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));

	m_pPingTimer->start(m_iPingInterval);
}

JanusWebsocket::~JanusWebsocket()
{
	QJsonObject destroyOby;
	destroyOby["janus"] = "destroy";
	emitMessage(destroyOby, std::bind(&JanusWebsocket::onPingAck, this));

	this->close();
	delete m_pPingTimer;
	delete m_pPingTimeOutTimer;
	delete m_pWebSocket;
}

void JanusWebsocket::onPingTimer()
{
	if (m_pWebSocket->state() == QAbstractSocket::ConnectedState)
	{
		if (!m_pPingTimeOutTimer->isActive())
		{
			m_pPingTimeOutTimer->start(m_iPingTimeout);
		}

		QJsonObject keepliveOjb;
		keepliveOjb["janus"] = "keepalive";

		emitMessage(keepliveOjb, std::bind(&JanusWebsocket::onPingAck, this));
	}
	else
	{
		m_pPingTimeOutTimer->stop();
	}
}

void JanusWebsocket::onPingTimeout()
{
	this->close();
	emit errorReceived("ping timeout!");
}

void JanusWebsocket::onPingAck()
{
	//reset ping timeout timer
	m_pPingTimeOutTimer->stop();
	m_pPingTimeOutTimer->start();
}

bool JanusWebsocket::open(const QString &url)
{
	QNetworkRequest request(url);
	request.setRawHeader("Sec-WebSocket-Protocol", "janus-protocol");

	if (url.startsWith("wss://"))
	{
		QSslConfiguration config = QSslConfiguration::defaultConfiguration();
		config.setPeerVerifyMode(QSslSocket::VerifyNone);
		request.setSslConfiguration(config);
	}

	m_pWebSocket->open(request);
	return true;
}

void JanusWebsocket::close()
{
	if (m_pWebSocket->state() != QAbstractSocket::SocketState::UnconnectedState)
	{
		m_pWebSocket->close();
	}
	m_pPingTimer->stop();
	m_pPingTimeOutTimer->stop();
}

void JanusWebsocket::onWsError(QAbstractSocket::SocketError error)
{
	qDebug() << " Error occurred: " << (int)error;
	emit errorReceived(" Error occurred: " + error);
}

void JanusWebsocket::onError(const QString &message)
{
	emit errorReceived(message);
}

QString JanusWebsocket::doEmitMessage(QJsonObject &message)
{
	QString transaction = GetRandomString(12);
	message["transaction"] = transaction;
	if (_sessionId != 0)
		message["session_id"] = _sessionId;

	qDebug() << "************************************" << message;
	m_pWebSocket->sendTextMessage(QString(QJsonDocument(message).toJson(QJsonDocument::Compact)));
	return transaction;
}

void JanusWebsocket::emitMessage(QJsonObject &message)
{
	doEmitMessage(message);
}

void JanusWebsocket::emitMessage(QJsonObject &message, const janus_event_listener &func)
{
	QString transaction = doEmitMessage(message);
	m_messageCallbacks[transaction] = func;
}

void JanusWebsocket::setEventCallBack(const QString &event_name, const janus_event_listener &func)
{
	m_eventCallbacks[event_name] = func;
}

void JanusWebsocket::setNotifyCallBack(const QString &notifyName, const janus_event_listener &func)
{
	m_notifyCallbacks[notifyName] = func;
}

void JanusWebsocket::onWsMessage(QString message)
{
	QJsonObject object;
	if (ConvertToJson(message, object))
	{
		qDebug() << "************************************" << object;

		QString janus;
		QString transaction;
		__GET_VALUE_FROM_OBJ__(object, "janus", String, janus);
		__GET_VALUE_FROM_OBJ__(object, "transaction", String, transaction);

		if (janus == "webrtcup"
			|| janus == "media"
			|| janus == "hangup")
		{
			onNotify(janus, object);
		}
		else if (janus == "event")
		{
			QJsonObject plugindataOjb;
			__GET_VALUE_FROM_OBJ__(object, "plugindata", Object, plugindataOjb);
// 
// 			QString plugin;
// 			__GET_VALUE_FROM_OBJ__(plugindataOjb, "plugin", String, plugin); // "plugin": "janus.plugin.videoroom"
// 
			QJsonObject dataOjb;
			__GET_VALUE_FROM_OBJ__(plugindataOjb, "data", Object, dataOjb);

			QString janusEvent;
			__GET_VALUE_FROM_OBJ__(dataOjb, "videoroom", String, janusEvent); // "videoroom": "joined"

			onEvent(janusEvent, object);
		}
		else if (janus == "ack"
			|| janus == "success")
		{
			QJsonObject dataOjb;
			__GET_VALUE_FROM_OBJ__(object, "data", Object, dataOjb);

			onMsgAck(transaction, dataOjb);
		}
	}
}

void JanusWebsocket::onEvent(const QString &event, const QJsonObject &object)
{
	for (auto itr = m_eventCallbacks.begin(); itr != m_eventCallbacks.end(); itr++)
	{
		if (itr.key() == event)
		{
			itr.value()(object);
		}
	}
}

void JanusWebsocket::onNotify(const QString &notify, const QJsonObject &object)
{
	auto itr = m_notifyCallbacks.find(notify);
	if (itr != m_notifyCallbacks.end())
	{
		auto callback = itr.value();
		callback(object);
		return;
	}
	qWarning() << "Invalid notify received: no name";
}

void JanusWebsocket::onMsgAck(const QString &transaction, const QJsonObject &object)
{
	auto itr = m_messageCallbacks.find(transaction);
	if (itr != m_messageCallbacks.end())
	{
		auto callback = itr.value();
		m_messageCallbacks.erase(itr);

		callback(object);
	}
}


#include "stdafx.h"
#include "QtSocketIoClient.h"
#include "ComFunction.h"

QtSocketIoClient::QtSocketIoClient(QObject *parent) :
    QObject(parent),
    m_pWebSocket(new QWebSocket()),
    m_requestUrl(),
	m_pPingTimer(new QTimer()),
	m_pPingTimeOutTimer(new QTimer())
{
	connect(m_pPingTimer, SIGNAL(timeout()), this, SLOT(onPingTimer()));
	connect(m_pPingTimeOutTimer, SIGNAL(timeout()), this, SLOT(OnPingTimeout()));

	connect(m_pWebSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onWsError(QAbstractSocket::SocketError)));
	connect(m_pWebSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(onWsMessage(QString)));
}

QtSocketIoClient::~QtSocketIoClient()
{
	this->close();
	delete m_pPingTimer;
	delete m_pPingTimeOutTimer;
	delete m_pWebSocket;
}

bool QtSocketIoClient::open(const QString &url)
{
// 	qint64 ss  = QDateTime::currentDateTime().currentMSecsSinceEpoch();
	QString socketUrl = url + /*QString("/socket.io/1/?t=%1").arg(ss) ;*/ QString("/socket.io/?EIO=3&transport=websocket");

	if(url.startsWith("wss://"))
	{
		QSslConfiguration config = QSslConfiguration::defaultConfiguration();
		config.setPeerVerifyMode(QSslSocket::VerifyNone);
		m_pWebSocket->setSslConfiguration(config);
	}

	m_pWebSocket->open(socketUrl);
    return true;
}

void QtSocketIoClient::close()
{
	if (m_pWebSocket->state() != QAbstractSocket::SocketState::UnconnectedState)
	{
		m_pWebSocket->close();
	}
	m_pPingTimer->stop();
	m_pPingTimeOutTimer->stop();
}

void QtSocketIoClient::on(QString const& event_name, event_listener const& func)
{
	m_eventCallbacks[event_name] = func;
}

void QtSocketIoClient::emitMessage(const QString &message, const QVariant &value)
{
	doEmitMessage(message, package(value));
}

void QtSocketIoClient::emitMessage(const QString &message, const QVariant &value, event_listener const &func)
{
	int id = doEmitMessage(message, package(value));
	m_messageCallbacks[id] = func;
}

void QtSocketIoClient::emitMessage(const QString & message, const valueList & value)
{
	doEmitMessage(message, value);
}

void QtSocketIoClient::emitMessage(const QString & message, const valueList & value, event_listener const & func)
{
	int id = doEmitMessage(message, value);
	m_messageCallbacks[id] = func;
}

void QtSocketIoClient::onWsError(QAbstractSocket::SocketError error)
{
	RTC_LOG(LS_INFO) << " Error occurred: " << (int)error;
	emit errorReceived(" Error occurred: " + error);
}

void QtSocketIoClient::onWsMessage(QString textMessage)
{
	QString ss = textMessage.toUtf8();
	RTC_LOG(LS_INFO)<< "************************************" << QT_TO_STD(ss)/*.replace(QRegExp("\""), "")*/;
    parseMessage(textMessage);
}

void QtSocketIoClient::onPingTimer()
{
	(void)m_pWebSocket->sendTextMessage(QStringLiteral("2"));
}

void QtSocketIoClient::onPingTimeout()
{
	this->close();
	emit errorReceived("ping timeout!");
}

QString QtSocketIoClient::package(const QVariant &value)
{
	if (value.isNull() || !value.isValid())
	{
		return "null";
	}

	QJsonDocument document;

	if (value.canConvert<QVariantMap>())
	{
		document = QJsonDocument(QJsonObject::fromVariantMap(value.toMap()));
	}
	else if (value.canConvert<QVariantList>())
	{
		document = QJsonDocument(QJsonArray::fromVariantList(value.toList()));
	}
	else
	{
		QJsonArray ar;
		ar.append(QJsonValue::fromVariant(value));
		document = QJsonDocument(ar);
	}

	return QString::fromUtf8(document.toJson(QJsonDocument::Compact));
}

static int id = 0;
int QtSocketIoClient::doEmitMessage(const QString &name, const QString &message)
{
	const QString msg = QStringLiteral("%1%2%3[\"%4\",%5]")
		.arg(FRAME_MESSAGE)
		.arg(PACKET_EVENT)
		.arg(++id)
		.arg(name)
		.arg(message);

	QString ss = msg;
	RTC_LOG(LS_INFO) << "************************************" << QT_TO_STD(ss)/*.replace(QRegExp("\""), "")*/;
	(void)m_pWebSocket->sendTextMessage(msg);
	return id;
}

int QtSocketIoClient::doEmitMessage(const QString &name, const valueList &values)
{
	QString msg = QStringLiteral("%1%2%3[\"%4\"")
		.arg(FRAME_MESSAGE)
		.arg(PACKET_EVENT)
		.arg(++id)
		.arg(name);

	for each (auto var in values)
	{
		msg.append(",");
		msg.append(package(var));
	}
	msg.append("]");

	QString ss = msg;
	RTC_LOG(LS_INFO) << "************************************" << QT_TO_STD(ss)/*.replace(QRegExp("\""), "")*/;
	(void)m_pWebSocket->sendTextMessage(msg);
	return id;
}

void QtSocketIoClient::parseMessage(const QString &message)
{
	QRegExp regExp("(\\d)?(\\d)?(\\d+)?([\\s\\S]*)?$", Qt::CaseInsensitive, QRegExp::RegExp2);
	if (regExp.indexIn(message) != -1)
	{
		QStringList captured = regExp.capturedTexts();
		FRAME_TYPE eFrameType = (FRAME_TYPE)captured.at(1).toInt();
		PACKET_TYPE ePacketType = (PACKET_TYPE)captured.at(2).toInt();
		int messageId = captured.at(3).toInt();
		QString message = captured.at(4);

		switch (eFrameType)
		{
		case FRAME_OPEN:
			this->onHandshake(message);
			break;
		case FRAME_CLOSE:
			this->close();
			emit disconnected("End by server");
			break;
		case FRAME_PONG:
			this->onPong();
			break;
		case FRAME_MESSAGE:
			this->onMessage(ePacketType, messageId, message);
			break;
		default:
			break;
		}
	}
}

void QtSocketIoClient::onHandshake(const QString &data)
{
	QJsonObject object;
	if (ConvertToJson(data, object))
	{
		__GET_VALUE_FROM_OBJ__(object, "sid", String, m_sid);
		__GET_VALUE_FROM_OBJ__(object, "pingInterval", Double, m_iPingInterval);
		__GET_VALUE_FROM_OBJ__(object, "pingTimeout", Double, m_iPingTimeout);

		if (!m_sid.isEmpty())
		{
			m_pPingTimer->start(m_iPingInterval);
			m_pPingTimeOutTimer->start(m_iPingTimeout);
			return;
		}
	}
	this->close();
	emit disconnected("Handshake error");
}

void QtSocketIoClient::onPong()
{
	//reset ping timeout timer
	m_pPingTimeOutTimer->stop();
	m_pPingTimeOutTimer->start();
}

void QtSocketIoClient::onMessage(const PACKET_TYPE ePacketType, const int messageId, const QString &message)
{
	switch (ePacketType)
	{
	case PACKET_CONNECT:
		//send msg buf
		emit connected();
		break;
	case PACKET_DISCONNECT:
		this->close();
		emit disconnected("PACKET_DISCONNECT");
		break;
	case PACKET_EVENT:
	case PACKET_BINARY_EVENT:
		onEvent(message);
		break;
	case PACKET_ACK:
	case PACKET_BINARY_ACK:
		onMsgAck(messageId, message);
		break;
	case PACKET_ERROR:
		break;
	default:
		break;
	}
}

void QtSocketIoClient::onEvent(const QString &message)
{
	QJsonArray array;
	if (ConvertToJson(message, array))
	{
		QString name = array.at(0).toString();

		auto itr = m_eventCallbacks.find(name);
		if (itr != m_eventCallbacks.end())
		{
			auto callback = itr.value();
			callback(message);
			return;
		}
		qWarning() << "Invalid event received: no name";
	}
}

void QtSocketIoClient::onMsgAck(const int messageId, const QString &message)
{
	auto itr = m_messageCallbacks.find(messageId);
	if (itr != m_messageCallbacks.end())
	{
		auto callback = itr.value();
		m_messageCallbacks.erase(itr);
		callback(message);
	}
}

void QtSocketIoClient::onError(const QString &message)
{
	emit errorReceived(message);
}

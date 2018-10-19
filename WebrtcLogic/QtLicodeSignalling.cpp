#include "stdafx.h"
#include "QtLicodeSignalling.h"
#include "ComFunction.h"
#include "HttpAPI/ServerApi.h"
using namespace std::placeholders;

QtLicodeSignalling::QtLicodeSignalling()
{
	QObject::connect(&_socket, SIGNAL(connected()), this, SLOT(on_socket_connected()));
	QObject::connect(&_socket, SIGNAL(disconnected(QString)), this, SLOT(on_socket_disconnected(QString)));
	QObject::connect(&_socket, SIGNAL(errorReceived(QString, QString)), this, SLOT(on_socket_errorReceived(QString, QString)));

	_socket.on("onAddStream", std::bind(&QtLicodeSignalling::onAddStream, this,std::placeholders::_1));
	_socket.on("signaling_message_erizo", std::bind(&QtLicodeSignalling::onSignalingMsgErizo, this, std::placeholders::_1));
	_socket.on("onRemoveStream", std::bind(&QtLicodeSignalling::onRemoveStream, this, std::placeholders::_1));
}

QtLicodeSignalling::~QtLicodeSignalling()
{
	DisconnectServer();
}

void QtLicodeSignalling::ConnectServer(const std::string& server)
{
	ServerHelper::s_serverUrl = STD_TO_QT(server);
	QString szContent = "{\"username\": \"user\", \"role\": \"presenter\"}";

	ServerHelper::request("createToken", szContent, std::bind(&QtLicodeSignalling::on_connectServer_response, this, _1, _2, _3));
}

void QtLicodeSignalling::on_connectServer_response(int errorCode, QString errorInfo, QByteArray bytes)
{
	QByteArray jsonString = QByteArray::fromBase64(bytes);

	QJsonParseError json_error;
	QJsonDocument parse_doucment = QJsonDocument::fromJson(jsonString, &json_error);
	if (json_error.error == QJsonParseError::NoError && parse_doucment.isObject())
	{
		QJsonObject obj = parse_doucment.object();

		__GET_VALUE_FROM_OBJ__(obj, "tokenId", String, this->tokenId);
		__GET_VALUE_FROM_OBJ__(obj, "host", String, this->host);
		__GET_VALUE_FROM_OBJ__(obj, "secure", Bool, this->secure);
		__GET_VALUE_FROM_OBJ__(obj, "signature", String, this->signature);

		this->connectWebsocket();
	}
}

void QtLicodeSignalling::DisconnectServer()
{
	const valueList values;
	_socket.emitMessage("disconnect", values);	
 	_socket.close();
}

bool QtLicodeSignalling::DisconnectPeer(qint64 id)
{
	return true;
}

void QtLicodeSignalling::connectWebsocket()
{
	QString url = secure ? "wss://" : "ws://";
	url += host;
	_socket.open(url);
}

// 1- Connect to Erizo-Controller
void QtLicodeSignalling::on_socket_connected()
{
	RTC_LOG(LS_INFO) << __FUNCTION__;
	//420["token",{"singlePC":false,"token":{"tokenId":"5af92e88ceeaf310459e1946","host":"192.168.1.118:8080","secure":true,"signature":"NmI5NTA0YmVkZGZkN2E3ZDExODY0OGIwMGUxYTRiYjFkYjRkNTljZg=="}}]
	QJsonObject obj;
	obj["singlePC"] = false;

	QJsonObject token;
	token["tokenId"] = tokenId;
	token["host"] = host;
	token["secure"] = secure;
	token["signature"] = signature;
	obj["token"] = token;

	_socket.emitMessage("token", obj, std::bind(&QtLicodeSignalling::onRoomConnectResult, this, std::placeholders::_1));
}

void QtLicodeSignalling::on_socket_disconnected(QString endpoint)
{
	RTC_LOG(LS_INFO) << QT_TO_STD(endpoint);
}

void QtLicodeSignalling::on_socket_errorReceived(QString reason, QString advice)
{
	RTC_LOG(LS_INFO) << "Error received:" << QT_TO_STD(reason) << "(advice" << QT_TO_STD(advice) << ")";
}

void QtLicodeSignalling::onRoomConnectResult(QString msg)
{
	QJsonArray arrs;
	if (ConvertToJson(msg, arrs))
	{
		QString result = arrs.at(0).toString();
		if (result == "success")
		{
			if (arrs.at(1).isObject())
			{
				QJsonObject obj = arrs.at(1).toObject();

				QJsonArray streams;
				QString clientId;
				double defaultVideoBW;
				double maxVideoBW;
				QJsonArray iceServers;

				__GET_VALUE_FROM_OBJ__(obj, "streams", Array, streams);
				__GET_VALUE_FROM_OBJ__(obj, "id", String, _roomid);
				__GET_VALUE_FROM_OBJ__(obj, "clientId", String, clientId);
				__GET_VALUE_FROM_OBJ__(obj, "defaultVideoBW", Double, defaultVideoBW);
				__GET_VALUE_FROM_OBJ__(obj, "maxVideoBW", Double, maxVideoBW);
				__GET_VALUE_FROM_OBJ__(obj, "iceServers", Array, iceServers);

				for (const auto ice :  iceServers)
				{
					QString iceUrl;
					__GET_VALUE_FROM_OBJ__(ice.toObject(), "url", String, iceUrl);

					// 2- Retrieve list of streams
				}

				// 3- Update RoomID

				// 4- publish
				publish();

				// 5- subscribeToStreams need fix
				for (const auto stream : streams)
				{
					if (stream.isObject())
					{
						QJsonObject streamObj = stream.toObject();

						qint64 id = 0;
						bool video = false;
						__GET_VALUE_FROM_OBJ__(streamObj, "id", Double, id);
						__GET_VALUE_FROM_OBJ__(streamObj, "video", Bool, video);

						if (id > 0 && video)
						{
							Subscribe(id);
						}
					}
				}
			}
		}
		else
		{

		}
	}
}

void QtLicodeSignalling::publish()
{
	QJsonObject obj;
	obj["state"] = "erizo";
	obj["data"] = true;
	obj["audio"] = true;
	obj["video"] = true;
	obj["screen"] = "";
	QJsonObject metadata;
	metadata["type"] = "publisher";
	obj["metadata"] = metadata;

	QJsonObject muteStream;
	muteStream["audio"] = false;
	muteStream["video"] = false;
	obj["muteStream"] = muteStream;

	obj["minVideoBW"] = 0;
	obj["label"] = "stream_label";
	obj["maxVideoBW"] = 300;

	valueList list;
	QVariant sdp;
	list.push_back(obj);
	list.push_back(sdp);

	_socket.emitMessage("publish", list, std::bind(&QtLicodeSignalling::onPublishResult, this, std::placeholders::_1));
}

void QtLicodeSignalling::onPublishResult(QString msg)
{
	QJsonArray arrs;
	if (ConvertToJson(msg, arrs))
	{
		_streamId = arrs.at(0).toDouble();
		emit signal_PeerConnected(_streamId, false);
	}	
}

void QtLicodeSignalling::Subscribe(qint64 streamId)
{
// {
// 	"streamId": 652514446341312000,
// 	"audio": true,
// 	"video": true,
// 	"data": true,
// 	"metadata": {
// 		"type": "subscriber"
// 	},
// 	"muteStream": {
// 		"audio": false,
// 		"video": false
// 	},
// 	"slideShowMode": false
// }
	QJsonObject obj;
	obj["streamId"] = streamId;
	obj["audio"] = true;
	obj["video"] = true;
	obj["maxVideoBW"] = 300;
	obj["data"] = true;
	obj["browser"] = "chrome-stable";

	QJsonObject metadata;
	metadata["type"] = "subscriber";
	obj["metadata"] = metadata;

	QJsonObject muteStream;
	muteStream["audio"] = false;
	muteStream["video"] = false;
	obj["muteStream"] = muteStream;

	obj["slideShowMode"] = false;

	valueList list;
	QVariant sdp_;
	list.push_back(obj);
	list.push_back(sdp_);

	_socket.emitMessage("Subscribe", list, std::bind(&QtLicodeSignalling::onSubscribe, this, streamId, std::placeholders::_1));
}

void QtLicodeSignalling::onSubscribe(qint64 streamId, QString msg)
{
	QJsonArray arrs;
	if (ConvertToJson(msg, arrs))
	{
		bool success = arrs.at(0).toBool();
		if (success)
		{
			emit signal_PeerConnected(streamId, true);
		}
		else
		{

		}
	}
}

void QtLicodeSignalling::onAddStream(QString message)
{
	//{"id":652514446341312000,"audio":true,"video":true,"data":true,"screen":""}"

	QJsonObject obj;
	if (!getEventObj(message, obj))
	{
		return;
	}

	qint64 streamId;
	bool audio;
	bool video;
	bool data;
	QString screen;

	__GET_VALUE_FROM_OBJ__(obj, "id", Double, streamId);
	__GET_VALUE_FROM_OBJ__(obj, "audio", Bool, audio);
	__GET_VALUE_FROM_OBJ__(obj, "video", Bool, video);
	__GET_VALUE_FROM_OBJ__(obj, "data", Bool, data);
	__GET_VALUE_FROM_OBJ__(obj, "screen", String, screen);

	if (streamId != _streamId)
	{
		//subscribeToStreams
		Subscribe(streamId);
	}
}

void QtLicodeSignalling::onSignalingMsgErizo(QString message)
{
	//{"mess":{"type":"answer","sdp":"sdp..."},"streamId":652514446341312000}]"

	QJsonObject obj;
	if (!getEventObj(message, obj))
	{
		return;
	}

	qint64 streamId = 0;
	qint64 peerId = 0;
	QJsonObject mess;
	QString type;

	__GET_VALUE_FROM_OBJ__(obj, "mess", Object, mess);
	__GET_VALUE_FROM_OBJ__(obj, "streamId", Double, streamId);
	__GET_VALUE_FROM_OBJ__(mess, "type", String, type);
 	__GET_VALUE_FROM_OBJ__(obj, "peerId", Double, peerId);

	if (type == "initializing")
	{
	}
	else if (type == "started")
	{
		emit signal_StreamStarted(streamId + peerId);
	}
	else if (type == "answer")
	{
		QString sdp;
		__GET_VALUE_FROM_OBJ__(mess, "sdp", String, sdp);
		emit signal_RetmoeSDP(streamId + peerId, type, sdp);
	}
	else if (type == "candidate")
	{
// 			emit signal_RetmoeIce();
	}
}

void QtLicodeSignalling::onRemoveStream(QString message)
{
	QJsonObject obj;
	if (!getEventObj(message, obj))
	{
		return;
	}
	qint64 streamId;
	__GET_VALUE_FROM_OBJ__(obj, "id", Double, streamId);
	emit signal_PeerDisconnected(streamId);
}

void QtLicodeSignalling::SendSDP(qint64 id, QString sdp, QString type)
{
	QJsonObject obj;
	obj["streamId"] = id;

	QJsonObject msg;
	msg["type"] = type;
	msg["sdp"] = sdp;

	QJsonObject config;
	config["maxVideoBW"] = 300;
	msg["config"] = config;

	obj["msg"] = msg;
	obj["browser"] = "chrome-stable";


	valueList list;
	QVariant sdp_;
	list.push_back(obj);
	list.push_back(sdp_);

	_socket.emitMessage("signaling_message", list);
}

void QtLicodeSignalling::SendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate)
{
	QJsonObject obj;
	obj["streamId"] = id;

	QJsonObject msg;
	msg["type"] = "candidate";

	QJsonObject candidate_;
	candidate_["sdpMLineIndex"] = sdpMLineIndex;
	candidate_["sdpMid"] = sdpMid;
	candidate_["candidate"] = candidate;
	msg["candidate"] = candidate_;
	obj["msg"] = msg;
	obj["browser"] = "chrome-stable";

	valueList list;
	QVariant sdp_;
	list.push_back(obj);
	list.push_back(sdp_);

	_socket.emitMessage("signaling_message", list);
}

bool QtLicodeSignalling::getEventObj(QString &message, QJsonObject &obj)
{
	QJsonArray arrs;
	if (!ConvertToJson(message, arrs)
		|| arrs.size() < 2)
	{
		return false;
	}

	QJsonValue signaling = arrs.at(1);
	if (!signaling.isObject())
	{
		return false;
	}

	obj = signaling.toObject();
	return true;
}

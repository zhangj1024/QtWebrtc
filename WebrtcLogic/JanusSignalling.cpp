#include "stdafx.h"
#include "JanusSignalling.h"
#include "ComFunction.h"
#include "HttpAPI/ServerApi.h"
using namespace std::placeholders;

#define JANUS "janus"

JanusSignalling::JanusSignalling(QObject *parent)
	: QObject(parent)
{
	QObject::connect(&_socket, SIGNAL(connected()), this, SLOT(on_socket_connected()));
	QObject::connect(&_socket, SIGNAL(disconnected(QString)), this, SLOT(on_socket_disconnected(QString)));
	QObject::connect(&_socket, SIGNAL(errorReceived(QString, QString)), this, SLOT(on_socket_errorReceived(QString, QString)));

// 	_socket.setNotifyCallBack("webrtcup", std::bind(&JanusSignalling::onAddStream, this, std::placeholders::_1));
// 	_socket.setNotifyCallBack("media", std::bind(&JanusSignalling::onAddStream, this, std::placeholders::_1));

	_socket.setEventCallBack("joined", std::bind(&JanusSignalling::OnEventJoin, this, std::placeholders::_1));
	_socket.setEventCallBack("event", std::bind(&JanusSignalling::OnEventRemoteSDP, this, std::placeholders::_1));
}

JanusSignalling::~JanusSignalling()
{
	DisconnectServer();
}

void JanusSignalling::ConnectServer(const std::string& server)
{
	_socket.open(STD_TO_QT(server));
}

void JanusSignalling::DisconnectServer()
{
 	_socket.close();
}

bool JanusSignalling::DisconnectPeer(qint64 id)
{
	return true;
}

void JanusSignalling::on_socket_connected()
{
	qDebug()<< __FUNCTION__;
	emit signal_SignedIn();
	CreateSessionId();
}

void JanusSignalling::on_socket_disconnected(QString endpoint)
{
	emit signal_Disconnected();
	qDebug()<< QT_TO_STD(endpoint);
}

void JanusSignalling::on_socket_errorReceived(QString reason, QString advice)
{
	qDebug()<< "Error received:" << reason << "(advice" << advice << ")";
}

void JanusSignalling::CreateSessionId()
{
	//create sessionid
	QJsonObject msg;
	msg[JANUS] = "create";

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnCreateSessionId, this, std::placeholders::_1));
}

void JanusSignalling::OnCreateSessionId(const QJsonObject &recvdata)
{
	__GET_VALUE_FROM_OBJ__(recvdata, "id", Double, _sessionId);
	_socket.setSessionId(_sessionId);

	AttachVideoRoom();
}

void JanusSignalling::AttachVideoRoom()
{
	// "janus":"attach", "plugin" : "janus.plugin.videoroom", "opaque_id" : "videoroomtest-iPrNflZMPxbT", "transaction" : "nAvaOPWn0znH", "session_id" : 4533699291890900

	QJsonObject msg;
	msg[JANUS] = "attach";
	msg["plugin"] = "janus.plugin.videoroom";
	msg["opaque_id"] = "videoroomtest-" + GetRandomString(12);

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnAttachVideoRoom, this, std::placeholders::_1));
}

void JanusSignalling::OnAttachVideoRoom(const QJsonObject &recvdata)
{
	qint64 id = 0;
	__GET_VALUE_FROM_OBJ__(recvdata, "id", Double, id);
	Join(id);
}

void JanusSignalling::Join(qint64 id)
{
	//"janus":"message", 
	//"body" : {"request":"join", "room" : 1234, "ptype" : "publisher", "display" : "123"}, 
	//"transaction" : "XE214w6N7t3g", "session_id" : 4533699291890900, "handle_id" : 6100401093801995

	QJsonObject msg;
	msg[JANUS] = "message";
	msg["handle_id"] = id;

	QJsonObject body;
	body["request"] = "join";
	body["room"] = 1234;
	body["ptype"] = "publisher";
	body["display"] = "zhangjian";

	msg["body"] = body;

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnJoin, this, std::placeholders::_1, id));
}

void JanusSignalling::OnJoin(const QJsonObject &recvdata, qint64 id)
{
	emit signal_PeerConnected(id, false);
}

void JanusSignalling::OnEventJoin(const QJsonObject &recvdata)
{
	//{
	//	"janus": "event",
	//	"session_id": 4533699291890900,
	//	"transaction": "XE214w6N7t3g",
	//	"sender": 6100401093801995,
	//	"plugindata": {
	//		"plugin": "janus.plugin.videoroom",
	//		"data": {
	//			"videoroom": "joined",
	//			"room": 1234,
	//			"description": "Demo Room",
	//			"id": 6742718672565716,
	//			"private_id": 2067637147,
	//			"publishers": []
	//		}
	//	}
	//}

	return;

	QJsonArray publishsObj;
	__GET_VALUE_FROM_OBJ__(recvdata, "publishers", Array, publishsObj);

	// subscribeToStreams
	for (const auto stream : publishsObj)
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

void JanusSignalling::Subscribe(qint64 streamId)
{
// 	QJsonObject obj;
// 	obj["streamId"] = streamId;
// 	obj["audio"] = true;
// 	obj["video"] = true;
// 	obj["maxVideoBW"] = 300;
// 	obj["data"] = true;
// 	obj["browser"] = "chrome-stable";
// 
// 	QJsonObject metadata;
// 	metadata["type"] = "subscriber";
// 	obj["metadata"] = metadata;
// 
// 	QJsonObject muteStream;
// 	muteStream["audio"] = false;
// 	muteStream["video"] = false;
// 	obj["muteStream"] = muteStream;
// 
// 	obj["slideShowMode"] = false;
// 
// 	valueList list;
// 	QVariant sdp_;
// 	list.push_back(obj);
// 	list.push_back(sdp_);
// 
// 	_socket.emitMessage("Subscribe", list, std::bind(&JanusSignalling::onSubscribe, this, streamId, std::placeholders::_1));
}

void JanusSignalling::onSubscribe(qint64 streamId, QString msg)
{
// 	QJsonArray arrs;
// 	if (ConvertToJson(msg, arrs))
// 	{
// 		bool success = arrs.at(0).toBool();
// 		if (success)
// 		{
// 			emit signal_PeerConnected(streamId, true);
// 		}
// 		else
// 		{
// 
// 		}
// 	}
}

// void JanusSignalling::onAddStream(QString message)
// {
// 	//{"id":652514446341312000,"audio":true,"video":true,"data":true,"screen":""}"
// 
// 	QJsonObject obj;
// 	if (!getEventObj(message, obj))
// 	{
// 		return;
// 	}
// 
// 	qint64 streamId;
// 	bool audio;
// 	bool video;
// 	bool data;
// 	QString screen;
// 
// 	__GET_VALUE_FROM_OBJ__(obj, "id", Double, streamId);
// 	__GET_VALUE_FROM_OBJ__(obj, "audio", Bool, audio);
// 	__GET_VALUE_FROM_OBJ__(obj, "video", Bool, video);
// 	__GET_VALUE_FROM_OBJ__(obj, "data", Bool, data);
// 	__GET_VALUE_FROM_OBJ__(obj, "screen", String, screen);
// 
// 	if (streamId != _streamId)
// 	{
// 		//subscribeToStreams
// 		Subscribe(streamId);
// 	}
// }
// 
// void JanusSignalling::onSignalingMsgErizo(QString message)
// {
// 	//{"mess":{"type":"answer","sdp":"sdp..."},"streamId":652514446341312000}]"
// 
// 	QJsonObject obj;
// 	if (!getEventObj(message, obj))
// 	{
// 		return;
// 	}
// 
// 	qint64 streamId = 0;
// 	qint64 peerId = 0;
// 	QJsonObject mess;
// 	QString type;
// 
// 	__GET_VALUE_FROM_OBJ__(obj, "mess", Object, mess);
// 	__GET_VALUE_FROM_OBJ__(obj, "streamId", Double, streamId);
// 	__GET_VALUE_FROM_OBJ__(mess, "type", String, type);
//  	__GET_VALUE_FROM_OBJ__(obj, "peerId", Double, peerId);
// 
// 	if (type == "initializing")
// 	{
// 	}
// 	else if (type == "started")
// 	{
// 		emit signal_StreamStarted(streamId + peerId);
// 	}
// 	else if (type == "answer")
// 	{
// 		QString sdp;
// 		__GET_VALUE_FROM_OBJ__(mess, "sdp", String, sdp);
// 		emit retmoeSDP(streamId + peerId, type, sdp);
// 	}
// 	else if (type == "candidate")
// 	{
// // 			emit retmoeIce();
// 	}
// }
// 
// void JanusSignalling::onRemoveStream(QString message)
// {
// 	QJsonObject obj;
// 	if (!getEventObj(message, obj))
// 	{
// 		return;
// 	}
// 	qint64 streamId;
// 	__GET_VALUE_FROM_OBJ__(obj, "id", Double, streamId);
// 	emit signal_PeerDisconnected(streamId);
// }

void JanusSignalling::SendSDP(qint64 id, QString sdp, QString type)
{
	//{
	//	"janus": "message",
	//	"body": {
	//		"request": "configure",
	//		"audio": true,
	//		"video": true
	//	},
	//	"transaction": "kXdw7wm3NKFS",
	//	"jsep": {
	//		"type": "offer",
	//		"sdp": "sdp"
	//	},
	//	"session_id": 4533699291890900,
	//	"handle_id": 6100401093801995
	//}

	QJsonObject msg;
	msg[JANUS] = "message";

	QJsonObject body;
	body["request"] = "configure";
	body["audio"] = true;
	body["video"] = true;
	msg["body"] = body;

	QJsonObject jsep;
	jsep["type"] = type;
	jsep["sdp"] = sdp;
	msg["jsep"] = jsep;

	msg["handle_id"] = id;

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnSendSDP, this, std::placeholders::_1));
}

void JanusSignalling::OnSendSDP(const QJsonObject &recvdata)
{

}

void JanusSignalling::OnEventRemoteSDP(const QJsonObject &recvdata)
{
	//{
	//	"janus": "event",
	//	"session_id": 4533699291890900,
	//	"transaction": "kXdw7wm3NKFS",
	//	"sender": 6100401093801995,
	//	"plugindata": {
	//		"plugin": "janus.plugin.videoroom",
	//		"data": {
	//			"videoroom": "event",
	//			"room": 1234,
	//			"configured": "ok",
	//			"audio_codec": "opus",
	//			"video_codec": "vp8"
	//		}
	//	},
	//	"jsep": {
	//		"type": "answer",
	//		"sdp": "sdp"
	//	}
	//}

	qint64 peerId;
	__GET_VALUE_FROM_OBJ__(recvdata, "sender", Double, peerId);


	QJsonObject jsepObj;
	__GET_VALUE_FROM_OBJ__(recvdata, "jsep", Object, jsepObj);

	QString type;
	QString sdp;
	__GET_VALUE_FROM_OBJ__(jsepObj, "type", String, type);
	__GET_VALUE_FROM_OBJ__(jsepObj, "sdp", String, sdp);

	emit retmoeSDP(peerId, type, sdp);
}

void JanusSignalling::SendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate)
{
	//{
	//	"janus": "trickle",
	//	"candidate": {
	//		"candidate": "candidate:0 1 UDP 2122252543 192.168.1.11 60472 typ host",
	//		"sdpMid": "sdparta_0",
	//		"sdpMLineIndex": 0
	//	},
	//	"transaction": "DWFhXZlzHxGO",
	//	"session_id": 4533699291890900,
	//	"handle_id": 6100401093801995
	//}
	QJsonObject msg;
	msg[JANUS] = "trickle";

	QJsonObject candidateObj;
	if (sdpMid == "end")
	{
		candidateObj["completed"] = true;
	}
	else
	{
		candidateObj["candidate"] = candidate;
		candidateObj["sdpMid"] = sdpMid;
		candidateObj["sdpMLineIndex"] = sdpMLineIndex;
	}
	msg["candidate"] = candidateObj;

	msg["handle_id"] = id;

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnSendCandidate, this, std::placeholders::_1));
}

void JanusSignalling::OnSendCandidate(const QJsonObject &recvmsg)
{

}

void JanusSignalling::SendCandidateCompleted(qint64 id)
{
	//{
	//	"janus": "trickle",
	//	"candidate": {
	//		"completed": true
	//	},
	//	"transaction": "guq9L0gLsZQa",
	//	"session_id": 4533699291890900,
	//	"handle_id": 6100401093801995
	//}

	QJsonObject msg;
	msg[JANUS] = "trickle";

	QJsonObject candidateObj;
	candidateObj["completed"] = true;
	msg["candidate"] = candidateObj;

	msg["handle_id"] = id;

	_socket.emitMessage(msg, std::bind(&JanusSignalling::OnSendCandidate, this, std::placeholders::_1));
}


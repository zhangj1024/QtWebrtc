#include "stdafx.h"
#include "JanusPeerconnection.h"

JanusPeerconnection::JanusPeerconnection(JanusWebsocket &socket, QString opaqueId, QObject *parent)
	: _socket(socket), _opaqueId(opaqueId), QObject(parent)
{
}

JanusPeerconnection::~JanusPeerconnection()
{
	DetachVideoRoom();
}

void JanusPeerconnection::DetachVideoRoom()
{
	// {"janus":"detach","transaction":"Nzw8frzaaryM","session_id":6427402872492965,"handle_id":3117642474465747}

	QJsonObject msg;
	msg[JANUS] = "detach";
	msg["handle_id"] = _handleId;

	_socket.emitMessage(msg, std::bind(&JanusPeerconnection::OnDetachVideoRoom, this, std::placeholders::_1));
}

void JanusPeerconnection::OnDetachVideoRoom(const QJsonObject &recvdata)
{

}

void JanusPeerconnection::AttachVideoRoom()
{
	// "janus":"attach", "plugin" : "janus.plugin.videoroom", "opaque_id" : "videoroomtest-iPrNflZMPxbT", "transaction" : "nAvaOPWn0znH", "session_id" : 4533699291890900

	QJsonObject msg;
	msg[JANUS] = "attach";
	msg["plugin"] = "janus.plugin.videoroom";
	msg["opaque_id"] = _opaqueId;

	_socket.emitMessage(msg, std::bind(&JanusPeerconnection::OnAttachVideoRoom, this, std::placeholders::_1));
}

void JanusPeerconnection::OnAttachVideoRoom(const QJsonObject &recvdata)
{
	__GET_VALUE_FROM_OBJ__(recvdata, "id", Double, _handleId);
	Join();
}

void JanusPeerconnection::Join()
{
	//"janus":"message", 
	//"body" : {"request":"join", "room" : 1234, "ptype" : "publisher", "display" : "123"}, 
	//"transaction" : "XE214w6N7t3g", "session_id" : 4533699291890900, "handle_id" : 6100401093801995

	QJsonObject msg;
	msg[JANUS] = "message";
	msg["handle_id"] = _handleId;

	QJsonObject body;
	body["request"] = "join";
	body["room"] = 1234;
	if (_subscribeId == 0)
	{
		body["ptype"] = "publisher";
		body["display"] = "zhangjian";
	}
	else
	{
		//"body":{"request":"join","room":1234,"ptype":"subscriber","feed":8452751313644181,"private_id":2775113861}
		body["ptype"] = "subscriber";
		body["feed"] = _subscribeId;
		body["private_id"] = _privateId;
	}

	msg["body"] = body;

	_socket.emitMessage(msg, std::bind(&JanusPeerconnection::OnJoin, this, std::placeholders::_1));
}

void JanusPeerconnection::OnJoin(const QJsonObject &recvdata)
{
// 	__GET_VALUE_FROM_OBJ__(recvdata, "private_id", Double, _privateId);
// 	emit signal_PeerConnected(_handleId, false);
}

void JanusPeerconnection::SendSDP(QString sdp, QString type)
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
	if (_subscribeId == 0)
	{
		body["request"] = "configure";
		body["audio"] = true;
		body["video"] = true;
	}
	else
	{
		body["request"] = "start";
		body["room"] = 1234;
	}
	msg["body"] = body;

	QJsonObject jsep;
	jsep["type"] = type;
	jsep["sdp"] = sdp;
	msg["jsep"] = jsep;

	msg["handle_id"] = _handleId;

	_socket.emitMessage(msg, std::bind(&JanusPeerconnection::OnSendSDP, this, std::placeholders::_1));
}

void JanusPeerconnection::OnSendSDP(const QJsonObject &recvdata)
{

}

void JanusPeerconnection::SendCandidate(QString sdpMid, int sdpMLineIndex, QString candidate)
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

	msg["handle_id"] = _handleId;

	_socket.emitMessage(msg, std::bind(&JanusPeerconnection::OnSendCandidate, this, std::placeholders::_1));
}

void JanusPeerconnection::OnSendCandidate(const QJsonObject &recvmsg)
{

}

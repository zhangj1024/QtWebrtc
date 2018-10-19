#include "stdafx.h"
#include "JanusVideoRoomManager.h"

JanusVideoRoomManager::JanusVideoRoomManager(QObject *parent)
	: QObject(parent)
{
	QObject::connect(&_socket, SIGNAL(connected()), this, SLOT(on_socket_connected()));
	QObject::connect(&_socket, SIGNAL(disconnected()), this, SLOT(on_socket_disconnected()));
	QObject::connect(&_socket, SIGNAL(errorReceived(QString)), this, SIGNAL(signal_Error(QString)));

	// 	_socket.setNotifyCallBack("webrtcup", std::bind(&JanusSignalling::onAddStream, this, std::placeholders::_1));
	// 	_socket.setNotifyCallBack("media", std::bind(&JanusSignalling::onAddStream, this, std::placeholders::_1));

	_socket.setEventCallBack("joined", std::bind(&JanusVideoRoomManager::OnEventJoin, this, std::placeholders::_1));
	_socket.setEventCallBack("event", std::bind(&JanusVideoRoomManager::OnEventEvent, this, std::placeholders::_1));
	_socket.setEventCallBack("attached", std::bind(&JanusVideoRoomManager::OnEventAttached, this, std::placeholders::_1));

	_opaqueId = "videoroomtest-" + GetRandomString(12);
}

JanusVideoRoomManager::~JanusVideoRoomManager()
{
	DisconnectServer();
}

void JanusVideoRoomManager::on_socket_connected()
{
	qDebug() << __FUNCTION__;
	CreateSessionId();
}

void JanusVideoRoomManager::on_socket_disconnected()
{
	emit signal_Logout();
	qDebug() << __FUNCTION__;
}

void JanusVideoRoomManager::ConnectServer(const std::string& server)
{
	_socket.open(STD_TO_QT(server));
}

void JanusVideoRoomManager::DisconnectServer()
{
	_socket.close();
}

void JanusVideoRoomManager::CreateSessionId()
{
	//create sessionid
	QJsonObject msg;
	msg[JANUS] = "create";

	_socket.emitMessage(msg, std::bind(&JanusVideoRoomManager::OnCreateSessionId, this, std::placeholders::_1));
}

void JanusVideoRoomManager::OnCreateSessionId(const QJsonObject &recvdata)
{
	__GET_VALUE_FROM_OBJ__(recvdata, "id", Double, _sessionId);
	_socket.setSessionId(_sessionId);

	emit signal_Login();
}

void JanusVideoRoomManager::Join()
{
	JanusPeerconnection *peer = new JanusPeerconnection(_socket, _opaqueId, this);
	peers.push_back(peer);

	peer->AttachVideoRoom();
}

bool JanusVideoRoomManager::DisconnectPeer(qint64 peer_id)
{
	return true;
}

QJsonObject GetDataObj(const QJsonObject &recvdata)
{
	QJsonObject plugindataOjb;
	__GET_VALUE_FROM_OBJ__(recvdata, "plugindata", Object, plugindataOjb);

	QJsonObject dataOjb;
	__GET_VALUE_FROM_OBJ__(plugindataOjb, "data", Object, dataOjb);

	return dataOjb;
}

void JanusVideoRoomManager::OnEventJoin(const QJsonObject &recvdata)
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

	//joined
	QJsonObject dataOjb = GetDataObj(recvdata);

	__GET_VALUE_FROM_OBJ__(dataOjb, "private_id", Double, _privateId);

	qint64 sender = 0;
	__GET_VALUE_FROM_OBJ__(recvdata, "sender", Double, sender);

	for (auto peer : peers)
	{
		if (peer->GetHandleId() == sender)
		{
			//start QtWebrtcRemoteStream
			emit signal_PeerConnected(sender, false, true);
			break;
		}
	}

	OnEventPublishers(recvdata);
}

void JanusVideoRoomManager::OnEventEvent(const QJsonObject &recvdata)
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

	OnEventSdp(recvdata);
	OnEventPublishers(recvdata);
	OnEventUnpublish(recvdata);
}

void JanusVideoRoomManager::OnEventAttached(const QJsonObject &recvdata)
{
	//{
	//   "janus": "event",
	//   "session_id": 7946665922395538,
	//   "transaction": "ZgJYJBFpm4PF",
	//   "sender": 2403299073732330,
	//   "plugindata": {
	//      "plugin": "janus.plugin.videoroom",
	//      "data": {
	//         "videoroom": "attached",
	//         "room": 1234,
	//         "id": 8452751313644181,
	//         "display": "2"
	//      }
	//   },
	//   "jsep": {
	//      "type": "offer",
	//      "sdp": "sdpdata"
	//   }
	//}

	OnEventSdp(recvdata);
}

void JanusVideoRoomManager::OnEventSdp(const QJsonObject &recvdata)
{
	//   "jsep": {
	//      "type": "offer",
	//      "sdp": "sdpdata"
	//   }

	qint64 sender = 0;
	__GET_VALUE_FROM_OBJ__(recvdata, "sender", Double, sender);

	QJsonObject jsepObj;
	__GET_VALUE_FROM_OBJ__(recvdata, "jsep", Object, jsepObj);

	QString type;
	QString sdp;
	__GET_VALUE_FROM_OBJ__(jsepObj, "type", String, type);
	__GET_VALUE_FROM_OBJ__(jsepObj, "sdp", String, sdp);

	if (!sdp.isEmpty())
	{
		emit signal_RetmoeSDP(sender, type, sdp);
	}
}

void JanusVideoRoomManager::OnEventPublishers(const QJsonObject &recvdata)
{
	//"publishers": [
	//	{
	//	   "id": 8452751313644181,
	//	   "display": "2",
	//	   "audio_codec": "opus",
	//	   "video_codec": "vp8"
	//	}
	//]

	QJsonObject dataOjb = GetDataObj(recvdata);

	QJsonArray publishersArr;
	__GET_VALUE_FROM_OBJ__(dataOjb, "publishers", Array, publishersArr);

	for (const auto publishersRef : publishersArr)
	{
		if (publishersRef.isObject())
		{
			QJsonObject publishObj = publishersRef.toObject();

			qint64 id = 0;
			QString display;
			__GET_VALUE_FROM_OBJ__(publishObj, "id", Double, id);
			__GET_VALUE_FROM_OBJ__(publishObj, "display", String, display);

			if (id > 0)
			{
				//subscribe
				JanusPeerconnection *peer = new JanusPeerconnection(_socket, _opaqueId, this);
				peers.push_back(peer);

				peer->SetSubscribe(id);
				peer->SetPrivateId(_privateId);
				peer->AttachVideoRoom();
			}
		}
	}
}

void JanusVideoRoomManager::OnEventUnpublish(const QJsonObject &recvdata)
{
	//{
	//   "janus": "event",
	//   "session_id": 2858344603551347,
	//   "sender": 6562380401450469,
	//   "plugindata": {
	//      "plugin": "janus.plugin.videoroom",
	//      "data": {
	//         "videoroom": "event",
	//         "room": 1234,
	//         "unpublished": 2593896363845411
	//      }
	//   }
	//}


	QJsonObject dataOjb = GetDataObj(recvdata);

	qint64 unpublishId = 0;
	__GET_VALUE_FROM_OBJ__(dataOjb, "unpublished", Double, unpublishId);

	if (unpublishId != 0)
	{
		for (auto itr = peers.begin(); itr != peers.end(); itr++)
		{
			if ((*itr)->GetSubscribe() == unpublishId)
			{
				emit signal_PeerDisconnected((*itr)->GetHandleId());
				peers.erase(itr);
				break;
			}
		}
	}
}


void JanusVideoRoomManager::SendSDP(qint64 &id, QString &sdp, QString &type)
{
	for (auto peer : peers)
	{
		if (peer->GetHandleId() == id)
		{
			peer->SendSDP(sdp, type);
			break;
		}
	}
}

void JanusVideoRoomManager::SendCandidate(qint64 &id, QString &sdpMid, int &sdpMLineIndex, QString &candidate)
{
	for (auto peer : peers)
	{
		if (peer->GetHandleId() == id)
		{
			peer->SendCandidate(sdpMid, sdpMLineIndex, candidate);
			break;
		}
	}
}

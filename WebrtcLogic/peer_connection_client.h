/*
 *  Copyright 2011 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#pragma once
#include <map>
#include <memory>
#include <string>

#include "rtc_base/nethelpers.h"
#include "rtc_base/physicalsocketserver.h"
#include "rtc_base/signalthread.h"
#include "rtc_base/third_party/sigslot/sigslot.h"
#include <QObject>

typedef std::map<qint64, std::string> Peers;

class PeerConnectionClient :
	public QObject,
	public sigslot::has_slots<> {

	Q_OBJECT

public:
	enum State {
		NOT_CONNECTED,
		RESOLVING,
		SIGNING_IN,
		CONNECTED,
		SIGNING_OUT_WAITING,
		SIGNING_OUT,
	};

	PeerConnectionClient();
	~PeerConnectionClient();

	void ConnectServer(const std::string& server);
	void DisconnectServer();
	bool DisconnectPeer(qint64 peer_id);

public slots:
	void SendSDP(qint64 id, QString sdp, QString type);
	void SendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate);

Q_SIGNALS:
	void signal_SignedIn();  // Called when we're logged on.
 	void signal_Disconnected();
	void signal_PeerConnected(qint64 id, bool show, bool connect);
	void signal_PeerDisconnected(qint64 id);
	void signal_ServerConnectionFailure();

	void signal_StreamStarted(qint64 id);

	void signal_RetmoeIce(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void signal_RetmoeSDP(qint64 id, QString type, QString sdp);

protected:
	bool is_connected() const;
	bool IsSendingMessage();
	void SendMsg(qint64 peer_id, std::string *message);
	void Connect(const std::string& server, int port,
		const std::string& client_name);
	qint64 id() const;
	const Peers& peers() const;
	bool SignOut();
	bool SendToPeer(qint64 peer_id, const std::string& message);


	void DoConnect();
	void Close();
	void InitSocketSignals();
	bool ConnectControlSocket();
	void OnConnect(rtc::AsyncSocket* socket);
	void OnHangingGetConnect(rtc::AsyncSocket* socket);
	void OnMessageFromPeer(qint64 peer_id, const std::string& message);
	void handlePeerMsg(qint64 peer_id, const std::string& message);

	// Quick and dirty support for parsing HTTP header values.
	bool GetHeaderValue(const std::string& data, size_t eoh,
		const char* header_pattern, size_t* value);

	bool GetHeaderValue(const std::string& data, size_t eoh,
		const char* header_pattern, std::string* value);

	// Returns true if the whole response has been read.
	bool ReadIntoBuffer(rtc::AsyncSocket* socket, std::string* data,
		size_t* content_length);

	void OnRead(rtc::AsyncSocket* socket);

	void OnHangingGetRead(rtc::AsyncSocket* socket);

	// Parses a single line entry in the form "<name>,<id>,<connected>"
	bool ParseEntry(const std::string& entry, std::string* name, qint64* id,
		bool* connected);

	int GetResponseStatus(const std::string& response);

	bool ParseServerResponse(const std::string& response, size_t content_length,
		size_t* peer_id, size_t* eoh);

	void OnClose(rtc::AsyncSocket* socket, int err);

	void OnResolveResult(rtc::AsyncResolverInterface* resolver);

	rtc::SocketAddress server_address_;
	rtc::AsyncResolver* resolver_;
	std::unique_ptr<rtc::AsyncSocket> control_socket_;
	std::unique_ptr<rtc::AsyncSocket> hanging_get_;
	std::string onconnect_data_;
	std::string control_data_;
	std::string notification_data_;
	std::string client_name_;
	Peers peers_;
	State state_;
	qint64 my_id_;

	std::deque<std::pair<qint64, std::string*> > pending_messages_;

	std::string server_;
};

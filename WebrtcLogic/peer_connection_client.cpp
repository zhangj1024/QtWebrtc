/*
 *  Copyright 2012 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "stdafx.h"
#include "peer_connection_client.h"

#include "defaults.h"
#include "rtc_base/checks.h"
#include "rtc_base/nethelpers.h"
#include "rtc_base/stringutils.h"
#include "rtc_base/json.h"

#ifdef WIN32
#include "rtc_base/win32socketserver.h"
#endif

const uint16_t kDefaultServerPort = 8888;

 // Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

using rtc::sprintfn;

namespace {

	// This is our magical hangup signal.
	const char kByeMessage[] = "BYE";
	// Delay between server connection retries, in milliseconds
	const int kReconnectDelay = 2000;

	rtc::AsyncSocket* CreateClientSocket(int family) {
#ifdef WIN32
		rtc::Win32Socket* sock = new rtc::Win32Socket();
		sock->CreateT(family, SOCK_STREAM);
		return sock;
#elif defined(WEBRTC_POSIX)
		rtc::Thread* thread = rtc::Thread::Current();
		RTC_DCHECK(thread != NULL);
		return thread->socketserver()->CreateAsyncSocket(family, SOCK_STREAM);
#else
#error Platform not supported.
#endif
	}

}  // namespace

PeerConnectionClient::PeerConnectionClient()
	: resolver_(NULL),
	state_(NOT_CONNECTED),
	my_id_(-1) {
}

PeerConnectionClient::~PeerConnectionClient() {
}

void PeerConnectionClient::InitSocketSignals() {
	RTC_DCHECK(control_socket_.get() != NULL);
	RTC_DCHECK(hanging_get_.get() != NULL);
	control_socket_->SignalCloseEvent.connect(this,
		&PeerConnectionClient::OnClose);
	hanging_get_->SignalCloseEvent.connect(this,
		&PeerConnectionClient::OnClose);
	control_socket_->SignalConnectEvent.connect(this,
		&PeerConnectionClient::OnConnect);
	hanging_get_->SignalConnectEvent.connect(this,
		&PeerConnectionClient::OnHangingGetConnect);
	control_socket_->SignalReadEvent.connect(this,
		&PeerConnectionClient::OnRead);
	hanging_get_->SignalReadEvent.connect(this,
		&PeerConnectionClient::OnHangingGetRead);
}

qint64 PeerConnectionClient::id() const {
	return my_id_;
}

bool PeerConnectionClient::is_connected() const {
	return my_id_ != -1;
}

const Peers& PeerConnectionClient::peers() const {
	return peers_;
}

void PeerConnectionClient::Connect(const std::string& server, int port,
	const std::string& client_name) {
	RTC_DCHECK(!server.empty());
	RTC_DCHECK(!client_name.empty());

	if (state_ != NOT_CONNECTED) {
		RTC_LOG(WARNING)
			<< "The client must not be connected before you can call Connect()";
		emit signal_ServerConnectionFailure();
		return;
	}

	if (server.empty() || client_name.empty()) {
		emit signal_ServerConnectionFailure();
		return;
	}

	if (port <= 0)
		port = kDefaultServerPort;

	server_address_.SetIP(server);
	server_address_.SetPort(port);
	client_name_ = client_name;

	if (server_address_.IsUnresolvedIP()) {
		state_ = RESOLVING;
		resolver_ = new rtc::AsyncResolver();
		resolver_->SignalDone.connect(this, &PeerConnectionClient::OnResolveResult);
		resolver_->Start(server_address_);
	}
	else {
		DoConnect();
	}
}

void PeerConnectionClient::OnResolveResult(
	rtc::AsyncResolverInterface* resolver) {
	if (resolver_->GetError() != 0) {
		emit signal_ServerConnectionFailure();
		resolver_->Destroy(false);
		resolver_ = NULL;
		state_ = NOT_CONNECTED;
	}
	else {
		server_address_ = resolver_->address();
		DoConnect();
	}
}

void PeerConnectionClient::DoConnect() {
	control_socket_.reset(CreateClientSocket(server_address_.ipaddr().family()));
	hanging_get_.reset(CreateClientSocket(server_address_.ipaddr().family()));
	InitSocketSignals();
	char buffer[1024];
	sprintfn(buffer, sizeof(buffer),
		"GET /sign_in?%s HTTP/1.0\r\n\r\n", client_name_.c_str());
	onconnect_data_ = buffer;

	bool ret = ConnectControlSocket();
	if (ret)
		state_ = SIGNING_IN;
	if (!ret) {
		emit signal_ServerConnectionFailure();
	}
}

bool PeerConnectionClient::SendToPeer(qint64 peer_id, const std::string& message) {
	if (state_ != CONNECTED)
		return false;

	RTC_DCHECK(is_connected());
	RTC_DCHECK(control_socket_->GetState() == rtc::Socket::CS_CLOSED);
	if (!is_connected() || peer_id == -1)
		return false;

	char headers[1024];
	sprintfn(headers, sizeof(headers),
		"POST /message?peer_id=%i&to=%i HTTP/1.0\r\n"
		"Content-Length: %i\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n",
		my_id_, peer_id, message.length());
	onconnect_data_ = headers;
	onconnect_data_ += message;
	return ConnectControlSocket();
}

bool PeerConnectionClient::DisconnectPeer(qint64 peer_id) {
	return SendToPeer(peer_id, kByeMessage);
}

bool PeerConnectionClient::IsSendingMessage() {
	return state_ == CONNECTED &&
		control_socket_->GetState() != rtc::Socket::CS_CLOSED;
}

bool PeerConnectionClient::SignOut() {
	if (state_ == NOT_CONNECTED || state_ == SIGNING_OUT)
		return true;

	if (hanging_get_->GetState() != rtc::Socket::CS_CLOSED)
		hanging_get_->Close();

	if (control_socket_->GetState() == rtc::Socket::CS_CLOSED) {
		state_ = SIGNING_OUT;

		if (my_id_ != -1) {
			char buffer[1024];
			sprintfn(buffer, sizeof(buffer),
				"GET /sign_out?peer_id=%i HTTP/1.0\r\n\r\n", my_id_);
			onconnect_data_ = buffer;
			return ConnectControlSocket();
		}
		else {
			// Can occur if the app is closed before we finish connecting.
			return true;
		}
	}
	else {
		state_ = SIGNING_OUT_WAITING;
	}

	return true;
}

void PeerConnectionClient::Close() {
	control_socket_->Close();
	hanging_get_->Close();
	onconnect_data_.clear();
	peers_.clear();
	if (resolver_ != NULL) {
		resolver_->Destroy(false);
		resolver_ = NULL;
	}
	my_id_ = -1;
	state_ = NOT_CONNECTED;
}

bool PeerConnectionClient::ConnectControlSocket() {
	RTC_DCHECK(control_socket_->GetState() == rtc::Socket::CS_CLOSED);
	int err = control_socket_->Connect(server_address_);
	if (err == SOCKET_ERROR) {
		Close();
		return false;
	}
	return true;
}

void PeerConnectionClient::OnConnect(rtc::AsyncSocket* socket) {
	RTC_DCHECK(!onconnect_data_.empty());
	RTC_LOG(LS_INFO) << "xxxxxxxxxxxxx" << onconnect_data_;
	size_t sent = socket->Send(onconnect_data_.c_str(), onconnect_data_.length());
	RTC_DCHECK(sent == onconnect_data_.length());
	onconnect_data_.clear();
}

void PeerConnectionClient::OnHangingGetConnect(rtc::AsyncSocket* socket) {
	char buffer[1024];
	sprintfn(buffer, sizeof(buffer),
		"GET /wait?peer_id=%i HTTP/1.0\r\n\r\n", my_id_);
	int len = static_cast<int>(strlen(buffer));
	int sent = socket->Send(buffer, len);
	RTC_DCHECK(sent == len);
}

void PeerConnectionClient::OnMessageFromPeer(qint64 peer_id,
	const std::string& message) {
	if (message.length() == (sizeof(kByeMessage) - 1) &&
		message.compare(kByeMessage) == 0) {
		RTC_LOG(LS_INFO) << "signal_PeerDisconnected" << peer_id;
		emit signal_PeerDisconnected(peer_id);
	}
	else {
		handlePeerMsg(peer_id, message);
	}
}

void PeerConnectionClient::handlePeerMsg(qint64 peer_id, const std::string& message) {
	RTC_DCHECK(!message.empty());

	RTC_LOG(LS_INFO) << "xxxxxxxxxxxxx" << message;

	Json::Reader reader;
	Json::Value jmessage;
	if (!reader.parse(message, jmessage)) {
		RTC_LOG(WARNING) << "Received unknown message. " << message;
		return;
	}
	std::string type;
	std::string json_object;

	rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
	if (!type.empty()) {

		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName,
			&sdp)) {
			RTC_LOG(WARNING) << "Can't parse received session description message.";
			return;
		}

		emit retmoeSDP(peer_id, STD_TO_QT(type), STD_TO_QT(sdp));
// 
// 		webrtc::SdpParseError error;
// 		webrtc::SessionDescriptionInterface* session_description(
// 			webrtc::CreateSessionDescription(type, sdp, &error));
// 		if (!session_description) {
// 			RTC_LOG(WARNING) << "Can't parse received session description message. "
// 				<< "SdpParseError was: " << error.description;
// 			return;
// 		}
// 		RTC_LOG(INFO) << " Received session description :" << message;
// 		peer_connection_->SetRemoteDescription(
// 			DummySetSessionDescriptionObserver::Create(), session_description);
// 		if (session_description->type() ==
// 			webrtc::SessionDescriptionInterface::kOffer) {
// 			peer_connection_->CreateAnswer(this, NULL);
// 		}
		return;
	}
	else {
		std::string sdp_mid;
		int sdp_mlineindex = 0;
		std::string sdp;
		if (!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpMidName,
			&sdp_mid) ||
			!rtc::GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
				&sdp_mlineindex) ||
			!rtc::GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
			RTC_LOG(WARNING) << "Can't parse received message.";
			return;
		}

		emit retmoeIce(peer_id, STD_TO_QT(sdp_mid), sdp_mlineindex, STD_TO_QT(sdp));

// 		webrtc::SdpParseError error;
// 		std::unique_ptr<webrtc::IceCandidateInterface> candidate(
// 			webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp, &error));
// 		if (!candidate.get()) {
// 			RTC_LOG(WARNING) << "Can't parse received candidate message. "
// 				<< "SdpParseError was: " << error.description;
// 			return;
// 		}
// 		if (!peer_connection_->AddIceCandidate(candidate.get())) {
// 			RTC_LOG(WARNING) << "Failed to apply the received candidate";
// 			return;
// 		}
		RTC_LOG(INFO) << " Received candidate :" << message;
		return;
	}
}


bool PeerConnectionClient::GetHeaderValue(const std::string& data,
	size_t eoh,
	const char* header_pattern,
	size_t* value) {
	RTC_DCHECK(value != NULL);
	size_t found = data.find(header_pattern);
	if (found != std::string::npos && found < eoh) {
		*value = atoi(&data[found + strlen(header_pattern)]);
		return true;
	}
	return false;
}

bool PeerConnectionClient::GetHeaderValue(const std::string& data, size_t eoh,
	const char* header_pattern,
	std::string* value) {
	RTC_DCHECK(value != NULL);
	size_t found = data.find(header_pattern);
	if (found != std::string::npos && found < eoh) {
		size_t begin = found + strlen(header_pattern);
		size_t end = data.find("\r\n", begin);
		if (end == std::string::npos)
			end = eoh;
		value->assign(data.substr(begin, end - begin));
		return true;
	}
	return false;
}

bool PeerConnectionClient::ReadIntoBuffer(rtc::AsyncSocket* socket,
	std::string* data,
	size_t* content_length) {
	char buffer[0xffff];
	do {
		int bytes = socket->Recv(buffer, sizeof(buffer), nullptr);
		if (bytes <= 0)
			break;
		data->append(buffer, bytes);
	} while (true);

	bool ret = false;
	size_t i = data->find("\r\n\r\n");
	if (i != std::string::npos) {
		RTC_LOG(INFO) << "Headers received";
		if (GetHeaderValue(*data, i, "\r\nContent-Length: ", content_length)) {
			size_t total_response_size = (i + 4) + *content_length;
			if (data->length() >= total_response_size) {
				ret = true;
				std::string should_close;
				const char kConnection[] = "\r\nConnection: ";
				if (GetHeaderValue(*data, i, kConnection, &should_close) &&
					should_close.compare("close") == 0) {
					socket->Close();
					// Since we closed the socket, there was no notification delivered
					// to us.  Compensate by letting ourselves know.
					OnClose(socket, 0);
				}
			}
			else {
				// We haven't received everything.  Just continue to accept data.
			}
		}
		else {
			RTC_LOG(LS_ERROR) << "No content length field specified by the server.";
		}
	}
	return ret;
}

void PeerConnectionClient::OnRead(rtc::AsyncSocket* socket) {
	size_t content_length = 0;
	if (ReadIntoBuffer(socket, &control_data_, &content_length)) {
		size_t peer_id = 0, eoh = 0;
		bool ok = ParseServerResponse(control_data_, content_length, &peer_id,
			&eoh);
		if (ok) {
			if (my_id_ == -1) {
				// First response.  Let's store our server assigned ID.
				RTC_DCHECK(state_ == SIGNING_IN);
				my_id_ = static_cast<qint64>(peer_id);
				RTC_DCHECK(my_id_ != -1);

				// The body of the response will be a list of already connected peers.
				if (content_length) {
					size_t pos = eoh + 4;
					while (pos < control_data_.size()) {
						size_t eol = control_data_.find('\n', pos);
						if (eol == std::string::npos)
							break;
						qint64 id = 0;
						std::string name;
						bool connected;
						if (ParseEntry(control_data_.substr(pos, eol - pos), &name, &id,
							&connected) && id != my_id_) {
							peers_[id] = name;
							RTC_LOG(LS_INFO) << "signal_PeerConnected id:" << id << " name:" << name.c_str();
							emit signal_PeerConnected(id, true);
						}
						pos = eol + 1;
					}
				}
				RTC_DCHECK(is_connected());
				emit signal_SignedIn();
			}
			else if (state_ == SIGNING_OUT) {
				Close();
				emit signal_Disconnected();
			}
			else if (state_ == SIGNING_OUT_WAITING) {
				SignOut();
			}
		}

		control_data_.clear();

		if (state_ == SIGNING_IN) {
			RTC_DCHECK(hanging_get_->GetState() == rtc::Socket::CS_CLOSED);
			state_ = CONNECTED;
			hanging_get_->Connect(server_address_);
		}
	}
}

void PeerConnectionClient::OnHangingGetRead(rtc::AsyncSocket* socket) {
	RTC_LOG(INFO) << __FUNCTION__;
	size_t content_length = 0;
	if (ReadIntoBuffer(socket, &notification_data_, &content_length)) {
		size_t peer_id = 0, eoh = 0;
		RTC_LOG(LS_INFO) << notification_data_.c_str();

		bool ok = ParseServerResponse(notification_data_, content_length,
			&peer_id, &eoh);

		if (ok) {
			// Store the position where the body begins.
			size_t pos = eoh + 4;

			if (my_id_ == static_cast<qint64>(peer_id)) {
				// A notification about a new member or a member that just
				// disconnected.
				qint64 id = 0;
				std::string name;
				bool connected = false;
				if (ParseEntry(notification_data_.substr(pos), &name, &id,
					&connected)) {
					if (connected) {
						peers_[id] = name;
						RTC_LOG(LS_INFO) << "signal_PeerConnected" << id << name.c_str();
						emit signal_PeerConnected(id, true);
					}
					else {
						peers_.erase(id);
						RTC_LOG(LS_INFO) << "signal_PeerDisconnected" << id << name.c_str();
						emit signal_PeerDisconnected(id);
					}
				}
			}
			else {
				OnMessageFromPeer(static_cast<qint64>(peer_id),
					notification_data_.substr(pos));
			}
		}

		notification_data_.clear();
	}

	if (hanging_get_->GetState() == rtc::Socket::CS_CLOSED &&
		state_ == CONNECTED) {
		hanging_get_->Connect(server_address_);
	}
}

bool PeerConnectionClient::ParseEntry(const std::string& entry,
	std::string* name,
	qint64* id,
	bool* connected) {
	RTC_DCHECK(name != NULL);
	RTC_DCHECK(id != NULL);
	RTC_DCHECK(connected != NULL);
	RTC_DCHECK(!entry.empty());

	*connected = false;
	size_t separator = entry.find(',');
	if (separator != std::string::npos) {
		*id = atoi(&entry[separator + 1]);
		name->assign(entry.substr(0, separator));
		separator = entry.find(',', separator + 1);
		if (separator != std::string::npos) {
			*connected = atoi(&entry[separator + 1]) ? true : false;
		}
	}
	return !name->empty();
}

int PeerConnectionClient::GetResponseStatus(const std::string& response) {
	int status = -1;
	size_t pos = response.find(' ');
	if (pos != std::string::npos)
		status = atoi(&response[pos + 1]);
	return status;
}

bool PeerConnectionClient::ParseServerResponse(const std::string& response,
	size_t content_length,
	size_t* peer_id,
	size_t* eoh) {
	int status = GetResponseStatus(response.c_str());
	if (status != 200) {
		RTC_LOG(LS_ERROR) << "Received error from server";
		Close();
		emit signal_Disconnected();
		return false;
	}

	*eoh = response.find("\r\n\r\n");
	RTC_DCHECK(*eoh != std::string::npos);
	if (*eoh == std::string::npos)
		return false;

	*peer_id = -1;

	// See comment in peer_channel.cc for why we use the Pragma header and
	// not e.g. "X-Peer-Id".
	GetHeaderValue(response, *eoh, "\r\nPragma: ", peer_id);

	return true;
}

void PeerConnectionClient::OnClose(rtc::AsyncSocket* socket, int err) {
	RTC_LOG(INFO) << __FUNCTION__;

	socket->Close();

#ifdef WIN32
	if (err != WSAECONNREFUSED) {
#else
	if (err != ECONNREFUSED) {
#endif
		if (socket == hanging_get_.get()) {
			if (state_ == CONNECTED) {
				hanging_get_->Close();
				hanging_get_->Connect(server_address_);
			}
		}
		else {
			SendMsg(0, NULL);
		}
	}
	else {
		if (socket == control_socket_.get()) {
			RTC_LOG(WARNING) << "Connection refused; retrying in 2 seconds";
// 			rtc::Thread::Current()->PostDelayed(RTC_FROM_HERE, kReconnectDelay, this,
// 				0);
		}
		else {
			Close();
			emit signal_Disconnected();
		}
	}
}

void PeerConnectionClient::SendMsg(qint64 peer_id, std::string *message)
{
	if (message && !message->empty()) {
		// For convenience, we always run the message through the queue.
		// This way we can be sure that messages are sent to the server
		// in the same order they were signaled without much hassle.
		pending_messages_.push_back(std::pair<qint64, std::string*>(peer_id, message));
	}

	if (!pending_messages_.empty() && !IsSendingMessage()) {
		std::pair<qint64, std::string*> message = pending_messages_.front();
		pending_messages_.pop_front();

		if (!SendToPeer(message.first, *message.second)) {
			RTC_LOG(LS_ERROR) << "SendToPeer failed";
		}
		delete message.second;
	}
}

void PeerConnectionClient::ConnectServer(const std::string& server) {
	if (is_connected())
		return;
	server_ = server;
	Connect(server, 0, GetPeerName());
}

void PeerConnectionClient::DisconnectServer() {
	if (is_connected())
		SignOut();
}

void PeerConnectionClient::sendSDP(qint64 id, QString sdp, QString type)
{
	Json::StyledWriter writer;
	Json::Value jmessage;

	//https://ask.csdn.net/questions/380557
	//使用toStdString或者使用toStdWString会出错，这个可能是有些版本的qt的bug，编译是能编过，但是运行时会段错误。你改为toLocal8bit().data()
	jmessage[kSessionDescriptionTypeName] = QT_TO_STD(type);
	jmessage[kSessionDescriptionSdpName] = QT_TO_STD(sdp);

	std::string *msg = new std::string(writer.write(jmessage));
	SendMsg(id, msg);
}

void PeerConnectionClient::sendCandidate(qint64 id, QString sdpMid, int sdpMLineIndex, QString candidate)
{
	Json::StyledWriter writer;
	Json::Value jmessage;

	jmessage[kCandidateSdpMidName] = QT_TO_STD(sdpMid);
	jmessage[kCandidateSdpMlineIndexName] = sdpMLineIndex;
	jmessage[kCandidateSdpName] = QT_TO_STD(candidate);

	std::string *msg = new std::string(writer.write(jmessage));
	SendMsg(id, msg);
}

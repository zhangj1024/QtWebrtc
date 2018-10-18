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
#include "QtWebrtcRemoteStream.h"

#include <memory>
#include <utility>
#include <vector>

#include "api/test/fakeconstraints.h"
#include "defaults.h"
#include "media/engine/webrtcvideocapturerfactory.h"
#include "modules/video_capture/video_capture_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/json.h"
#include "rtc_base/logging.h"

#include "ConnecttionFactory.h"
#include "QtWebrtcLocalStream.h"

const char kStreamId[] = "stream_label";

class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver
{
public:
	static DummySetSessionDescriptionObserver* Create()
	{
		return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
	virtual void OnFailure(webrtc::RTCError error)
	{
		RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
			<< error.message();
	}
};

QtWebrtcRemoteStream::QtWebrtcRemoteStream(qint64 peer_id)
	: peer_id_(peer_id)
{
	QObject::connect(_WebrtcLocalStream, SIGNAL(VideoTrackChanged()), this, SLOT(OnVideoTrackChanged()));
}

QtWebrtcRemoteStream::~QtWebrtcRemoteStream()
{
	RTC_DCHECK(!peer_connection_);
}

bool QtWebrtcRemoteStream::InitializePeerConnection()
{
	RTC_DCHECK(_ConnecttionFactory);
	RTC_DCHECK(!peer_connection_);

	if (!CreatePeerConnection(/*dtls=*/true))
	{
		RTC_LOG(INFO) << "CreatePeerConnection failed";
		DeletePeerConnection();
	}

	AddTracks();

	return peer_connection_ != nullptr;
}

bool QtWebrtcRemoteStream::CreatePeerConnection(bool dtls)
{
	RTC_DCHECK(_ConnecttionFactory);
	RTC_DCHECK(!peer_connection_);

	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
	config.enable_dtls_srtp = dtls;
	webrtc::PeerConnectionInterface::IceServer server;
	server.uri = GetPeerConnectionString();
	config.servers.push_back(server);

	peer_connection_ = _ConnecttionFactory->CreatePeerConnection(
		config, nullptr, nullptr, this);
	return peer_connection_ != nullptr;
}

void QtWebrtcRemoteStream::DeletePeerConnection()
{
	peer_connection_ = nullptr;
	//   _ConnecttionFactory = nullptr;
	peer_id_ = -1;
}

void QtWebrtcRemoteStream::AddTracks()
{
	if (!peer_connection_->GetSenders().empty())
	{
		return;  // Already added tracks.
	}

	auto result_or_error = peer_connection_->AddTrack(_WebrtcLocalStream->GetAudioTrack(), { kStreamId });
	if (!result_or_error.ok())
	{
		RTC_LOG(LS_ERROR) << "Failed to add audio track to PeerConnection: "
			<< result_or_error.error().message();
	}

	result_or_error = peer_connection_->AddTrack(_WebrtcLocalStream->GetVideoTrack(), { kStreamId });
	if (!result_or_error.ok())
	{
		RTC_LOG(LS_ERROR) << "Failed to add video track to PeerConnection: "
			<< result_or_error.error().message();
	}
}

//
// PeerConnectionObserver implementation.
//

void QtWebrtcRemoteStream::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
	const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&streams)
{
	RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id() << " "
		<< receiver->track()->kind();

	QMetaObject::invokeMethod(this, "on_peerConnection_addTrack", Q_ARG(webrtc::MediaStreamTrackInterface*, receiver->track().release()));
}

void QtWebrtcRemoteStream::on_peerConnection_addTrack(webrtc::MediaStreamTrackInterface* track)
{
	if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind)
	{
		auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
		this->SetVideoTrack(video_track);
		this->StartView();
	}
}

void QtWebrtcRemoteStream::OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver)
{
	RTC_LOG(INFO) << __FUNCTION__ << " " << receiver->id();
	QMetaObject::invokeMethod(this, "on_peerConnection_removeTrack", Q_ARG(webrtc::MediaStreamTrackInterface*, receiver->track().release()));
}

void QtWebrtcRemoteStream::on_peerConnection_removeTrack(webrtc::MediaStreamTrackInterface* track)
{
	if (track->kind() == webrtc::MediaStreamTrackInterface::kVideoKind)
	{
		auto* video_track = static_cast<webrtc::VideoTrackInterface*>(track);
		this->StopView();
	}

	track->Release();
}

void QtWebrtcRemoteStream::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
	switch (new_state)
	{
	case webrtc::PeerConnectionInterface::kIceGatheringNew:
		RTC_LOG(LERROR) << "--------------------------" << "kIceGatheringNew";
		break;
	case webrtc::PeerConnectionInterface::kIceGatheringGathering:
		RTC_LOG(LERROR) << "--------------------------" << "kIceGatheringGathering";
		break;
	case webrtc::PeerConnectionInterface::kIceGatheringComplete:
		RTC_LOG(LERROR) << "--------------------------" << "kIceGatheringComplete";
		break;
	default:
		break;
	}

	if (new_state == webrtc::PeerConnectionInterface::kIceGatheringComplete)
	{
		emit IceGatheringComplete(peer_id_);
	}
}

void QtWebrtcRemoteStream::OnIceCandidate(const webrtc::IceCandidateInterface* candidate)
{
	RTC_LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
	std::string sdp;
	if (!candidate->ToString(&sdp))
	{
		RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
		return;
	}

	emit LocalIceCandidate(peer_id_, STD_TO_QT(candidate->sdp_mid()), candidate->sdp_mline_index(), STD_TO_QT(sdp));
}

void QtWebrtcRemoteStream::AddPeerIceCandidate(QString sdp_mid_, int sdp_mlineindex_, QString candidate_)
{
	RTC_DCHECK(!sdp_mid_.isEmpty());
	RTC_DCHECK(!candidate_.isEmpty());

	if (!peer_connection_.get())
	{
		RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
		return;
	}

	webrtc::SdpParseError error;
	std::unique_ptr<webrtc::IceCandidateInterface> candidate(
		webrtc::CreateIceCandidate(QT_TO_STD(sdp_mid_), sdp_mlineindex_, QT_TO_STD(candidate_), &error));
	if (!candidate.get())
	{
		RTC_LOG(WARNING) << "Can't parse received candidate message. "
			<< "SdpParseError was: " << error.description;
		return;
	}
	if (!peer_connection_->AddIceCandidate(candidate.get()))
	{
		RTC_LOG(WARNING) << "Failed to apply the received candidate";
		return;
	}
	// 		RTC_LOG(INFO) << " Received candidate :" << message;
	return;
}

void QtWebrtcRemoteStream::SetPeerSDP(QString type, QString sdp)
{
	RTC_DCHECK(!sdp.isEmpty());
	if (!peer_connection_.get())
	{
		RTC_LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
		return;
	}

	webrtc::SdpParseError error;
	webrtc::SessionDescriptionInterface* session_description(
		webrtc::CreateSessionDescription(QT_TO_STD(type), QT_TO_STD(sdp), &error));
	if (!session_description)
	{
		RTC_LOG(WARNING) << "Can't parse received session description message. "
			<< "SdpParseError was: " << error.description;
		return;
	}

	peer_connection_->SetRemoteDescription(
		DummySetSessionDescriptionObserver::Create(), session_description);
	if (session_description->type() ==
		webrtc::SessionDescriptionInterface::kOffer)
	{
		peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
	}
}

//
// MainWndCallback implementation.
//

void QtWebrtcRemoteStream::ConnectToPeer()
{
	RTC_DCHECK(peer_id_ != -1);

	if (peer_connection_.get())
	{
		RTC_LOG(INFO) << "We only support connecting to one peer at a time";
		return;
	}

	if (InitializePeerConnection())
	{
		peer_connection_->CreateOffer(
			this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
	}
	else
	{
		RTC_LOG(INFO) << "Failed to initialize PeerConnection";
	}
}

void QtWebrtcRemoteStream::OnSuccess(webrtc::SessionDescriptionInterface* desc)
{
	peer_connection_->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	emit LocalSDP(peer_id_, STD_TO_QT(sdp), STD_TO_QT(desc->type()));
}

void QtWebrtcRemoteStream::OnFailure(webrtc::RTCError error)
{
	RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}


void QtWebrtcRemoteStream::OnVideoTrackChanged()
{
	std::vector<rtc::scoped_refptr<webrtc::RtpSenderInterface>> sends = peer_connection_->GetSenders();
	for (auto send : sends)
	{
		if (send->media_type() == cricket::MEDIA_TYPE_VIDEO)
		{
			send->SetTrack(_WebrtcLocalStream->GetVideoTrack());
		}
	}
}

void QtWebrtcRemoteStream::EnableSend(bool enable)
{
	peer_connection_->EnableSendAudio(enable);
	peer_connection_->EnableSendVideo(enable);
}
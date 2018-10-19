/*
*  Copyright 2012 The WebRTC Project Authors. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

#include <deque>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <QObject>
#include "QtWebrtcStream.h"

#include "api/mediastreaminterface.h"
#include "api/peerconnectioninterface.h"

namespace webrtc
{
	class VideoCaptureModule;
}  // namespace webrtc

namespace cricket
{
	class VideoRenderer;
}  // namespace cricket

class QtWebrtcRemoteStream
	: public QObject,
	public QtWebrtcStream,
	public webrtc::PeerConnectionObserver,
	public webrtc::CreateSessionDescriptionObserver
{
	Q_OBJECT

public:
	QtWebrtcRemoteStream(qint64 peer_id);

	qint64 id() const { return peer_id_; };
	void setid(qint64 id) { peer_id_ = id; };

public slots:
	void ConnectToPeer();
	bool AddPeerIceCandidate(QString sdp_mid, int sdp_mlineindex, QString candidate);
	bool SetPeerSDP(QString type, QString sdp);
	void DeletePeerConnection();
	void OnVideoTrackChanged();
	void EnableSend(bool enable);
Q_SIGNALS:
	void LocalIceCandidate(qint64 id, QString sdp_mid, int sdp_mlineindex, QString candidate);
	void LocalSDP(qint64 id, QString sdp, QString type);
	void IceGatheringComplete(qint64 id);

protected:
	~QtWebrtcRemoteStream();
	bool InitializePeerConnection();
	bool CreatePeerConnection(bool dtls);
	void AddTracks();

	//
	// PeerConnectionObserver implementation.
	//

	void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {};
	void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
		const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>& streams) override;
	void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
	void OnRenegotiationNeeded() override {}
	void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {};
	void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;
	void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
	void OnIceConnectionReceivingChange(bool receiving) override {}

	// CreateSessionDescriptionObserver implementation.
	void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
	void OnFailure(webrtc::RTCError error) override;

private slots:
	void on_peerConnection_addTrack(webrtc::MediaStreamTrackInterface* track);
	void on_peerConnection_removeTrack(webrtc::MediaStreamTrackInterface* track);

protected:

	qint64 peer_id_ = -1;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
};

#endif  // EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_

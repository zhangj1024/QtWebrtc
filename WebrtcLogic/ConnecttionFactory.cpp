#include "stdafx.h"
#include "ConnecttionFactory.h"

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> ConnecttionFactory::_peer_connection_factory = NULL;

bool ConnecttionFactory::Init()
{
	_peer_connection_factory = webrtc::CreatePeerConnectionFactory(
		nullptr /* network_thread */, nullptr /* worker_thread */,
		nullptr /* signaling_thread */, nullptr /* default_adm */,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		webrtc::CreateBuiltinVideoEncoderFactory(),
		webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
		nullptr /* audio_processing */);

	return _peer_connection_factory == NULL;
}

rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> ConnecttionFactory::Get()
{
	if (_peer_connection_factory == NULL)
	{
		Init();
	}

	return _peer_connection_factory;
}
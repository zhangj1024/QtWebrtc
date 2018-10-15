#pragma once
#include "api/peerconnectioninterface.h"

class ConnecttionFactory
{
public:
	static bool Init();
	static void UnInit() { _peer_connection_factory = nullptr; };

	static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> Get();

private:
	static rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> _peer_connection_factory;
};


#define _ConnecttionFactory ConnecttionFactory::Get()
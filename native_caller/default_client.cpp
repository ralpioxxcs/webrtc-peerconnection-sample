#include "default_client.h"

#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>

#include <iostream>

class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
 public:
	static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create() {
		return rtc::make_ref_counted<DummySetSessionDescriptionObserver>();
	}
	void OnSuccess() override {
		std::cout << "\nDummySetSessionDescriptionObserver OnSuccess" << std::endl;
	}
	void OnFailure(webrtc::RTCError error) override {
		std::cout << "\nDummySetSessionDescriptionObserver OnFailure (err: " << ToString(error.type()) << ", msg: " << error.message() << ")" << std::endl;
	}
};

// Initialize peer
void DefaultClient::Initialize() {
	// create network thread
	if (!mNetworkThread.get()) {
		mNetworkThread = rtc::Thread::Create();
		mNetworkThread->SetName("network_thread", nullptr);
	}

	// create worker thread
	if (!mWorkerThread.get()) {
		mWorkerThread = rtc::Thread::Create();
		mWorkerThread->SetName("worker_thread", nullptr);
	}

	// create signaling thread
	if (!mSignalThread.get()) {
		mSignalThread = rtc::Thread::CreateWithSocketServer();
		mSignalThread->SetName("signal_thread", nullptr);
	}

	mNetworkThread->Start();
	mWorkerThread->Start();
	mSignalThread->Start();

	mPeerConnectionFactory = webrtc::CreatePeerConnectionFactory(
			mNetworkThread.get(),		// network
			mWorkerThread.get(),		// worker
			mSignalThread.get(),		// signal
			nullptr,								// adm
			webrtc::CreateBuiltinAudioEncoderFactory(), webrtc::CreateBuiltinAudioDecoderFactory(),
			webrtc::CreateBuiltinVideoEncoderFactory(), webrtc::CreateBuiltinVideoDecoderFactory(),
			nullptr, nullptr);

	if (!mPeerConnectionFactory) {
		std::cerr << "Error!, Failed to initialize PeerConnectionFactory" << std::endl;
		return;
	}
}

void DefaultClient::ConnectPeer() {
	// create peer connection
	webrtc::PeerConnectionInterface::RTCConfiguration config;
	config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;

	webrtc::PeerConnectionInterface::IceServer stun_1;
	webrtc::PeerConnectionInterface::IceServer stun_2;
	stun_1.uri = "stun:stun1.l.google.com:19302";
	stun_2.uri = "stun:stun2.l.google.com:19302";
	config.servers.push_back(stun_1);
	config.servers.push_back(stun_2);

	webrtc::PeerConnectionDependencies peerConnectionDependencies(this);
	auto errOrPeerConn = mPeerConnectionFactory->CreatePeerConnectionOrError(config, std::move(peerConnectionDependencies));
	if(errOrPeerConn.ok()) {
		std::cout << "## success to create peer connection" << std::endl; 
		mPeerConnection = errOrPeerConn.MoveValue();
	} else {
		std::cerr << "## failed to create peer connection (err: " << errOrPeerConn.error().message() << ")" << std::endl;
	}

	// create data channel
	webrtc::DataChannelInit dataChannelConfig;
	dataChannelConfig.ordered = true;

	auto errOrDataChannel = mPeerConnection->CreateDataChannelOrError("data label", &dataChannelConfig);
	if(errOrDataChannel.ok()) {
		std::cout << "## success to create data channel" << std::endl; 
		mDataChannel = errOrDataChannel.MoveValue();
	} else {
		std::cerr << "## failed to create data channel (err: " << errOrDataChannel.error().message() << ")" << std::endl;
	}

	mDataChannel->RegisterObserver(this);

	//
	// 1. Create Offer SDP
	//
	std::string offer;
	std::getline(std::cin, offer);
	mPeerConnection->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void DefaultClient::ReceiveAnswer(std::string answer) {
	std::cout << "set remote description "<< std::endl;
	std::unique_ptr<webrtc::SessionDescriptionInterface> sessDesc = webrtc::CreateSessionDescription(webrtc::SdpType::kAnswer, answer);
	mPeerConnection->SetRemoteDescription(DummySetSessionDescriptionObserver::Create().get(), sessDesc.release());
}

void DefaultClient::ReceiveIce(std::string candidate, std::string sdpMid, std::string sdpMLineIndex) {
		webrtc::SdpParseError errSdp;
		webrtc::IceCandidateInterface *iceInterface = webrtc::CreateIceCandidate(sdpMid, std::atoi(sdpMLineIndex.c_str()), candidate, &errSdp);
		if (!errSdp.line.empty() && !errSdp.description.empty()) {
			std::cerr << "error on create ice candidate " << std::endl;
            return;
		}

		if (!mPeerConnection->AddIceCandidate(iceInterface)) {
			std::cerr << "## failed to add ice candidates" << std::endl;
            return;
		}

		std::cout << "## success to add ice candidate" << std::endl;
}


void DefaultClient::OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) {
	std::cout << "OnDataChannel (+)" << std::endl;
	std::cout << "OnDataChannel (-)" << std::endl;
}

void DefaultClient::OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
															 const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &treams) {
	std::cout << "OnAddTrack (+)" << std::endl;
	std::cout << "OnAddTrack (-)" << std::endl;
};

void DefaultClient::OnIceCandidate(const webrtc::IceCandidateInterface *candidate) {
	std::cout << "OnIceCandidate (+)" << std::endl;

	//mPeerConnection->AddIceCandidate(candidate);

	std::string candidateStr;
	candidate->ToString(&candidateStr);
	std::cout << "-----------------------------[ICE (+)]------------------------------\n";
	std::cout << "-- ICE candidate:\n" << candidateStr << std::endl;
	std::cout << "-- ICE sdp_mid:\n" << candidate->sdp_mid() << std::endl;
	std::cout << "-- ICE sdp_mline_index:\n" << candidate->sdp_mline_index() << std::endl;
	std::cout << "-----------------------------[ICE (-)]------------------------------\n";

    Json::Value data;
    data["candidate"] = candidateStr;
    data["sdp_mid"] = candidate->sdp_mid();
    data["sdp_mline_index"] = candidate->sdp_mline_index();

    Json::Value payload;
    payload["type"] = "ice";
    payload["data"] = data;
	
    mWs->broadcastMessage(payload);

	std::cout << "OnIceCandidate (-)" << std::endl;
}

void DefaultClient::OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) {
	std::cout << "OnSignalingChange (+)" << std::endl;

	std::cout << "-- Signaling State: " << webrtc::PeerConnectionInterface::AsString(newState) << std::endl;

	std::cout << "OnSignalingChange (-)" << std::endl;
};

void DefaultClient::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state)
{
	std::cout << "OnIceConnectionChange (+)" << std::endl;

	std::cout << "-- ice connection state: " << webrtc::PeerConnectionInterface::AsString(new_state) << std::endl;

	std::cout << "OnIceConnectionChange (-)" << std::endl;
}

void DefaultClient::OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state)
{
	std::cout << "OnIceGatheringChange (+)" << std::endl;

	std::cout << "-- ice gathering state: " << webrtc::PeerConnectionInterface::AsString(new_state) << std::endl;

	std::cout << "OnIceGatheringChange (-)" << std::endl;
}

void DefaultClient::OnSuccess(webrtc::SessionDescriptionInterface *desc) {
	std::cout << "OnSuccess (+)" << std::endl;

	mPeerConnection->SetLocalDescription(DummySetSessionDescriptionObserver::Create().get(), desc);

	std::string sdp;
	desc->ToString(&sdp);

	/*
	std::cout << "==> SDP Offer [[Ctrl+C]]\n"
	<< "----------------------------------------------------------\n" 
	<< sdp
	<< "----------------------------------------------------------\n" 
	<< std::endl;
	*/

    Json::Value data;
    data["sdp"] = sdp;
    data["type"] = "offer";

	Json::Value payload;
	payload["type"] = "offer";
	payload["data"] = data;

	mWs->broadcastMessage(payload);

	std::cout << "OnSuccess (-)" << std::endl;
}

void DefaultClient::OnFailure(webrtc::RTCError err) {
	std::cout << "OnFailure(+)" << std::endl;
	std::cout << "OnFailure(-)" << std::endl;
}

void DefaultClient::OnStateChange() {
	std::cout << "OnStateChange(+)" << std::endl;
	std::cout << "OnStateChange(-)" << std::endl;
}

void DefaultClient::OnMessage(const webrtc::DataBuffer &buffer) {
	std::cout << "OnMessage(+)" << std::endl;

	std::cout << "-- Message from data channel: " << std::string(buffer.data.data<char>(), buffer.data.size()) << std::endl;

	std::cout << "OnMessage(-)" << std::endl;
}

void DefaultClient::OnBufferedAmountChange(uint64_t sent_data_size) {
	std::cout << "OnBufferedAmountChange(+)" << std::endl;

	std::cout << "-- Send data size: " << sent_data_size << std::endl;

	std::cout << "OnBufferedAmountChange(-)" << std::endl;
}

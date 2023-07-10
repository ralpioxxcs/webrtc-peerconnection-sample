#include <api/create_peerconnection_factory.h>
#include <api/media_stream_interface.h>
#include <api/peer_connection_interface.h>
#include <rtc_base/thread.h>

#include <memory>
#include "websocket_server.h"
// PeerConnection, SDP Create 상속
class DefaultClient : public webrtc::PeerConnectionObserver,
											public webrtc::CreateSessionDescriptionObserver,
                      public webrtc::DataChannelObserver {
 public:
	DefaultClient(){};
	~DefaultClient() { mPeerConnection->Close(); }

	void Initialize();

	void ConnectPeer();

    void ReceiveAnswer(std::string answer);

    void ReceiveIce(std::string candidate, std::string sdpMid, std::string sdpMLineIndex);

	void SetSocket(WebsocketServer* wss) {
		mWs = wss;
	}

 protected:
	//
	// [PeerConnectionObserver Impl] {{
	//
	// Triggered when a remote peer opens a data channel.
	void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel) override;

	// Triggered when the SignalingState changed.
	void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState newState) override;

	// This is called when a receiver and its track are created.
	void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
			            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>> &treams) override;

	// A new ICE candidate has been gathered.
	void OnIceCandidate(const webrtc::IceCandidateInterface *candidate) override;

	void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;

	void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override;

	void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override{};

	void OnIceConnectionReceivingChange(bool receiving) override{};

	//
	// [CreateSessionDescriptionObserver Impl]
	//
	void OnSuccess(webrtc::SessionDescriptionInterface *desc) override;

	void OnFailure(webrtc::RTCError err) override;

  //
  // [DataChannelObserver Impl]
  //
  // The data channel state have changed.
  void OnStateChange() override;

  //  A data buffer was successfully received.
  void OnMessage(const webrtc::DataBuffer& buffer) override;

  // The data channel's buffered_amount has changed.
  void OnBufferedAmountChange(uint64_t sent_data_size) override;

	//
	// ???
	//
	void AddRef() const override{};
	rtc::RefCountReleaseStatus Release() const override {
		return rtc::RefCountReleaseStatus::kDroppedLastRef;
	};

 private:
	//
	// Threads
	//
	std::unique_ptr<rtc::Thread> mNetworkThread;
	std::unique_ptr<rtc::Thread> mSignalThread;
	std::unique_ptr<rtc::Thread> mWorkerThread;

	rtc::scoped_refptr<webrtc::DataChannelInterface> mDataChannel;

	rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> mPeerConnectionFactory;
	rtc::scoped_refptr<webrtc::PeerConnectionInterface> mPeerConnection;

	// weboskcet (signaling)
	WebsocketServer* mWs;
};

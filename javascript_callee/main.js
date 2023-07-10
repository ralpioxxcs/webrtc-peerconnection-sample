const configuration = {
    "iceServers": [{ "url": "stun:stun2.1.google.com:19302" }]
};

let socket = null;

let peerConnection = null;
let sendDataChannel = null;
let recvDataChannel = null;

function init() {
  socket = new WebSocket("ws://172.29.10.208:8888"); // open websocket for signaling

  socket.addEventListener("open", (e) => {
    console.log("connection established");
  });

  socket.addEventListener("message", async (e) => {
    console.log("received data: ", e.data);

    const data = JSON.parse(e.data);
    console.log("parsed data: ", data);

    if (data["type"] == "offer") {
      console.log("offer: ", data["data"]);
      connectPeer(data["data"]);

    } else if (data["type"] == "ice") {
      console.log("ice: ", data["data"]);
      addIce(data["data"]);
    }

  });

  socket.addEventListener("close", (e) => {
    if (e.wasClean) {
      console.log("connection is closed gracfully");
    } else {
      console.log("connection is dead");
    }
  });

  socket.addEventListener("errror", () => {
    console.log("error");
  });

  // buttons
  document.querySelector("#connectBtn").addEventListener("click", connectPeer);
  document.querySelector("#sendBtn").addEventListener("click", sendMsg);
}

async function addIce(ice) {
  await peerConnection.addIceCandidate(
    new RTCIceCandidate({
      candidate: ice.candidate,
      sdpMid: ice.sdp_mid,
      sdpMLineIndex: ice.spd_mline_index,
    })
  );
  console.log("success to add ice candidates");
}

async function connectPeer(offer) {
  // create (remote) peer connection
  peerConnection = new RTCPeerConnection(configuration);

  // initialize peerconnection event listeners
  registerPeerConnectionListeners();

  // ice candidates callback
  peerConnection.addEventListener("icecandidate", async (event) => {
    if (!event.candidate) {
      console.log("got a final ICE candidate");
      return;
    }
    socket.send(JSON.stringify(
      {
        "type": "ice",
        "data": event.candidate
      }
    ));
    console.log("# got a ICE candidate", event.candidate.candidate);
  });

  // create data channel
  sendDataChannel = peerConnection.createDataChannel("myDataChannel");
  sendDataChannel.addEventListener("open", () => {
    console.log("channel opened! (state: " + sendDataChannel.readyState + ")");
  });
  sendDataChannel.addEventListener("close", () => {
    console.log("channel closed! (state: " + sendDataChannel.readyState + ")");
  });
  sendDataChannel.addEventListener("message", (event) => {
    console.log("received message: ", event.data);
  });

  peerConnection.addEventListener("datachannel", (e) => {
    console.log("on datachannel");

    recvDataChannel = e.channel;
    recvDataChannel.addEventListener("open", () => {
      console.log(
        "recv channel opened! (state: " + recvDataChannel.readyState + ")"
      );
    });
    recvDataChannel.addEventListener("close", () => {
      console.log(
        "recv channel closed! (state: " + recvDataChannel.readyState + ")"
      );
    });
    recvDataChannel.addEventListener("message", (event) => {
      console.log("recv received message: ", event.data);
    });
  });

  // set remote description from offer
  console.log("###### set remote description");
  await peerConnection.setRemoteDescription(new RTCSessionDescription(offer));

  console.log("###### create answer");
  const answer = await peerConnection.createAnswer();

  socket.send(JSON.stringify(
    {
      "type": "answer",
      "data": answer
    }
  ));

  console.log("###### set local description");
  peerConnection.setLocalDescription(answer);

  document.querySelector("#message-box").disabled = false;
  document.querySelector("#sendBtn").disabled = false;
}

async function sendMsg() {
  const data = document.querySelector("#message-box").value;
  if (data === "") {
    window.alert("message not exists");
    return;
  }

  sendDataChannel.send(data);

  console.log("send data: " + data);
}

function registerPeerConnectionListeners() {
  // when the state of the ICE candidate gathering process changes
  // iceGatheringState property has changed
  // * new: The peer connection was just created and has not done any networking yet
  // * gathering: The ICE agent is in the process of gathering candidates for the connection
  // * complete: The ICE agent has finished gathering candidates
  peerConnection.addEventListener("icegatheringstatechange", () => {
    console.log(
      `ICE gathering state changed: "${peerConnection.iceGatheringState}"`
    );
  });

  // RTCPeerConnection object after a new track has been added to an RTCRtpReceiver which is part of the connection
  peerConnection.addEventListener("connectionstatechange", (event) => {
    switch (peerConnection.connectionState) {
      case "new":
      case "checking":
        document.querySelector("#connectStatus").innerText = "Connecting ...";
        break;
      case "connected":
        document.querySelector("#connectStatus").innerText = "Online ...";
        break;
      case "disconnected":
        document.querySelector("#connectStatus").innerText =
          "Disconnecting ...";
        break;
      case "closed":
        document.querySelector("#connectStatus").innerText = "Offline";
        break;
      case "failed":
        document.querySelector("#connectStatus").innerText = "Error";
        break;
      default:
        document.querySelector("#connectStatus").innerText = "Unknown";
        break;
    }
    console.log(`Connection state change: ${peerConnection.connectionState}`);
  });

  peerConnection.addEventListener("negotiationneeded", (event) => {
    // signaling message is created and sent to the remote peer through the signaling server,
    // to share that offer with the other peer
    console.log("negotiation needed");
  });

  // Notify it that its signaling state, as indicated by the signalingState property, has changed.
  peerConnection.addEventListener("signalingstatechange", (event) => {
    console.log("signalingState state: ", peerConnection.signalingState);

    switch (peerConnection.signalingState) {
      case "stable":
        console.log("ICE negotiation complete");
        break;
    }
  });

  peerConnection.addEventListener("iceconnectionstatechange ", () => {
    console.log(
      `ICE connection state change: ${peerConnection.iceConnectionState}`
    );
  });
}

init();
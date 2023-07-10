#include <chrono>
#include <iostream>
#include <thread>

#include <asio/io_service.hpp>

#include "websocket_server.h"
#include "default_client.h"

//The port number the WebSocket server listens on
#define PORT_NUMBER 8888

int main(int argc, char *argv[]) {
	std::cout << "Hello WebRTC" << std::endl;

	//Create the event loop for the main thread, and the WebSocket server
	asio::io_service mainEventLoop;
	WebsocketServer server;
	DefaultClient client;
	
	//Register our network callbacks, ensuring the logic is run on the main thread's event loop
	server.connect([&mainEventLoop, &server](ClientConnection conn)
	{
		mainEventLoop.post([conn, &server]()
		{
			std::cout << "connection opened." << std::endl;
			std::cout << "There are now " << server.numConnections() << " open connections." << std::endl;
		});
	});

	server.disconnect([&mainEventLoop, &server](ClientConnection conn)
	{
		mainEventLoop.post([conn, &server]()
		{
			std::cout << "connection closed." << std::endl;
			std::cout << "There are now " << server.numConnections() << " open connections." << std::endl;
		});
	});

	server.message("answer", [&mainEventLoop, &server, &client](ClientConnection conn, const Json::Value& args)
	{
		mainEventLoop.post([conn, args, &server, &client]()
		{
			std::cout << "message handler on the main thread" << std::endl;
			std::cout << args << std::endl;
            client.ReceiveAnswer(args["sdp"].asString());
		});
	});

	server.message("ice", [&mainEventLoop, &server, &client](ClientConnection conn, const Json::Value& args)
	{
		mainEventLoop.post([conn, args, &server, &client]()
		{
			std::cout << "message handler on the main thread" << std::endl;
			std::cout << args << std::endl;
            client.ReceiveIce(args["candidate"].asString(), args["sdpMid"].asString(), args["sdpMLineIndex"].asString());
		});
	});
	
	//Start the networking thread
	std::thread serverThread([&server]() {
		server.run(PORT_NUMBER);
	});
	
    std::thread([&]() {
	client.SetSocket(&server);

	std::this_thread::sleep_for(std::chrono::seconds(3));

	client.Initialize();
	client.ConnectPeer();

  }).detach();
	
	//Start the event loop for the main thread
	asio::io_service::work work(mainEventLoop);
	mainEventLoop.run();

	return 0;
}

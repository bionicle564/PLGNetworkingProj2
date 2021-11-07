#define WIN32_LEAN_AND_MEAN

//client_main.cpp
//Gian Tullo, 0886424 / Lucas Magalhaes / Philip Tomaszewski
//23/10/21
//A chatroom client, allowing for the sending and receiving of messages

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <conio.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>
#include <Buffer.h>
#include <ProtocolHelper.h>


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512						// Default buffer length of our buffer in characters
#define DEFAULT_PORT "27015"					// The default port to use
#define SERVER "127.0.0.1"						// The IP of our server

Buffer outgoing(DEFAULT_BUFLEN);
Buffer incoming(DEFAULT_BUFLEN);

//The server doesn't know what rooms are, it just sends and then each client either acepts or rejects the data
//depending on what rooms the client knows about.
std::vector<std::string> rooms;


int main(int argc, char **argv)
{
	WSADATA wsaData;							// holds Winsock data
	SOCKET connectSocket = INVALID_SOCKET;		// Our connection socket used to connect to the server

	struct addrinfo *infoResult = NULL;			// Holds the address information of our server
	struct addrinfo *ptr = NULL;
	struct addrinfo hints;
	u_long mode = 1;

	// The messsage to send to the server
	std::string message = "";
	std::vector<std::string> chatlog;

	char recvbuf[DEFAULT_BUFLEN];				// The maximum buffer size of a message to send
	int result;									// code of the result of any command we use
	int recvbuflen = DEFAULT_BUFLEN;			// The length of the buffer we receive from the server

	// Step #1 Initialize Winsock
	result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
	{
		printf("WSAStartup failed with error: %d\n", result);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Step #2 Resolve the server address and port
	result = getaddrinfo(SERVER, DEFAULT_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}

	bool isConnected = false;
	// Step #3 Attempt to connect to an address until one succeeds
	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		connectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}


		// Connect to server.
		result = connect(connectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			closesocket(connectSocket);
			connectSocket = INVALID_SOCKET;
			continue;
		}

		result = ioctlsocket(connectSocket, FIONBIO, &mode);
		if (result != NO_ERROR) {
			printf("ioctlsocket failed with error: %ld\n", result);
			
		}
			isConnected = true;
		break;
	}

	freeaddrinfo(infoResult);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//Get login info from user
	//this is blocking, but that okay since we're no in a room yet
	std::string username = "";
	std::string roomname = "";
	std::cout << "Enter your username: ";
	std::cin >> username;
	std::cout << "Enter the name of the room you want to join: ";
	std::cin >> roomname;

	//assemble the protocol
	outgoing = ProtocolMethods::MakeProtocol(JOIN_ROOM, username, roomname, "");//this has no inherent message
	sProtocolData data = ProtocolMethods::ParseBuffer(outgoing);

	//change it to a format we can transport
	char* payload = outgoing.PayloadToString();
	//send it
	result = send(connectSocket, payload, outgoing.readUInt32BE(0), 0);
	if (result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	rooms.push_back(roomname);

	//clean up
	delete[] payload;
	printf("Bytes Sent: %ld\n", result);
	
	//start Client loop
	bool updateLog = false;
	bool quit = false;
	bool helpRequested = false;
	bool invalidCommand = false;
	while (!quit) {

		//get keyboard input
		if (_kbhit()) {
			char key = _getch();

			if (key == 27) { //esc to quit
				quit = true;
			}else if (key == 8) { //back to remove
				if (message.length() != 0)
				{
					message.pop_back();
					system("cls"); //supposedly this isn't a safe thing to do, but I'm pretty sure LG showed it in class
					updateLog = true;
				}
			}
			else if (key == 13) { // enter to send

				helpRequested = false; //remove the help
				
				if (message[0] == '/') // is a command
				{
					size_t pos = message.find(" ");
					std::string command = message.substr(0, pos);
					message.erase(0, pos + 1);

					if (command == "/help" || command == "/h") {
						helpRequested = true;
						updateLog;
					}else if (command == "/join" || command == "/j")
					{
						if (std::count(rooms.begin(), rooms.end(), message))
						{
							chatlog.push_back("You are already in the room " + message);
						}
						else
						{
							//assemble the protocol
							outgoing = ProtocolMethods::MakeProtocol(JOIN_ROOM, username, message, "");//this has no inherent message
							sProtocolData data = ProtocolMethods::ParseBuffer(outgoing);

							//change it to a format we can transport
							char* payload = outgoing.PayloadToString();
							//send it
							result = send(connectSocket, payload, outgoing.readUInt32BE(0), 0);
							if (result == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}

							rooms.push_back(message);

							//clean up
							delete[] payload;
							printf("Bytes Sent: %ld\n", result);
						}
					}
					else if (command == "/message" || command == "/m")
					{
						pos = message.find(" ");
						std::string room = message.substr(0, pos);
						message.erase(0, pos + 1);

						if (std::count(rooms.begin(), rooms.end(), room) == 0)
						{
							chatlog.push_back("You currently aren't in the room " + room);
						}
						else
						{
							//call to asseble the protocol
							outgoing = ProtocolMethods::MakeProtocol(SEND_MESSAGE, username, room, message);
							

							//change it to a form we can transport
							char* payload = outgoing.PayloadToString();
							//send it
							result = send(connectSocket, payload, outgoing.readUInt32BE(0), 0);

							if (result == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}

							//clean up
							delete[] payload;
							printf("Bytes Sent: %ld\n", result);
						}
					}
					else if (command == "/leave" || command == "/l")
					{
						if (std::count(rooms.begin(), rooms.end(), message) == 0)
						{
							chatlog.push_back("You currently aren't in the room " + message);
						}
						else
						{
							//Leave
							outgoing = ProtocolMethods::MakeProtocol(LEAVE_ROOM, username, message, "");

							//change it to a format we can transport
							char* leavePayload = outgoing.PayloadToString();
							//send it
							result = send(connectSocket, leavePayload, outgoing.readUInt32BE(0), 0);
							if (result == SOCKET_ERROR)
							{
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(connectSocket);
								WSACleanup();
								return 1;
							}

							//clean up
							delete[] leavePayload;
							printf("Bytes Sent: %ld\n", result);
							rooms.erase(std::find(rooms.begin(), rooms.end(), message));
						}
					}
				}
				else{ invalidCommand = true; }
				message = "";
				updateLog = true;
			}
			else {
				message.push_back(key);
				system("cls"); //supposedly this isn't a safe thing to do, but I'm pretty sure LG showed it in class
				updateLog = true;				
			}
		}

		if (isConnected) {
			result = recv(connectSocket, recvbuf, recvbuflen, 0);
			if (result > 0)
			{
				
				//get the inbound message, and put it in the buffer
				printf("Bytes received: %d\n", result);
				std::string received = "";
				for (int i = 0; i < recvbuflen; i++) {
					received.push_back(recvbuf[i]);
				}

				incoming.LoadBuffer(received);

				//Parse the data in the buffer

				sProtocolData data = ProtocolMethods::ParseBuffer(incoming);

				//if it comes from a room that we're in, add it to the chat log
				system("cls"); //supposedly this isn't a safe thing to do, but I'm pretty sure LG showed it in class
				for (int i = 0; i < rooms.size(); i++) {
					if (rooms[i] == data.room) { 
						chatlog.push_back("<" + data.room + "> " + data.userName + ":\t" + data.message);
					}
					updateLog = true;
				}
			}
			else if (result == 0)
			{
				//if we've left
				printf("Connection closed\n");
			}
			else
			{
				if (WSAGetLastError() == 10035) {
					//this error isn't actually bad, it just means we've not received anything
				}
				else {
					printf("Message error happend, opps. time to exit");
					quit = true;
				}
			}

			//print out the chat log, if it need to be updated
			if (updateLog) {
				for (int i = 0; i < chatlog.size(); i++) {
					std::cout << chatlog[i] << std::endl;
				}
				std::cout << std::endl;
				//User feedback
				if (invalidCommand) {
					printf("\x1B[91m%s\033[0m\n", "!Invalid Command!");
					invalidCommand = false;
				}
				if (helpRequested) {
					//print out list of commands
					std::cout << "/help: get list of commands" << std::endl;
					std::cout << "/join [room]: joins the room" << std::endl;
					std::cout << "/message [room]: sends a message to the room" << std::endl;
					std::cout << "/leave [room]: leaves the room" << std::endl;
					//make sure this isn't always on
					//helpRequested = false;
				}
				else {
					std::cout << "type /help for a list of commands" << std::endl;
				}
				//user input
				std::cout << "message: ";
				std::cout << message << std::endl;
				updateLog = false;
			}

		}

	}

	for (int i = 0; i < rooms.size(); i++)
	{
		//Leave
		outgoing = ProtocolMethods::MakeProtocol(LEAVE_ROOM, username, rooms[i], "");//this has no inherent message
		sProtocolData leaveData = ProtocolMethods::ParseBuffer(outgoing);

		//change it to a format we can transport
		char* leavePayload = outgoing.PayloadToString();
		//send it
		result = send(connectSocket, leavePayload, outgoing.readUInt32BE(0), 0);
		if (result == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		//clean up
		delete[] leavePayload;
		printf("Bytes Sent: %ld\n", result);
	}

	// Step #5 shutdown the connection since no more data will be sent
	result = shutdown(connectSocket, SD_SEND);
	if (result == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	// Step #7 cleanup
	closesocket(connectSocket);
	WSACleanup();

	system("pause");

	return 0;
}
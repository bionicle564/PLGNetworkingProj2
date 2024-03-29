#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

//server_main.cpp
//Gian Tullo, 0886424 / Lucas Magalhaes /Philip Tomaszewski
//23/10/21
//A chatroom server, allowing for the sending and receiving of messages

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <Buffer.h>
#include <ProtocolHelper.h>
#include "../gen/account.pb.h"
//#include "../gen/CreateAccount.pb.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"
#define DEFAULT_AUTH_PORT "27016"
#define SERVER "127.0.0.1"	

Buffer outgoing(DEFAULT_BUFLEN);
Buffer ingoing(DEFAULT_BUFLEN);
Buffer authentication(DEFAULT_BUFLEN);

char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

// Client structure
struct ClientInfo
{
	SOCKET socket;

	// Buffer information (this is basically you buffer class)
	WSABUF dataBuf;
	char buffer[DEFAULT_BUFLEN];
	int bytesRECV;
};

int TotalClients = 0;
ClientInfo* ClientArray[FD_SETSIZE];

void RemoveClient(int index)
{
	ClientInfo* client = ClientArray[index];
	closesocket(client->socket);
	printf("Closing socket %d\n", (int)client->socket);

	for (int clientIndex = index; clientIndex < TotalClients; clientIndex++)
	{
		ClientArray[clientIndex] = ClientArray[clientIndex + 1];
	}

	TotalClients--;

	// We also need to cleanup the ClientInfo data
	// TODO: Delete Client
	delete client;
}

int main(int argc, char** argv)
{
	WSADATA wsaData;
	SOCKET authenticationSocket = INVALID_SOCKET;

	struct addrinfo* infoResult = NULL;
	struct addrinfo* ptr = NULL;
	u_long mode = 1;

	int iResult;
	int result;


	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		// Something went wrong, tell the user the error id
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}
	else
	{
		printf("WSAStartup() was successful!\n");
	}

	// #1 Socket
	SOCKET listenSocket = INVALID_SOCKET;
	SOCKET acceptSocket = INVALID_SOCKET;

	struct addrinfo* addrResult = NULL;
	struct addrinfo hints;

	// Define our connection address info 
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &addrResult);
	if (iResult != 0)
	{
		printf("getaddrinfo() failed with error %d\n", iResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("getaddrinfo() is good!\n");
	}

	result = getaddrinfo(SERVER, DEFAULT_AUTH_PORT, &hints, &infoResult);
	if (result != 0)
	{
		printf("getaddrinfo failed with error: %d\n", result);
		WSACleanup();
		return 1;
	}

	//tutorial::CreateAccountWeb newAccount;

	//newAccount.set_requestid(1);
	//newAccount.set_plaintextpassword("password");
	//newAccount.set_email("spam@abc.com");

	//std::string serialized;
	//newAccount.SerializeToString(&serialized);

	//tutorial::CreateAccountWeb created;
	//if (!created.ParseFromString(serialized))
	//{
	//	std::cout << "Parsing failed" << std::endl;
	//}
	//else
	//{
	//	std::cout << created.requestid() << std::endl;
	//	std::cout << created.email() << std::endl;
	//	std::cout << created.plaintextpassword() << std::endl;
	//}

	//=================

	bool isConnected = false;
	// Step #3 Attempt to connect to an address until one succeeds
	for (ptr = infoResult; ptr != NULL; ptr = ptr->ai_next)
	{
		// Create a SOCKET for connecting to server
		authenticationSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

		if (authenticationSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}


		// Connect to server.
		result = connect(authenticationSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (result == SOCKET_ERROR)
		{
			closesocket(authenticationSocket);
			authenticationSocket = INVALID_SOCKET;
			continue;
		}

		result = ioctlsocket(authenticationSocket, FIONBIO, &mode);
		if (result != NO_ERROR)
		{
			printf("ioctlsocket failed with error: %ld\n", result);

		}
		isConnected = true;
		break;
	}

	freeaddrinfo(infoResult);

	if (authenticationSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	//result = send(authenticationSocket, "hi there", 9, 0); //just to see if it connects

	//===========================





	// Create a SOCKET for connecting to the server
	listenSocket = socket(
		addrResult->ai_family,
		addrResult->ai_socktype,
		addrResult->ai_protocol
	);
	if (listenSocket == INVALID_SOCKET)
	{
		// -1 -> Actual Error Code
		// https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
		printf("socket() failed with error %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("socket() is created!\n");
	}

	// #2 Bind - Setup the TCP listening socket
	iResult = bind(
		listenSocket,
		addrResult->ai_addr,
		(int)addrResult->ai_addrlen
	);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(addrResult);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("bind() is good!\n");
	}

	// We don't need this anymore
	freeaddrinfo(addrResult);

	// #3 Listen
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	else
	{
		printf("listen() was successful!\n");
	}

	// Change the socket mode on the listening socket from blocking to
	// non-blocking so the application will not block waiting for requests
	DWORD NonBlock = 1;
	iResult = ioctlsocket(listenSocket, FIONBIO, &NonBlock);
	if (iResult == SOCKET_ERROR)
	{
		printf("ioctlsocket() failed with error %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	printf("ioctlsocket() was successful!\n");

	FD_SET ReadSet;
	int total;
	
	DWORD RecvBytes;
	DWORD SentBytes;

	printf("Entering accept/recv/send loop...\n");
	while (true)
	{
		timeval tv = { 0 };
		tv.tv_sec = 2;
		// Initialize our read set
		FD_ZERO(&ReadSet);

		// Always look for connection attempts
		FD_SET(listenSocket, &ReadSet);

		// Set read notification for each socket.
		for (int i = 0; i < TotalClients; i++)
		{
			FD_SET(ClientArray[i]->socket, &ReadSet);
		}

		// Call our select function to find the sockets that
		// require our attention
		printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			printf("select() is successful!\n");
		}

		// #4 Check for arriving connections on the listening socket
		if (FD_ISSET(listenSocket, &ReadSet))
		{
			total--;
			acceptSocket = accept(listenSocket, NULL, NULL);
			if (acceptSocket == INVALID_SOCKET)
			{
				printf("accept() failed with error %d\n", WSAGetLastError());
				return 1;
			}
			else
			{
				iResult = ioctlsocket(acceptSocket, FIONBIO, &NonBlock);
				if (iResult == SOCKET_ERROR)
				{
					printf("ioctsocket() failed with error %d\n", WSAGetLastError());
				}
				else
				{
					printf("ioctlsocket() success!\n");

					ClientInfo* info = new ClientInfo();
					info->socket = acceptSocket;
					info->bytesRECV = 0;
					ClientArray[TotalClients] = info;
					TotalClients++;
					printf("New client connected on socket %d\n", (int)acceptSocket);
				}
			}
		}

		// #5 recv & send
		for (int i = 0; i < TotalClients; i++)
		{
			ClientInfo* client = ClientArray[i];

			// If the ReadSet is marked for this socket, then this means data
			// is available to be read on the socket
			if (FD_ISSET(client->socket, &ReadSet))
			{
				total--;
				client->dataBuf.buf = client->buffer;
				client->dataBuf.len = DEFAULT_BUFLEN;

				DWORD Flags = 0;
				iResult = WSARecv(
					client->socket,
					&(client->dataBuf),
					1,
					&RecvBytes,
					&Flags,
					NULL,
					NULL
				);



				std::string received = "";

				for (DWORD i = 0; i < RecvBytes; i++) {
					received.push_back(client->dataBuf.buf[i]);
				}

				ingoing.LoadBuffer(received);

				sProtocolData data = ProtocolMethods::ParseBuffer(ingoing);

				//setup buffer for messages to the clients
				if (data.type == JOIN_ROOM)
				{
					outgoing = ProtocolMethods::MakeProtocol(RECV_MESSAGE, "Server", data.room, data.message);
				}
				else if (data.type == SEND_MESSAGE)
				{
					outgoing = ProtocolMethods::MakeProtocol(RECV_MESSAGE, data.userName, data.room, data.message);
				}
				else if (data.type == LEAVE_ROOM)
				{
					outgoing = ProtocolMethods::MakeProtocol(RECV_MESSAGE, "Server", data.room, data.message);
				}
				else if (data.type == LOGIN_USER)
				{
					//send data to auth server with g-protocols
					// for now, just reply saying they logged in
					//send(authenticationSocket, "hi there", 9, 0);

					//data: username	-> the name they will use to sign in
					//		room		-> the password they will use 


					//the return message should have 'yes' for the room if logged in sucessfully
					
					tutorial::AuthenticateWeb userLogin;

					userLogin.set_requestid(i);
					userLogin.set_email(data.userName);
					userLogin.set_plaintextpassword(data.room);

					std::string serialized;
					userLogin.SerializeToString(&serialized);

					authentication = ProtocolMethods::MakeProtocol(G_AUTHENTICATE, "", "", serialized);

					char* payload = authentication.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = authentication.readUInt32BE(0);

					iResult = WSASend(
						authenticationSocket,
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					// Example using send instead of WSASend...
					//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

					if (SentBytes == SOCKET_ERROR)
					{
						printf("send error %d\n", WSAGetLastError());
					}
					else if (SentBytes == 0)
					{
						printf("Send result is 0\n");
					}
					else
					{
						printf("Successfully sent %d bytes!\n", SentBytes);
					}

					delete[] payload; //clean the payload
				}
				else if (data.type == REGISTER_USER)
				{
					//see comments above
					//outgoing = ProtocolMethods::MakeProtocol(LOGIN_USER, "Server", "yes", "Logged in succesfull");
					tutorial::CreateAccountWeb newAccount;

					newAccount.set_requestid(i);
					newAccount.set_email(data.userName);
					newAccount.set_plaintextpassword(data.room);

					std::string serialized;
					newAccount.SerializeToString(&serialized);

					authentication = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT, "", "", serialized);

					char* payload = authentication.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = authentication.readUInt32BE(0);
					
					iResult = WSASend(
						authenticationSocket,
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					// Example using send instead of WSASend...
					//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

					if (SentBytes == SOCKET_ERROR)
					{
						printf("send error %d\n", WSAGetLastError());
					}
					else if (SentBytes == 0)
					{
						printf("Send result is 0\n");
					}
					else
					{
						printf("Successfully sent %d bytes!\n", SentBytes);
					}
					
					delete[] payload; //clean the payload
				}

				
				std::cout << "RECVd: " << received << std::endl;

				if (iResult == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEWOULDBLOCK)
					{
						// We can ignore this, it isn't an actual error.
					}
					else
					{
						printf("WSARecv failed on socket %d with error: %d\n", (int)client->socket, WSAGetLastError());
						RemoveClient(i);
					}
				}
				else
				{
					printf("WSARecv() is OK!\n");
					if (RecvBytes == 0)
					{
						RemoveClient(i);
					}
					else if (RecvBytes == SOCKET_ERROR)
					{
						printf("recv: There was an error..%d\n", WSAGetLastError());
						continue;
					}
					else
					{
						char* payload = outgoing.PayloadToString();
						WSABUF buf;
						buf.buf = payload;
						buf.len = outgoing.readUInt32BE(0);

						for (int i = 0; i < TotalClients; i++)
						{
							iResult = WSASend(
								ClientArray[i]->socket,
								&(buf), 
								1,
								&SentBytes,
								Flags,
								NULL,
								NULL
							);

							// Example using send instead of WSASend...
							//int iSendResult = send(client->socket, client->dataBuf.buf, iResult, 0);

							if (SentBytes == SOCKET_ERROR)
							{
								printf("send error %d\n", WSAGetLastError());
							}
							else if (SentBytes == 0)
							{
								printf("Send result is 0\n");
							}
							else
							{
								printf("Successfully sent %d bytes!\n", SentBytes);
							}
						}
						delete[] payload; //clean the payload
					}
				}
			}
		}

		result = recv(authenticationSocket, recvbuf, recvbuflen, 0);
		if (result > 0)
		{

			//get the inbound message, and put it in the buffer
			printf("Bytes received: %d\n", result);
			std::string received = "";
			for (int i = 0; i < recvbuflen; i++) {
				received.push_back(recvbuf[i]);
			}

			ingoing.LoadBuffer(received);

			//Parse the data in the buffer

			sProtocolData data = ProtocolMethods::ParseBuffer(ingoing);
			
			if (data.type == G_CREATE_ACCOUNT_SUCCESS)
			{
				tutorial::CreateAccountWebSuccess createSuccess;

				if (!createSuccess.ParseFromString(data.message))
				{
					std::cout << "Parsing failed" << std::endl;
				}
				else
				{
					outgoing = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT_SUCCESS, "", "", data.message);

					char* payload = outgoing.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = outgoing.readUInt32BE(0);

					DWORD Flags = 0;
					iResult = WSASend(
						ClientArray[createSuccess.requestid()]->socket, // CHANGE TO CLIENT SOCKET
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					delete[] payload;
				}
			}
			else if (data.type == G_CREATE_ACCOUNT_FAILURE)
			{
				tutorial::CreateAccountWebFailure createFailure;

				if (!createFailure.ParseFromString(data.message))
				{
					std::cout << "Parsing failed" << std::endl;
				}
				else
				{
					if (createFailure.returncode() == tutorial::CreateAccountWebFailure::ACCOUNT_ALREADY_EXISTS)
						outgoing = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT_FAILURE, "", "", "ACCOUNT_ALREADY_EXISTS");
					else if (createFailure.returncode() == tutorial::CreateAccountWebFailure::INTERNAL_SERVER_ERROR)
						outgoing = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT_FAILURE, "", "", "INTERNAL_SERVER_ERROR");

					char* payload = outgoing.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = outgoing.readUInt32BE(0);

					DWORD Flags = 0;
					iResult = WSASend(
						ClientArray[createFailure.requestid()]->socket, // CHANGE TO CLIENT SOCKET
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					delete[] payload;
				}
			}
			else if (data.type == G_AUTHENTICATE_SUCCESS)
			{
				tutorial::CreateAccountWebSuccess authenticateSuccess;

				if (!authenticateSuccess.ParseFromString(data.message))
				{
					std::cout << "Parsing failed" << std::endl;
				}
				else
				{
					outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_SUCCESS, "", "", data.message);

					char* payload = outgoing.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = outgoing.readUInt32BE(0);

					DWORD Flags = 0;
					iResult = WSASend(
						ClientArray[authenticateSuccess.requestid()]->socket, // CHANGE TO CLIENT SOCKET
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					delete[] payload;
				}
			}
			else if (data.type == G_AUTHENTICATE_FAILURE)
			{
				tutorial::CreateAccountWebFailure authenticateFailure;

				if (!authenticateFailure.ParseFromString(data.message))
				{
					std::cout << "Parsing failed" << std::endl;
				}
				else
				{
					if (authenticateFailure.returncode() == tutorial::AuthenticateWebFailure::INTERNAL_SERVER_ERROR)
						outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_FAILURE, "", "", "INTERNAL_SERVER_ERROR");
					else if (authenticateFailure.returncode() == tutorial::AuthenticateWebFailure::INVALID_CREDENTIALS)
						outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_FAILURE, "", "", "INVALID_CREDENTIALS");
					else if (authenticateFailure.returncode() == tutorial::AuthenticateWebFailure::INVALID_PASSWORD)
						outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_FAILURE, "", "", "INVALID_PASSWORD");

					char* payload = outgoing.PayloadToString();
					WSABUF buf;
					buf.buf = payload;
					buf.len = outgoing.readUInt32BE(0);

					DWORD Flags = 0;
					iResult = WSASend(
						ClientArray[authenticateFailure.requestid()]->socket, // CHANGE TO CLIENT SOCKET
						&(buf),
						1,
						&SentBytes,
						Flags,
						NULL,
						NULL
					);

					delete[] payload;
				}
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
			}
		}
	}

	// #6 close
	iResult = shutdown(acceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(acceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(acceptSocket);
	WSACleanup();

	return 0;
}
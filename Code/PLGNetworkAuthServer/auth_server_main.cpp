#define WIN32_LEAN_AND_MEAN			// Strip rarely used calls

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
#include "SaltHasher.h"
#include "DBHelper.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016" //new port number for this server

Buffer outgoing(DEFAULT_BUFLEN);
Buffer ingoing(DEFAULT_BUFLEN);

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
	DBHelper db;
	db.Connect("localhost:3306", "root", "password");

	WSADATA wsaData;
	int iResult;

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
		//printf("Waiting for select()...\n");
		total = select(0, &ReadSet, NULL, NULL, &tv);
		if (total == SOCKET_ERROR)
		{
			printf("select() failed with error: %d\n", WSAGetLastError());
			return 1;
		}
		else
		{
			//printf("select() is successful!\n");
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

				for (DWORD i = 0; i < RecvBytes; i++)
				{
					received.push_back(client->dataBuf.buf[i]);
				}

				ingoing.LoadBuffer(received);

				sProtocolData data = ProtocolMethods::ParseBuffer(ingoing);

				//=====================put parsing ptrotobuffs here=================
				if (data.type == G_CREATE_ACCOUNT)
				{
					tutorial::CreateAccountWeb created;
					if (!created.ParseFromString(data.message))
					{
						std::cout << "Parsing failed" << std::endl;
					}
					else
					{
						std::string email = created.email();
						std::string password = created.plaintextpassword();

						// sql call
						SaltHasher sh;
						std::string salt = sh.GenerateSalt(64);
						std::string saltedPassword = salt + password;
						std::string hashedPassword = sh.HashPassword(saltedPassword);

						if (db.IsConnected())
						{
							DatabaseResponse response = db.CreateAccount(email, hashedPassword, salt);
							if (response.result == DatabaseReturnCode::SUCCESS)
							{
								tutorial::CreateAccountWebSuccess successResponse;

								successResponse.set_requestid(created.requestid());
								successResponse.set_userid(response.userId);

								std::string successString;
								successResponse.SerializeToString(&successString);

								outgoing = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT_SUCCESS, "", "", successString);
							}
							else
							{
								tutorial::CreateAccountWebFailure failureResponse;

								failureResponse.set_requestid(created.requestid());

								if (response.result == DatabaseReturnCode::ACCOUNT_ALREADY_EXISTS)
									failureResponse.set_returncode(tutorial::CreateAccountWebFailure::ACCOUNT_ALREADY_EXISTS);
								else if (response.result == DatabaseReturnCode::INTERNAL_SERVER_ERROR)
									failureResponse.set_returncode(tutorial::CreateAccountWebFailure::INTERNAL_SERVER_ERROR);

								std::string failureString;
								failureResponse.SerializeToString(&failureString);

								outgoing = ProtocolMethods::MakeProtocol(G_CREATE_ACCOUNT_FAILURE, "", "", failureString);
							}
						}
					}					
				}
				else if (data.type == G_AUTHENTICATE)
				{
					tutorial::AuthenticateWeb login;

					if (!login.ParseFromString(data.message))
					{
						std::cout << "Parsing failed" << std::endl;
					}
					else
					{
						std::string email = login.email();
						std::string password = login.plaintextpassword();

						// sql call
						if (db.IsConnected())
						{
							DatabaseResponse response = db.LoginUser(email, password);
							if (response.result == DatabaseReturnCode::SUCCESS)
							{
								tutorial::AuthenticateWebSuccess successResponse;

								successResponse.set_requestid(login.requestid());
								successResponse.set_userid(response.userId);
								successResponse.set_creationdate(response.date);

								std::string successString;
								successResponse.SerializeToString(&successString);

								outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_SUCCESS, "", "", successString);
							}
							else
							{
								tutorial::AuthenticateWebFailure failureResponse;

								failureResponse.set_requestid(login.requestid());

								if (response.result == DatabaseReturnCode::INVALID_PASSWORD)
									failureResponse.set_returncode(tutorial::AuthenticateWebFailure::INVALID_PASSWORD);
								else if (response.result == DatabaseReturnCode::INVALID_CREDENTIAL)
									failureResponse.set_returncode(tutorial::AuthenticateWebFailure::INVALID_CREDENTIALS);
								else if (response.result == DatabaseReturnCode::INTERNAL_SERVER_ERROR)
									failureResponse.set_returncode(tutorial::AuthenticateWebFailure::INTERNAL_SERVER_ERROR);

								std::string failureString;
								failureResponse.SerializeToString(&failureString);

								outgoing = ProtocolMethods::MakeProtocol(G_AUTHENTICATE_FAILURE, "", "", failureString);
							}
						}
					}

				}


				//std::cout << "RECVd: " << received << std::endl;

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

						iResult = WSASend(
							client->socket,
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
				}
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
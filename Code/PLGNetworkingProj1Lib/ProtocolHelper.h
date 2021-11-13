#pragma once

//ProtocolHelper.h
//Gian Tullo, 0886424 / Lucas Magalhaes / Philip Tomaszewski
//23/10/21
//Outlines our protocols

#define DEFAULT_BUFLEN 512

#include <string>
#include "Buffer.h"

enum ProtocolType
{
	JOIN_ROOM = 1,
	LEAVE_ROOM,
	SEND_MESSAGE,
	RECV_MESSAGE,
	REGISTER_USER,
	LOGIN_USER,
	G_CREATE_ACCOUNT,
	G_CREATE_ACCOUNT_SUCCESS,
	G_CREATE_ACCOUNT_FAILURE,
	G_AUTHENTICATE,
	G_AUTHENTICATE_SUCCESS,
	G_AUTHENTICATE_FAILURE,
};

//structure of data from a protocol
struct sProtocolData
{
	ProtocolType type;
	std::string userName;
	std::string room;
	std::string message;
};


class ProtocolMethods
{
	ProtocolMethods() = delete;

public:
	//assembles a buffer using the protocal data
	static Buffer MakeProtocol(ProtocolType type, std::string name, std::string room, std::string message);

	//Retreives data from the buffer into a more usable format
	static sProtocolData ParseBuffer(Buffer input);
};





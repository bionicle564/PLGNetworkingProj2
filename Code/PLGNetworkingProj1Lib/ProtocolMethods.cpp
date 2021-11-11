#include "ProtocolHelper.h"

//ProtocolMethods.cpp
//Gian Tullo, 0886424 / Lucas Magalhaes / Philip Tomaszewski
//23/10/21
//Provides utility methods for the protocal
//  see header for method manifest

//Protocol structure
/*
	JOIN_ROOM :  [header][len][name][len][roomName]
	LEAVE_ROOM:  [header][len][name][len][roomName]
	SEND_MSG:    [header][len][name][len][roomName][len][message]
	RECV_MSG:    [header][len][name][len][roomName][len][message]
*/

Buffer ProtocolMethods::MakeProtocol(ProtocolType type, std::string name, std::string room, std::string message)
{
	Buffer tempBuf(DEFAULT_BUFLEN);

	if (type == JOIN_ROOM)
	{
		tempBuf.writeUInt32BE(name.length());
		tempBuf.writeUInt8BE(name);

		tempBuf.writeUInt32BE(room.length());
		tempBuf.writeUInt8BE(room);

		//length of everything, including header, which is 8 bytes (2 ints)
		int length = tempBuf.GetWriteIndex() + 8;
		tempBuf.writeUInt32BE(0, length);

		tempBuf.writeUInt32BE(4, JOIN_ROOM);

		
	}
	else if (type == LEAVE_ROOM)
	{
		tempBuf.writeUInt32BE(name.length());
		tempBuf.writeUInt8BE(name);

		tempBuf.writeUInt32BE(room.length());
		tempBuf.writeUInt8BE(room);

		//length of everything, including header, which is 8 bytes (2 ints)
		int length = tempBuf.GetWriteIndex() + 8;
		tempBuf.writeUInt32BE(0, length);

		tempBuf.writeUInt32BE(4, LEAVE_ROOM);

	}
	else if (type == SEND_MESSAGE)
	{
		tempBuf.writeUInt32BE(name.length());
		tempBuf.writeUInt8BE(name);

		tempBuf.writeUInt32BE(room.length());
		tempBuf.writeUInt8BE(room);

		tempBuf.writeUInt32BE(message.length());
		tempBuf.writeUInt8BE(message);

		int length = tempBuf.GetWriteIndex() + 8;
		tempBuf.writeUInt32BE(0, length);

		tempBuf.writeUInt32BE(4, SEND_MESSAGE);
	}
	else if (type == RECV_MESSAGE) 
	{
		tempBuf.writeUInt32BE(name.length());
		tempBuf.writeUInt8BE(name);

		tempBuf.writeUInt32BE(room.length());
		tempBuf.writeUInt8BE(room);

		tempBuf.writeUInt32BE(message.length());
		tempBuf.writeUInt8BE(message);

		int length = tempBuf.GetWriteIndex() + 8;
		tempBuf.writeUInt32BE(0, length);

		tempBuf.writeUInt32BE(4, RECV_MESSAGE);
	}
	else if (type == LOGIN_USER)
	{
		tempBuf.writeUInt32BE(name.length());
		tempBuf.writeUInt8BE(name);

		tempBuf.writeUInt32BE(room.length());
		tempBuf.writeUInt8BE(room);

		tempBuf.writeUInt32BE(message.length());
		tempBuf.writeUInt8BE(message);

		int length = tempBuf.GetWriteIndex() + 8;
		tempBuf.writeUInt32BE(0, length);

		tempBuf.writeUInt32BE(4, LOGIN_USER);
	}
	return tempBuf;
}

sProtocolData ProtocolMethods::ParseBuffer(Buffer input)
{
	sProtocolData data;
	if (input.GetBuffer().size() == 0) //size check
	{
		return data;
	}
	
	int length;
	length = input.readUInt32BE();
	data.type = (ProtocolType)input.readUInt32BE();

	if (data.type == JOIN_ROOM)
	{
		int itemLength = input.readUInt32BE();
		data.userName = input.readUInt8BE(itemLength);

		itemLength = input.readUInt32BE();
		data.room = input.readUInt8BE(itemLength);

		//construct the message
		data.message = data.userName + " entered " + data.room + " room";
	}
	else if (data.type == LEAVE_ROOM)
	{
		int itemLength = input.readUInt32BE();
		data.userName = input.readUInt8BE(itemLength);

		itemLength = input.readUInt32BE();
		data.room = input.readUInt8BE(itemLength);

		//construct the message
		data.message = data.userName + " left the " + data.room + " room";
	}
	else if (data.type == SEND_MESSAGE) 
	{
		uint32_t nameLength = input.readUInt32BE();
		data.userName = input.readUInt8BE(nameLength);
		uint32_t roomLength = input.readUInt32BE();
		data.room = input.readUInt8BE(roomLength);
		uint32_t msgLength = input.readUInt32BE();
		data.message = input.readUInt8BE(msgLength);
	}
	else if (data.type == RECV_MESSAGE)
	{

		uint32_t nameLength = input.readUInt32BE();
		data.userName = input.readUInt8BE(nameLength);
		uint32_t roomLength = input.readUInt32BE();
		data.room = input.readUInt8BE(roomLength);
		uint32_t msgLength = input.readUInt32BE();
		data.message = input.readUInt8BE(msgLength);
	}
	else if (data.type == LOGIN_USER)
	{

		uint32_t nameLength = input.readUInt32BE();
		data.userName = input.readUInt8BE(nameLength);
		uint32_t roomLength = input.readUInt32BE();
		data.room = input.readUInt8BE(roomLength);
		uint32_t msgLength = input.readUInt32BE();
		data.message = input.readUInt8BE(msgLength);
	}

	return data;
}
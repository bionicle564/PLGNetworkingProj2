//Buffer.h
//Gian Tullo, 0886424 / Lucas Magalhaes /Philip Tomaszewski
//23/10/21
//Outlines the functionality of a TCP Packet buffer

#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>



class Buffer {
private:
	int indexRead;
	int indexWrite;
	std::vector<uint8_t> buffer;

public:
	//constructor
	Buffer(std::size_t size);

	//Write and Reading Unsigned int 32
	void writeUInt32BE(std::size_t index, int32_t value);
	void writeUInt32BE(int32_t value);
	uint32_t readUInt32BE(std::size_t index);
	uint32_t readUInt32BE();

	//Write and Reading Unsigned int 16
	void writeUInt16BE(std::size_t index, int16_t value);
	void writeUInt16BE(int16_t value);
	uint16_t readUInt16BE(std::size_t index);
	uint16_t readUInt16BE();

	//Write and Reading Unsigned int 8 (AKA a char)
	void writeUInt8BE(std::size_t index, std::string value);
	void writeUInt8BE(std::string value);
	std::string readUInt8BE(std::size_t index, int stringSize);
	std::string readUInt8BE(int stringSize);

	//buffer getter
	std::vector<uint8_t> GetBuffer();
	//index getter
	int GetWriteIndex();

	//converts the buffer to a char array
	char* PayloadToString();

	//loads the buffer
	void LoadBuffer(std::string);

	//flushes the buffer
	void ClearBuffer();
};

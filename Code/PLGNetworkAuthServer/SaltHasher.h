#pragma once

//SaltHasher.h
//Gian Tullo
//today
//this generates salts and hashes strings

#include <string>
#include <array>

class SaltHasher {
public:
	SaltHasher();

	std::string GenerateSalt(int length);
	std::string HashPassword(std::string saltyPassword);
private:
	char allChars[36] = { '0', '1', '2', '3', '4',
						  '5', '6', '7', '8', '9', 
						  'A', 'B', 'C', 'D', 'E',
						  'F', 'G', 'H', 'I', 'J', 
						  'K', 'L', 'M', 'N', 'O', 
						  'P', 'Q', 'R', 'S', 'T', 
						  'U', 'V', 'W', 'X', 'Y', 
						  'Z' };

	char GetRandomChar();
};
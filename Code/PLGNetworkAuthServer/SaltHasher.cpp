#include "SaltHasher.h"

#include <cstdlib>

#include "SHA256.h";

char SaltHasher::GetRandomChar() {
	return allChars[rand() % 36];
}

SaltHasher::SaltHasher() {

}

std::string SaltHasher::GenerateSalt(int length) {
	std::string salt = "";
	for (int i = 0; i < length; i++) {
		salt += GetRandomChar();
	}
	return salt;
}

std::string SaltHasher::HashPassword(std::string saltyPassword) {
	std::string passwordCopy = saltyPassword;
	SHA256 sha;
	sha.update(passwordCopy);
	uint8_t* digest = sha.digest();

	std::string hashedPassword = SHA256::toString(digest);

	delete[] digest;

	return hashedPassword;
}
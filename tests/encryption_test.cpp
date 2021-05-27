#include "../encryption.h"
#include <iostream>

#define verify(cond) if (!(cond)) {std::cout << "--- Test failed at: " << __FILE__ << ", Line: " << __LINE__ << " ---" << std::endl; }

void onerror(const unsigned char error) {
    std::cout << "[ERROR] ";
    switch (error) {
        default:
            std::cout << "Unhandled error id: " << error;
            break;
    }

    std::cout << std::endl;
}

void ondebug(const char *msg) {
    std::cout << "[DEBUG] " << msg << std::endl;
} 


void testEncryption() {
    unsigned char key[CHACHA_KEY_SIZE] = {
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
		0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
		0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    };

    Encryption e(key, ondebug, onerror);

    // this is what we would like to encrypt
    const char *msg = "Hello";
    int msgSize = strlen(msg);

    // this will hold the encrypted cypher text
    int encryptedSize = e.calc_cipherstream_size(msgSize);
    unsigned char encrypted[encryptedSize];
	memset(encrypted, 0, encryptedSize*sizeof(unsigned char));

    // do the encryption
    verify(e.encrypt((uint8_t*)msg, msgSize, encrypted, encryptedSize));

    int decryptedSize = e.calc_plaintext_size(encryptedSize);
    uint8_t decrypted[decryptedSize];
    memset(decrypted, 0, decryptedSize*sizeof(uint8_t));

    verify(e.decrypt(encrypted, encryptedSize, decrypted, decryptedSize));
}


int main() {
    std::cout << "--- STARTING encryption tests ---" << std::endl;
    testEncryption();
    std::cout << "--- FINISHED encryption tests ---" << std::endl;
    return 0;
}
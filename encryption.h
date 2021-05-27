#ifndef MARCONI_ENCRYPTION_H_
#define MARCONI_ENCRYPTION_H_

#include <stdio.h>
#include <ChaChaPoly.h>
#include <string.h>
#include <stdlib.h>
#include "eventing.h"

// chacha poly related
#define CHACHA_KEY_SIZE 32
#define CHACHA_IV_SIZE 12
#define CHACHA_AUTH_TAG_SIZE 16

// this class can be used in order to encrypt or decrypt messages sent via marconi protocol
class Encryption{
	public:
		Encryption(uint8_t key[CHACHA_KEY_SIZE], debugCallback *debug_callback, errorCallback *error_callback);
		~Encryption() {
			delete iv_;
		};
		
		// used to encrypt a message
		bool encrypt(uint8_t *in, int in_size, uint8_t *out, int out_size);
		// given the length of plaintext to encrypt this func will calculate the size of the resulting cipherstream
		int calc_cipherstream_size(int plaintext_size);

		bool decrypt(uint8_t *in, int in_size, uint8_t *out, int out_size);
		// given the size of a cipher stream this func will calculate the resulting plaintext size
		int calc_plaintext_size(int cipherstream_size);
	private:
		Eventing *eventing_;

		// encryption relevant variables
		ChaChaPoly chaChaPoly_;
		uint8_t *key_;
		uint8_t *iv_;
		bool overrideIv_;

		// bumps up the iv by one bit
		void increaseIv();
		void increaseIvPos(int pos);

		void randomBytes(uint8_t *arr, size_t size);
		uint8_t randomChar();
};

const uint8_t kErrorInvalidPlaintextSize = 0x01;
const uint8_t kErrorInvalidCipherstreamSize = 0x02;

#endif // MARCONI_ENCRYPTION_H_

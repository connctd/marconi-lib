#include "encryption.h"

// constructor
Encryption::Encryption(uint8_t key[CHACHA_KEY_SIZE], debugCallback *debug_callback, errorCallback *error_callback) {
	eventing_ = new Eventing(debug_callback, error_callback);
	iv_ = new uint8_t[CHACHA_IV_SIZE]();

	// init iv
	randomBytes(iv_, CHACHA_IV_SIZE);
	eventing_->debug("Generating initialization vector:");
	eventing_->debugBytes(iv_, CHACHA_IV_SIZE);

	key_ = key;
	eventing_->debug("Using key:");
	eventing_->debugBytes(key_, CHACHA_KEY_SIZE);

	overrideIv_ = false;
}

bool Encryption::encrypt(uint8_t *in, int in_size, uint8_t *out, int out_size) {
	eventing_->debug("(encrypt) Encrypting message:");
	eventing_->debugBytes(in, in_size);

	if (out_size < 0) {
		eventing_->error(kErrorInvalidCipherstreamSize);
		return false;
	}

	// increase iv by on bit
	increaseIv();

	eventing_->debug("(encrypt) Generated IV:");
	eventing_->debugBytes(iv_, CHACHA_IV_SIZE);
	
	// tmp storage for auth tag
	uint8_t tag[CHACHA_AUTH_TAG_SIZE] = {};
	// tmp storage for cipher text
	uint8_t cipher_text[in_size];
	memset(cipher_text, 0, in_size*sizeof(uint8_t));

    chaChaPoly_.clear();
    chaChaPoly_.setKey(key_, CHACHA_KEY_SIZE);
    chaChaPoly_.setIV(iv_, CHACHA_IV_SIZE);

	chaChaPoly_.encrypt(cipher_text, in, in_size);
    chaChaPoly_.computeTag(tag, CHACHA_AUTH_TAG_SIZE);

	eventing_->debug("(encrypt) Computed auth tag:");
	eventing_->debugBytes(tag, CHACHA_AUTH_TAG_SIZE);
	
	eventing_->debug("(encrypt) Cipher text:");
	eventing_->debugBytes(cipher_text, in_size);

	// append nonce to result
	for (int i = 0; i < CHACHA_IV_SIZE; i++) {
		out[i] = iv_[i];
	}

	// append ciphertext
	for (int i = 0; i < in_size; i++) {
		out[CHACHA_IV_SIZE+i] = cipher_text[i];
	}

	// append tag to cipher text
	for (int i = 0; i < CHACHA_AUTH_TAG_SIZE; i++) {
		out[CHACHA_IV_SIZE+in_size+i] = tag[i];
	}
	
	eventing_->debug("(encrypt) Encrypted cipherstream:");
	eventing_->debugBytes(out, out_size);

	return true;
}

int Encryption::calc_cipherstream_size(int plaintext_size) {
	return CHACHA_IV_SIZE*sizeof(uint8_t) + plaintext_size + CHACHA_AUTH_TAG_SIZE*sizeof(uint8_t);
}

bool Encryption::decrypt(uint8_t *in, int in_size, uint8_t *out, int out_size) {
	eventing_->debug("(decrypt) Decrypting cipherstream");
	eventing_->debugBytes(in, in_size);

	if (out_size < 0) {
		eventing_->error(kErrorInvalidPlaintextSize);
		return false;
	}

	uint8_t iv[CHACHA_IV_SIZE] = {};
	uint8_t tag[CHACHA_AUTH_TAG_SIZE] = {};
	
	uint8_t cipher_text[out_size];
	memset(cipher_text, 0, out_size*sizeof(uint8_t));

	// extract nonce
	for (int i = 0; i < CHACHA_IV_SIZE; i++) {
		iv[i] = in[i];
	}

	// extract ciphertext
	for (int i = 0; i < out_size; i++) {
		cipher_text[i] = in[CHACHA_IV_SIZE+i];
	}

	// extract auth tag
	for (int i = 0; i < CHACHA_AUTH_TAG_SIZE; i++) {
		tag[i] = in[CHACHA_IV_SIZE+out_size+i];
	}

	eventing_->debug("(decrypt) Extracted iv:");
	eventing_->debugBytes(iv, CHACHA_IV_SIZE);

	eventing_->debug("(decrypt) Extracted ciphertext:");
	eventing_->debugBytes(cipher_text, out_size);

	eventing_->debug("(decrypt) Extracted auth tag:");
	eventing_->debugBytes(tag, CHACHA_AUTH_TAG_SIZE);

    chaChaPoly_.clear();
    chaChaPoly_.setKey(key_, CHACHA_KEY_SIZE);
    chaChaPoly_.setIV(iv, CHACHA_IV_SIZE);
	chaChaPoly_.decrypt(out, cipher_text, out_size);

	eventing_->debug("(decrypt) Decrypted:");
	eventing_->debugBytes(out, out_size);

	return true;
}

int Encryption::calc_plaintext_size(int cipherstream_size) {
	return cipherstream_size-CHACHA_IV_SIZE*sizeof(uint8_t)-CHACHA_AUTH_TAG_SIZE*sizeof(uint8_t);
}

void Encryption::increaseIv() {
	increaseIvPos(CHACHA_IV_SIZE-1);
}

void Encryption::increaseIvPos(int pos) {
	if (pos < 0) {
		// roll over iv
		iv_[CHACHA_IV_SIZE-1] = 0x01;
	} else if (iv_[pos] < 0xff) {
		// just bump up current pos
		iv_[pos] += 0x01;
	} else {
		// jump to next pos
		iv_[pos] = 0x00;
		increaseIvPos(pos - 1);
	}
}


void Encryption::randomBytes(uint8_t *arr, size_t size) {
    for (size_t i = 0; i < size; i++) {
		// pick random int between 0 and 256 and return last byte
        arr[i] = (rand()%256) & 0xFF;
	}
}

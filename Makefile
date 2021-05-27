ARDUINO_LIBS="/home/sgarn/Arduino/libraries"

test:
# Build dependencies
	g++ \
	-Wextra -Wall -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -flto -c eventing.cpp -o eventing.o

# Encryption tests
	g++ \
	-I$(ARDUINO_LIBS)/Crypto  \
	-I$(ARDUINO_LIBS)/Crypto/src \
	-Wextra -Wall -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -flto -c encryption.cpp -o encryption.o

	g++ \
	-I$(ARDUINO_LIBS)/Crypto  \
	-I$(ARDUINO_LIBS)/Crypto/src \
	-Wextra -Wall -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -flto tests/encryption_test.cpp -o encryption_test.o \
	encryption.o \
	eventing.o \
	$(ARDUINO_LIBS)/Crypto/src/SHA512.o \
	$(ARDUINO_LIBS)/Crypto/src/XOF.o \
	$(ARDUINO_LIBS)/Crypto/src/KeccakCore.o \
	$(ARDUINO_LIBS)/Crypto/src/P521.o \
	$(ARDUINO_LIBS)/Crypto/src/AES128.o \
	$(ARDUINO_LIBS)/Crypto/src/SHA256.o \
	$(ARDUINO_LIBS)/Crypto/src/Poly1305.o \
	$(ARDUINO_LIBS)/Crypto/src/BlockCipher.o \
	$(ARDUINO_LIBS)/Crypto/src/Ed25519.o \
	$(ARDUINO_LIBS)/Crypto/src/ChaCha.o \
	$(ARDUINO_LIBS)/Crypto/src/Cipher.o \
	$(ARDUINO_LIBS)/Crypto/src/AES192.o \
	$(ARDUINO_LIBS)/Crypto/src/OMAC.o \
	$(ARDUINO_LIBS)/Crypto/src/XTS.o \
	$(ARDUINO_LIBS)/Crypto/src/ChaChaPoly.o \
	$(ARDUINO_LIBS)/Crypto/src/BLAKE2s.o \
	$(ARDUINO_LIBS)/Crypto/src/GHASH.o \
	$(ARDUINO_LIBS)/Crypto/src/AES256.o \
	$(ARDUINO_LIBS)/Crypto/src/GF128.o \
	$(ARDUINO_LIBS)/Crypto/src/AESEsp32.o \
	$(ARDUINO_LIBS)/Crypto/src/BigNumberUtil.o \
	$(ARDUINO_LIBS)/Crypto/src/EAX.o \
	$(ARDUINO_LIBS)/Crypto/src/Crypto.o \
	$(ARDUINO_LIBS)/Crypto/src/CTR.o \
	$(ARDUINO_LIBS)/Crypto/src/NoiseSource.o \
	$(ARDUINO_LIBS)/Crypto/src/SHA3.o \
	$(ARDUINO_LIBS)/Crypto/src/RNG.o \
	$(ARDUINO_LIBS)/Crypto/src/SHAKE.o \
	$(ARDUINO_LIBS)/Crypto/src/AESCommon.o \
	$(ARDUINO_LIBS)/Crypto/src/AuthenticatedCipher.o \
	$(ARDUINO_LIBS)/Crypto/src/BLAKE2b.o \
	$(ARDUINO_LIBS)/Crypto/src/Hash.o \
	$(ARDUINO_LIBS)/Crypto/src/Curve25519.o \
	$(ARDUINO_LIBS)/Crypto/src/GCM.o \
	$(ARDUINO_LIBS)/EpoxyDuino/cores/epoxy/Arduino.o
	./encryption_test.o

# Parser tests
	g++ \
	-Wextra -Wall -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -flto -c parser.cpp -o parser.o

	g++ \
	-Wextra -Wall -std=gnu++11 -fno-exceptions -fno-threadsafe-statics -flto tests/parser_test.cpp -o parser_test.o \
	eventing.o \
	parser.o
	./parser_test.o

clean:
	rm encryption_test.o
	rm encryption.o
	rm eventing.o
	rm parser.o
	rm parser_test.o

	
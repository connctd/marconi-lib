#include "eventing.h"
#include <stdio.h>
#include <string.h>

void Eventing::debug(const char *msg) {
	if (debug_callback_) {
		debug_callback_(msg);
	}
}

void Eventing::error(unsigned char err) {
	if (error_callback_) {
		error_callback_(err);
	}
}

void Eventing::debugBytes(unsigned char *b, int len) {
	// we are generating 2 chars per byte
    char buf[2*len];
	memset(buf, 0, 2*len*sizeof(char));

	char tmp[4];

	for (int i = 0; i < len; i++) {
		sprintf(tmp, "%02X", b[i]);
		strcat(buf, tmp);
	}
	
	debug_callback_(buf);
}

void Eventing::debugByte(unsigned char b) {
	char tmp[4];

	sprintf(tmp, "%02X", b);

	debug_callback_(tmp);
}

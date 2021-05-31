#ifndef MARCONI_PARSER_H_
#define MARCONI_PARSER_H_

#include <inttypes.h>
#include <string.h>
#include "eventing.h"

// chacha poly related
#define SESSION_SIZE 8          // 8 bytes

struct ActionRequest {  
    unsigned char id;  
    char *value;
    long action_counter_id;
    unsigned char session_id[SESSION_SIZE];

    ~ActionRequest() {
        if ( value ) delete[] value;
        value = nullptr;
    }
};  

class Parser{
	public:
        Parser(debugCallback *debug_callback, errorCallback *error_callback);

        // builds a byte array from given property; return value needs to be deleted after it was used; also return value
        // has to be encrypted before sending it
        uint8_t* buildPropertyUpdate(long counter_id, uint8_t session_id[SESSION_SIZE], uint8_t property_id, char *value);

        // builds an action subscription
        uint8_t* buildActionSubscription(long counter_id, uint8_t session_id[SESSION_SIZE]);

        // ActionRequest needs to be deleted after it was used; payload needs to be decrypted first
        ActionRequest parseAction(uint8_t *payload, uint8_t payloadlen);
    private:
        Eventing *eventing_;
};

const uint8_t kErrorActionParsingBadLength = 0x11;

const uint8_t kSubscription = 0xFF;


#endif // MARCONI_PARSER_H_

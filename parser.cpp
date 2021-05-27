#include "parser.h"
#include <iostream>

Parser::Parser(debugCallback *debug_callback, errorCallback *error_callback) {
	eventing_ = new Eventing(debug_callback, error_callback);
}

uint8_t* Parser::buildPropertyUpdate(long counter_id, unsigned char session_id[SESSION_SIZE], char property_id[PROPERTY_ID_SIZE], char *value) {
    // holds serialized update
    int size = sizeof(long)+SESSION_SIZE+PROPERTY_ID_SIZE+strlen(value)*sizeof(char);
    uint8_t *out = new uint8_t[size]();
    memset(out, 0, size);

    // copy counter id to out
    uint8_t *point = out;
    memcpy(point, &counter_id, sizeof(long));
    
    // copy session id to out
    point += sizeof(long);
    memcpy(point, session_id, SESSION_SIZE);

    // copy property id to out
    point += SESSION_SIZE;
    memcpy(point, property_id, strlen(property_id)*sizeof(char));

    // copy value to out; include 0 termination
    point += PROPERTY_ID_SIZE;
    memcpy(point, value, strlen(value)*sizeof(char));

    return out;
}

uint8_t* Parser::buildActionSubscription(long counter_id, unsigned char session_id[SESSION_SIZE]) {
    return buildPropertyUpdate(counter_id, session_id, "observe", "1");
}

ActionRequest Parser::parseAction(uint8_t *payload, uint8_t payloadlen) {
    int minimal_expected_size = ACTION_ID_SIZE+SESSION_SIZE+sizeof(long);

    // as value ensure we have also room for 0 termination character
    char *value = new char[payloadlen - minimal_expected_size + 1]();
    memset(value, 0, payloadlen - minimal_expected_size + 1);

    ActionRequest a = {
        {}, // id
        value, // value
        0, // action id
        {}, // session id
    };

    if (payloadlen < minimal_expected_size) {
        eventing_->error(kErrorActionParsingBadLength);
        return a;
    }

    // extract action counter id
    uint8_t *point = payload;
    memcpy(&a.action_counter_id, point, sizeof(long));

    // extract session
    point += sizeof(long);
    memcpy(&a.session_id, point, SESSION_SIZE);

    // extract action id
    point += SESSION_SIZE;
    memcpy(&a.id, point, ACTION_ID_SIZE);

    // extract the value
    point += ACTION_ID_SIZE;
    memcpy(a.value, point, payloadlen-minimal_expected_size);

    return a;
}
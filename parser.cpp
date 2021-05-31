#include "parser.h"
#include <iostream>

Parser::Parser(debugCallback *debug_callback, errorCallback *error_callback) {
	eventing_ = new Eventing(debug_callback, error_callback);
}

uint8_t* Parser::buildPropertyUpdate(long counter_id, uint8_t session_id[SESSION_SIZE], uint8_t property_id, char *value) {
    // holds serialized update
    int out_length = sizeof(long)+SESSION_SIZE*sizeof(uint8_t)+1*sizeof(uint8_t)+strlen(value)+1*sizeof(char);
    uint8_t *out = new uint8_t[out_length]();
    memset(out, 0, out_length*sizeof(uint8_t));

    // copy counter id to out
    uint8_t *point = out;
    memcpy(point, &counter_id, sizeof(long));
    
    // copy session id to out
    point += sizeof(long);
    memcpy(point, session_id, SESSION_SIZE);

    // copy property id to out
    point += SESSION_SIZE*sizeof(uint8_t);
    memcpy(point, &property_id, sizeof(uint8_t));

    // copy value to out; include 0 termination
    point += sizeof(uint8_t);
    memcpy(point, value, strlen(value)*sizeof(char));

    return out;
}

uint8_t* Parser::buildActionSubscription(long counter_id, uint8_t session_id[SESSION_SIZE]) {
    return buildPropertyUpdate(counter_id, session_id, kSubscription, "1");
}

ActionRequest Parser::parseAction(uint8_t *payload, uint8_t payloadlen) {
    int minimal_expected_length = sizeof(long)+SESSION_SIZE*sizeof(uint8_t)+1*sizeof(uint8_t);

    // as value ensure we have also room for 0 termination character
    char *value = new char[payloadlen - minimal_expected_length + 1]();
    memset(value, 0, payloadlen - minimal_expected_length + 1);

    ActionRequest a = {
        {}, // id
        value, // value
        0, // action id
        {}, // session id
    };

    if (payloadlen < minimal_expected_length) {
        eventing_->error(kErrorActionParsingBadLength);
        return a;
    }

    // extract action counter id
    uint8_t *point = payload;
    memcpy(&a.action_counter_id, point, sizeof(long));

    // extract session
    point += sizeof(long);
    memcpy(&a.session_id, point, SESSION_SIZE*sizeof(uint8_t));

    // extract action id
    point += SESSION_SIZE*sizeof(uint8_t);
    memcpy(&a.id, point, sizeof(uint8_t));

    // extract the value
    point += sizeof(uint8_t);
    memcpy(a.value, point, payloadlen-minimal_expected_length);

    return a;
}
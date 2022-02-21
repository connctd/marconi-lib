#include "marconi_client.h"

MarconiClient::MarconiClient(IPAddress ip, int port, char device_id[DEVICE_ID_SIZE], unsigned char key[CHACHA_KEY_SIZE], connectionStateCallback *connection_state_callback, debugCallback *debug_callback, errorCallback *error_callback) {
    instance = this;
    eventing_ = new Eventing(debug_callback, error_callback);
    parser_ = new Parser(debug_callback, error_callback);
    encr_ = new Encryption(key, debug_callback, error_callback);

    // limit device length
    device_id[DEVICE_ID_SIZE-1] = '\0';

    ip_ = ip;
    port_ = port;
    device_id_ = device_id;

    char *path_state = new char[24+DEVICE_ID_SIZE]();
    sprintf(path_state, "api/v1/devices/%s/state", device_id);

    char *path_session = new char[25+DEVICE_ID_SIZE];
    sprintf(path_session, "api/v1/devices/%s/session", device_id);

    char *path_action = new char[24+DEVICE_ID_SIZE];
    sprintf(path_action, "api/v1/devices/%s/action", device_id);

    path_state_ = path_state;
    path_session_ = path_session;
    path_action_ = path_action;

    eventing_->debug("Marconi library version");
    eventing_->debug(STRINGIFY(VERSION));

    eventing_->debug("Using the following paths");
    eventing_->debug(path_state_);
    eventing_->debug(path_session_);
    eventing_->debug(path_action_);

    session_id_ = new unsigned char[SESSION_SIZE]();

    connection_state_callback_ = connection_state_callback;

    coap_.response(serverCallback);

    eventing_->debug("Marconi lib setup completed");
}

MarconiClient::~MarconiClient() {
    delete parser_;
    delete encr_;
    delete eventing_;
    delete path_state_;
    delete path_session_;
    delete path_action_;
    delete session_id_;
}

// can be periodically called and triggers the session creation
// Note: after reinitializing a session you need to resubscribe for actions as
// encryption relevant data will change
void MarconiClient::init() {
    connection_state_callback_(kConnectionStateUninitialized);
    eventing_->debug("Sending session init");
    last_init_msg_id_ = coap_.get(ip_, port_, path_session_);
}

bool MarconiClient::loop() {
    return coap_.loop();
}

// can be used to send a property update
void MarconiClient::sendRawPropertyUpdate(uint8_t property_id, char *value) {
    eventing_->debug("Sending property update");

    int unencrypted_size = sizeof(long)+SESSION_SIZE*sizeof(uint8_t)+1*sizeof(uint8_t)+strlen(value)*sizeof(char);
    uint8_t *unencrypted = parser_->buildPropertyUpdate(property_counter_id_, session_id_, property_id, value);

    // calculate size of cipherstream
    int encrypted_length = encr_->calc_cipherstream_size(unencrypted_size);
    unsigned char encrypted[encrypted_length];
    memset(encrypted, 0, sizeof(unsigned char)*encrypted_length);

    // do the encryption
    if (!encr_->encrypt(unencrypted, unencrypted_size, encrypted, encrypted_length)) {
        eventing_->debug("Encryption of property update has failed");
        eventing_->error(kErrorEncryptionFailed);
    } else {
        property_counter_id_ += 1;
        uint8_t token=rand();
        eventing_->debug("Now sending property update");
        coap_.send(ip_, port_, path_state_, COAP_CON, COAP_POST, &token, sizeof(token), encrypted, encrypted_length, 0, NULL);
    }

    // free memory
    delete [] unencrypted;
}

void MarconiClient::sendFloatPropertyUpdate(uint8_t property_id, float value) {
    char buf[20];
    sprintf(buf, "%f", value);
    sendRawPropertyUpdate(property_id, buf);
}

void MarconiClient::sendBooleanPropertyUpdate(uint8_t property_id, bool value) {
    if (value) {
        char buf[2] = {0x31, 0x00};
        sendRawPropertyUpdate(property_id, buf);
    } else {
        char buf[2] = {0x30, 0x00};
        sendRawPropertyUpdate(property_id, buf);
    }
}

void MarconiClient::subscribeForActions(actionCallback *action_callback) {
    eventing_->debug("Subscribing for action");

    action_callback_ = action_callback;

    // inform about requested subscription
    connection_state_callback_(kConnectionObservationRequested);

    int unencrypted_size = sizeof(long)+SESSION_SIZE+2*sizeof(char);
    uint8_t *unencrypted = parser_->buildActionSubscription(property_counter_id_, session_id_);

    // calculate size of cipherstream
    int encrypted_size = encr_->calc_cipherstream_size(unencrypted_size);
    unsigned char encrypted[encrypted_size];
    memset(encrypted, 0, sizeof(unsigned char)*encrypted_size);

    // do the encryption
    if (!encr_->encrypt(unencrypted, unencrypted_size, encrypted, encrypted_size)) {
        eventing_->debug("Encryption of action subscription has failed");
        eventing_->error(kErrorEncryptionFailed);
    } else {
        property_counter_id_ += 1;

        // send observe message including encrypted payload
        uint8_t token=rand();
        eventing_->debug("Now sending subscription request");
        last_observe_msg_id_ = coap_.send(ip_, port_, path_action_, COAP_CON,COAP_GET, &token, sizeof(token), encrypted, encrypted_size, COAP_OBSERVE, NULL);
    }

    // free memory
    delete [] unencrypted;
}

void MarconiClient::handleServerMessage(coapPacket &packet, IPAddress ip, int port) {
    eventing_->debug("Server message received");

    // this is the response to our init request
    if (packet.messageid == last_init_msg_id_) {
        handleSessionResponse(packet, ip, port);

        // reset init msg id as the init request was obviously handled
        last_init_msg_id_ = -1;
    }
    // this is the response to our observation request
    else if (packet.messageid == last_observe_msg_id_) {
        handleObservationResponse(packet, ip, port);

        // reset observe msg id as the observation request was obviously handled
        last_observe_msg_id_ = -1;
    }
    else if (packet.type == COAP_CON || packet.type == COAP_NONCON) {
        if (packet.type == COAP_CON && packet.code == COAP_EMPTY) {
            handlePing(packet, ip, port);
        } else {
            // check for further handling
            handleRequest(packet, ip, port);
        }
    }
}

void MarconiClient::handleSessionResponse(coapPacket &packet, IPAddress ip, int port) {
    if (packet.code == kMessageCodeCreated) {
        eventing_->debug("Session Response received");
        if (packet.payloadlen < SESSION_SIZE) {
            eventing_->debug("Session has invalid size");
            eventing_->error(kErrorInvalidSessionSize);
        } else {
            eventing_->debug("Session was initialized");
            memcpy(session_id_, packet.payload, SESSION_SIZE*sizeof(unsigned char));
            last_action_counter_id_ = 0;
            property_counter_id_ = rand();
            connection_state_callback_(kConnectionStateInitialized);
        }
    } else {
        eventing_->debug("Session init rejected");
        connection_state_callback_(kConnectionStateInitRejected);
    }
}

void MarconiClient::handleObservationResponse(coapPacket &packet, IPAddress ip, int port) {
    if (packet.code == kMessageCodeCreated) {
        eventing_->debug("Observation accepted");
        connection_state_callback_(kConnectionObservationOngoing);
    } else {
        eventing_->debug("Observation rejected");
        connection_state_callback_(kConnectionObservationRejected);
    }
}

void MarconiClient::handlePing(coapPacket &packet, IPAddress ip, int port) {
    eventing_->debug("Ping received. Sending Pong");

    coapPacket p;

    // build ack
    p.type = COAP_ACK;
    p.code = COAP_EMPTY;
    p.token = NULL;
    p.tokenlen = 0;
    p.payload = NULL;
    p.payloadlen = 0;
    p.optionnum = 0;
    p.messageid = packet.messageid;

    eventing_->debug("Now sending pong");
    coap_.sendPacket(p, ip_, port_);
}


void MarconiClient::handleRequest(coapPacket &packet, IPAddress ip, int port) {
    eventing_->debug("Action request received");

    // we assume this is an action request
    if (packet.payloadlen > 0 && packet.code == kMessageCodeContent) {
        int decryptedSize = encr_->calc_plaintext_size((size_t)packet.payloadlen);
        
        // create array and init with 0
        uint8_t *decrypted = new uint8_t[decryptedSize]();
        memset(decrypted, 0, decryptedSize*sizeof(uint8_t));
        
        if (!encr_->decrypt(packet.payload, packet.payloadlen, decrypted, decryptedSize)) {
            eventing_->debug("Failed to decrypt action request");
            eventing_->error(kErrorDecryptionFailed);
        } else {
            
            ActionRequest a = parser_->parseAction(decrypted, decryptedSize);

            if (a.action_counter_id < last_action_counter_id_) {
                eventing_->debug("Action request rejected due to bad id");
                eventing_->error(kErrorActionRequestRejected);
            } else {
                last_action_counter_id_ = a.action_counter_id;
                
                if (action_callback_) {
                    action_callback_(a.id, a.value);
                } else {
                    eventing_->debug("No action callback set");
                }
            }
            
        }
        
        delete [] decrypted;
    }

    if (packet.type == COAP_CON) {
        eventing_->debug("Sending ack");
        coapPacket p;

        // build ack
        p.type = COAP_ACK;
        p.code = COAP_EMPTY;
        p.token = packet.token;
        p.tokenlen = packet.tokenlen;
        p.payload = NULL;
        p.payloadlen = 0;
        p.optionnum = 0;
        p.messageid = packet.messageid;

        eventing_->debug("Now sending ack");
        coap_.sendPacket(p, ip_, port_);
    }
}

// holds current instance
MarconiClient *instance;

void serverCallback(coapPacket &packet, IPAddress ip, int port) {
	instance->handleServerMessage(packet, ip, port);
}


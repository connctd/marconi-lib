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

    eventing_->debug("Using the following paths");
    eventing_->debug(path_state_);
    eventing_->debug(path_session_);
    eventing_->debug(path_action_);

    session_id_ = new unsigned char[SESSION_SIZE]();

    connection_state_callback_ = connection_state_callback;

    coap_.start(port);
    coap_.response(serverCallback);
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
void MarconiClient::init() {
    connection_state_callback_(kConnectionStateUninitialized);
    eventing_->debug("Sending session init");
    last_init_msg_id_ = coap_.get(ip_, port_, path_session_);
}

bool MarconiClient::loop() {
    return coap_.loop();
}

// can be used to send a property update
void MarconiClient::sendRawPropertyUpdate(char property_id[PROPERTY_ID_SIZE], char *value) {
    eventing_->debug("Sending raw value");

    int unencryptedSize = sizeof(long)+SESSION_SIZE+PROPERTY_ID_SIZE+strlen(value)*sizeof(char);
    uint8_t *unencrypted = parser_->buildPropertyUpdate(property_counter_id_, session_id_, property_id, value);

    // calculate size of cipherstream
    int encryptedSize = encr_->calc_cipherstream_size(unencryptedSize);
    unsigned char encrypted[encryptedSize];

	memset(encrypted, 0, encryptedSize*sizeof(unsigned char));

    // do the encryption
    bool ok = encr_->encrypt(unencrypted, unencryptedSize, encrypted, encryptedSize);
    if (!ok) {
        eventing_->debug("Encryption has failed");
    } else {
        property_counter_id_ += 1;
        coap_.send(ip_, port_, path_state_, COAP_CON, COAP_POST, NULL, 0, encrypted, encryptedSize, 0, NULL);
    }

    // free memory
    delete [] unencrypted;
}

void MarconiClient::sendFloatPropertyUpdate(const char property_id[PROPERTY_ID_SIZE], float value) {

}

void MarconiClient::subscribeForActions(actionCallback *action_callback) {
    action_callback_ = action_callback;
    connection_state_callback_(kConnectionObservationRequested);
    last_observe_msg_id_ = coap_.observe(ip_, port_, path_action_, 0);
}

void MarconiClient::handleServerMessage(coapPacket &packet, IPAddress ip, int port) {
    eventing_->debug("Server message received");

    if (packet.type == COAP_ACK) {
        eventing_->debug("ACK received");

        // this is the response to our init request
        if (packet.messageid == last_init_msg_id_) {
            if (packet.code == kCodeCreated) {
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
        // this is the response to our observation request
        else if (packet.messageid == last_observe_msg_id_) {
            if (packet.code == kCodeCreated) {
                eventing_->debug("Observation accepted");
                connection_state_callback_(kConnectionObservationOngoing);
            } else {
                eventing_->debug("Observation rejected");
                connection_state_callback_(kConnectionObservationRejected);
            }
        }
    } else if (packet.type == COAP_CON || packet.type == COAP_NONCON) {
        if (packet.type == COAP_CON) {
            eventing_->debug("Sending ACK");
            coapPacket *ack = buildAck(packet.messageid);
            coap_.sendPacket(*ack, ip, port);
            delete ack;
        }

        // interprete message as an action request
        if (packet.payloadlen > 0) {
            if (action_callback_ != NULL) {
                // TODO decrypt actions
            }
        }
    }
}

coapPacket *MarconiClient::buildAck(uint16_t messageid) {
  coapPacket packet;

  //make packet
  packet.type = COAP_ACK;
  packet.code = COAP_EMPTY;
  packet.token = NULL;
  packet.tokenlen = 0;
  packet.payload = NULL;
  packet.payloadlen = 0;
  packet.optionnum = 0;
  packet.messageid = messageid;

  return &packet;
}

// holds current instance
MarconiClient *instance;

void serverCallback(coapPacket &packet, IPAddress ip, int port) {
	instance->handleServerMessage(packet, ip, port);
}


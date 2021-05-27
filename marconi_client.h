#ifndef MARCONI_CLIENT_H_
#define MARCONI_CLIENT_H_

#include "eventing.h"
#include "encryption.h"
#include "parser.h"
#include "coap_client.h"

#define DEVICE_ID_SIZE 16 + 1 // since this is a string reserve one byte for 0 termination

typedef void connectionStateCallback(const unsigned char code);

typedef void actionCallback(char *actionId, char *value);

class MarconiClient{
	public:
 		MarconiClient(IPAddress ip, int port, char device_id[DEVICE_ID_SIZE], unsigned char key[CHACHA_KEY_SIZE], connectionStateCallback *connection_state_callback, debugCallback *debug_callback, errorCallback *error_callback);
		~MarconiClient();
		// can be periodically called and triggers the session creation
		void init();

		// has to be called in a loop
		bool loop();

		// takes care of handling incoming messages
		void handleServerMessage(coapPacket &packet, IPAddress ip, int port);

		// can be used to send a property update
		void sendRawPropertyUpdate(char property_id[PROPERTY_ID_SIZE], char *value);
		void sendFloatPropertyUpdate(const char property_id[PROPERTY_ID_SIZE], float value);

		void subscribeForActions(actionCallback *action_callback);

		
	private:
		// build and parse outgoing incoming datagrams
		Parser *parser_;

		// does cryptographic operations for us
		Encryption *encr_;

		// used to propagate errors and debug messages
		Eventing *eventing_;

		// used to interact with server
		IPAddress ip_;
		int port_;
		char *device_id_;

		// for property updates
		char *path_state_;

		// for initialization of session
		char *path_session_;

		// in order to subscribe for actions
		char *path_action_;

		// exchanged during init and send within every message
		unsigned char *session_id_;

		// bumped up whenever a property update is sent
		long property_counter_id_;

		// keeps track of last action server sent to us
		long last_action_counter_id_;

		// holds id of last observation request
		long last_observe_msg_id_;

		// lib stores last init msg id here
		uint16_t last_init_msg_id_;

		connectionStateCallback *connection_state_callback_;

		actionCallback *action_callback_;

		coapClient coap_;

		coapPacket *buildAck(uint16_t messageid);
};

/*
    Used coap library expects a pointer to function for callbacks. This we are
    working with classes we have to define a global variable which can be used in
    order to call the member function of our instance.
*/
extern MarconiClient *instance;
extern void serverCallback(coapPacket &packet, IPAddress ip, int port);

// sent when session initilization is ongoing or missing
const uint8_t kConnectionStateUninitialized = 0x01;

// event sent when session was successfully initialized
const uint8_t kConnectionStateInitialized = 0x02;

// init was rejected
const uint8_t kConnectionStateInitRejected = 0x03;

// sent when observation was requested
const uint8_t kConnectionObservationRequested = 0x06;

// sent when observation was successfully initialized
const uint8_t kConnectionObservationOngoing = 0x07;

// observation was rejected
const uint8_t kConnectionObservationRejected = 0x08;

// received session has invalid size
const uint8_t kErrorInvalidSessionSize = 0x41;

const uint8_t kCodeCreated = 0x41;

#endif // MARCONI_CLIENT_H_

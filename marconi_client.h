#ifndef MARCONI_CLIENT_H_
#define MARCONI_CLIENT_H_

#include "eventing.h"
#include "encryption.h"
#include "parser.h"
#include "coap_client.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_BUILD 1

#define _STRINGIFY(x) #x
#define STRINGIFY(x) _STRINGIFY(x)

#define VERSION VERSION_MAJOR.VERSION_MINOR.VERSION_BUILD


#define DEVICE_ID_SIZE 16 + 1 // since this is a string reserve one byte for 0 termination

typedef void connectionStateCallback(const unsigned char code);

typedef void actionCallback(uint8_t actionId, char *value);

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
		void sendRawPropertyUpdate(uint8_t property_id, char *value);
		void sendFloatPropertyUpdate(uint8_t property_id, float value);
		void sendBooleanPropertyUpdate(uint8_t property_id, bool value);

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

		// informs about connection state changes
		connectionStateCallback *connection_state_callback_;

		// is called whenever an action request was received
		actionCallback *action_callback_;

		// client used to send coap messages
		coapClient coap_;

		// packet handling functions
		void handleSessionResponse(coapPacket &packet, IPAddress ip, int port);
		void handleObservationResponse(coapPacket &packet, IPAddress ip, int port);
		void handlePing(coapPacket &packet, IPAddress ip, int port);
		void handleRequest(coapPacket &packet, IPAddress ip, int port);
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

const uint8_t kErrorDecryptionFailed = 0x42;
const uint8_t kErrorEncryptionFailed = 0x43;

// the action request was rejected presumably because of bad action id
const uint8_t kErrorActionRequestRejected = 0x44;

const uint8_t kMessageCodeCreated = 0x41;

// used for action requests
const uint8_t kMessageCodeContent = 0x45;

#endif // MARCONI_CLIENT_H_

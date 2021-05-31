#include "marconi_client.h"
#include <WiFi.h> // use for ESP32
//#include <ESP8266WiFi.h> // use for esp8266

// WiFi connection info
const char* ssid = "Hexentasse2";
const char* password = "affenwurst5";

// marconi settings
IPAddress ip(35, 205, 82, 53);
int port = 5683;

unsigned long resubscribeInterval = 60000; // in ms
unsigned long propertyUpdateInterval = 2000; // in ms

char device_id[DEVICE_ID_SIZE] = "0xl7n4igwd4k8g2t";  // unique device id

unsigned char key[CHACHA_KEY_SIZE] = {                // unique device key
  0x61, 0x3e, 0x28, 0x39, 0x88, 0x5d, 0xf2, 0xbe,
  0x74, 0x81, 0xb1, 0xc7, 0x3e, 0xe3, 0x8f, 0x36,
  0x19, 0x4f, 0xe0, 0xbc, 0xd3, 0xf2, 0x1d, 0xab,
  0x8a, 0x4c, 0x4a, 0x91, 0x7a, 0x97, 0x50, 0x5a 
};

// runtime vars
MarconiClient *c;
unsigned long lastResubscribe = 0; // periodically resubscribe
unsigned long lastPropertyUpdate = 0; // time when property updates were sent
bool initialized = false; // indicates if a session was initialized


void setup() {
  // initialize pseudo random number generator since tokens and cryptographic vectors are generated
  // this is crucial! Otherwise encrypted messages will look the same and connector might ignore
  // them due to caching
  //srand (analogRead(0)); // for ESP8266 you might need to use a lib

  srand(esp_random()); // for ESP32
  
  Serial.begin(115200);

  c = new MarconiClient(ip, port, device_id, key, onConnectionStateChange, onDebug, onErr);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currTime = millis();

    // exchange session
    if (!initialized) {
      blockingInitSession();
    }

    // resubscribe if necessary
    if (initialized && (currTime - lastResubscribe > resubscribeInterval || lastResubscribe == 0 )) {
      lastResubscribe = currTime;
      c->subscribeForActions(onAction);
    }

    // periodically send property updates
    if (initialized && currTime - lastPropertyUpdate > propertyUpdateInterval) {
      lastPropertyUpdate = currTime;
      //c->sendRawPropertyUpdate("state", "22.0");
      c->sendFloatPropertyUpdate("temperature", 22.0);
    }

    // wait for incoming messages
    c->loop();
  } else {
    // reconnect mechanism
    initialized = false;
    lastResubscribe = 0;
    
    Serial.println("No connection! Connect!");
    blockingConnectToWifi();
  }
}

// called whenever an action is invoked
void onAction(char *actionId, char *value) {
  Serial.println("Action called");
  Serial.println(actionId);
  Serial.println(value);
}

// called whenever marconi lib sends debug data
void onDebug(const char *msg) {
    //Serial.printf("[DEBUG] %s\n", msg);
} 

// called whenever connection state changes
void onConnectionStateChange(const unsigned char state) {
    Serial.printf("[CON] ");
    switch (state) {
      case kConnectionStateInitialized:
        Serial.println("Session was initialized");
        initialized = true;
        break;
      case kConnectionStateUninitialized:
        Serial.println("Session initialization ongoing");
        initialized = false;
        break;
      case kConnectionStateInitRejected:
        Serial.println("Session initialization has failed");
        initialized = false;
        break;
      case kConnectionObservationRequested:
        Serial.println("Observation was requested");
        break;
      case kConnectionObservationOngoing:
        Serial.println("Observation is now ongoing");
        break;
      case kConnectionObservationRejected:
        Serial.println("Observation was rejected");

        // reinit session in case connector was restarted
        initialized = false;

        // after reinit we want to resubscribe
        lastResubscribe = 0;
        break;
      default:
        Serial.printf("Unknown connection event %x\n", state);
        break;
    }
}

// called whenever an error occurs in marconi lib
void onErr(const unsigned char error) {
    Serial.printf("[ERROR] ");
    switch (error) {
        case kErrorInvalidPlaintextSize:
            Serial.println("Plaintext size too small");
            break;
        case kErrorInvalidCipherstreamSize:
            Serial.println("Encryption failed. Cipherstream too small");
            break;
        case kErrorActionRequestRejected:
            Serial.println("Received action request was rejected");
            break;
        case kErrorDecryptionFailed:
            Serial.println("Decryption error");
            break;
        case kErrorEncryptionFailed:
            Serial.println("Encryption error");
            break;
        default:
            Serial.printf("Unknown error event %x\n", error);
            break;
    }
}

// requestes a session id from connector which will be exchanged in every
// message to prevent replay attacks. Can be called multiple times
void blockingInitSession() {
  initialized = false;
  Serial.println("Initializing session");
  
  int retries = 0;
  c->init();
  while (!initialized) {
    if (retries > 10) {
      Serial.println("\nSession can not be established. Resending init");
      retries = 0;
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("No wifi connection. Abort session init");
        return;
      }
      c->init();
    }

    Serial.print(".");
    retries += 1;
    c->loop();
    delay(200);
  }

  Serial.println("");
}

// can be used to connect to wifi
void blockingConnectToWifi() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  int retries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    // SEEMS LIKE DEVICE MIGHT STAY HERE FOR AGES IN BUGGY CASES - reboot?
    Serial.print(".");
    delay(500);

    if (retries > 10) {
      Serial.println("Max retries reached - reinitialize connection");
      retries = 0;
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }

    retries += 1;
  }

  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Connected with wifi. IP: ");
  Serial.println(WiFi.localIP());
}
#include "marconi_client.h"
#include <ESP8266WiFi.h>

// WiFi connection info
const char* ssid = "321";
const char* password = "123";

// marconi settings
IPAddress ip(192,168,1,133);
int port = 5683;

unsigned long resubscribeInterval = 60000; // in ms
unsigned long propertyUpdateInterval = 10000; // in ms

char device_id[DEVICE_ID_SIZE] = "rqf8raquo2j3x5vc";  // unique device id

unsigned char key[CHACHA_KEY_SIZE] = {                // unique device key
  0x31,0x31,0x31,0x31,0x31,
  0x32,0x32,0x32,0x32,0x32,
  0x33,0x33,0x33,0x33,0x33,
  0x34,0x34,0x34,0x34,0x34,
  0x35,0x35,0x35,0x35,0x35,
  0x36,0x36,0x36,0x36,0x36,
  0x37,0x37,  
};

// runtime vars
MarconiClient *c;
unsigned long lastResubscribe = 0; // periodically resubscribe
unsigned long lastPropertyUpdate = 0; // time when property updates were sent
bool initialized = false; // indicates if a session was initialized

void setup() {
  // initialize pseudo random number generator since tokens are generated
  srand (analogRead(0));
  
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No connection! Connect!");
    digitalWrite(LED_BUILTIN, LOW);
    connectToWifi();
  }
  
  c = new MarconiClient(ip, port, device_id, key, onConnectionStateChange, onDebug, onErr);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currTime = millis();

    // exchange session
    if (!initialized) {
      initSession();
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
      c->sendFloatPropertyUpdate("state", 22.0);
    }

    // wait for incoming messages
    c->loop();
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

// called whenever marconi lib sends debug data
void onDebug(const char *msg) {
    Serial.printf("[DEBUG] %s\n", msg);
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
        break;
      default:
        Serial.printf("Unknown connection event %x\n", state);
        break;
    }
}

// called whenever an action is invoked
void onAction(char *actionId, char *value) {
  Serial.println("Action called");
  Serial.println(actionId);
  Serial.println(value);
}

// requestes a session id from connector which will be exchanged in every
// message to prevent replay attacks. Can be called multiple times
void initSession() {
  initialized = false;
  Serial.println("Initializing session");
  
  int retries = 0;
  c->init();
  while (!initialized) {
    if (retries > 10) {
      Serial.println("\nSession can not be established. Resending init");
      retries = 0;
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
void connectToWifi() {
  WiFi.disconnect();
  WiFi.begin(ssid, password);

  int retries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    // SEEMS LIKE DEVICE MIGHT STAY HERE FOR AGES IN BUGGY CASES - reboot?
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
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

  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Connected with wifi. IP: ");
  Serial.println(WiFi.localIP());
}
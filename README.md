# Marconi Lib

Used for communication between marconi connector and esp32. Message exchange is based on coap. Payloads are encrypted
with ChaCha20-Poly1305.

## Installation

`cd $HOME/Arduino/libraries/` and do `git clone git@github.com:connctd/marconi-lib.git`

# Usage

```
char device_id[DEVICE_ID_SIZE] = "rqf8raquo2j3x5vc";  // 16 characters long unique device id

unsigned char key[CHACHA_KEY_SIZE] = {                // 32 bytes key
  0x31,0x31,0x31,0x31,0x31,
  0x32,0x32,0x32,0x32,0x32,
  0x33,0x33,0x33,0x33,0x33,
  0x34,0x34,0x34,0x34,0x34,
  0x35,0x35,0x35,0x35,0x35,
  0x36,0x36,0x36,0x36,0x36,
  0x37,0x37,  
};

void setup() {
    // important: seed random number generate
    srand (analogRead(0));

    c = new MarconiClient(ip, port, device_id, key, onConnectionStateChange, onDebug, onErr);
}

void loop() {
    if (!initialized) {
        // call this to fetch a session id
        c->init();
    }

    // when initialized, periodically do things like

    c->subscribeForActions(onAction);   // subscribe for actions
    c->sendFloatPropertyUpdate("state", 22.0); // send updates

    // receive new udp messages
    c->loop(); 
}

// callback you have to pass
void onConnectionStateChange(const unsigned char state) {
    switch (state) {
      case kConnectionStateInitialized:
        initialized = true;
        break;
    ......
}

// called whenever an action is invoked
void onAction(unsigned char actionId, char *value) {
  Serial.printf("Action called. Id: %x Value: %s\n", actionId, value);
}

void onDebug(const char *msg) {
    Serial.printf("[DEBUG] %s\n", msg);
} 
```

## Testing

Some primitve tests are used to ensure parts of lib are working in expected manner.

```
cd $HOME/Arduino/libraries

# simulates core functionalities of arduino so you can execute tests on local machine (for testing)
git clone https://github.com/bxparks/EpoxyDuino.git
cd EpoxyDuino
make
```

Get further dependencies (see below) and do `make test` afterwards.

## Dependencies

```
cd $HOME/Arduino/libraries

# library that offers crypto functionalities
git clone https://github.com/OperatorFoundation/Crypto.git

# slightly modified coap lib
git clone git@github.com:connctd/ESP-CoAP.git
```


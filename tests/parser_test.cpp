#include "../parser.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#define verify(cond) if (!(cond)) {std::cout << "--- Test failed at: " << __FILE__ << ", Line: " << __LINE__ << " ---" << std::endl; }

void propertyTest() {
    std::cout << "--- RUNNING parser property tests ---" << std::endl;

    Parser p(NULL, NULL);
    // for testing purpose we just take a 0 terminated string as session id
    uint8_t *res = p.buildPropertyUpdate(123, (unsigned char*)"aaaaaaa", "bbbbbbbbbbbbbb", "ccc");

    uint8_t expected[35] = {
        0x7B,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x61,0x61,0x61,0x61,0x61,0x61,0x61,0x00,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x62,0x00,0x00,0x63,0x63,0x63,
    };

    // compare expected with result
    verify(memcmp(res, expected, 35) == 0);

    delete res;
}

void actionTest() {
    std::cout << "--- RUNNING parser action tests ---" << std::endl;

    // right now action messages are structurally build the same way like properties
    // for testing purposes build an property, serialize it and parse it as an action
    Parser p(NULL, NULL);
        // for testing purpose we just take a 0 terminated string as session id
    uint8_t *res = p.buildPropertyUpdate(123, (unsigned char*)"aaaaaaa", "bbbbbbbbbbbbbb", "ccc");

    ActionRequest a = p.parseAction(res, 35);
    verify(a.action_counter_id == 123);
    verify(strcmp((char*)a.session_id, "aaaaaaa") == 0);
    verify(strcmp(a.id, "bbbbbbbbbbbbbb") == 0);
    verify(strcmp(a.value, "ccc") == 0);

    delete res;
}

int main() {
    std::cout << "--- STARTING parser tests ---" << std::endl;
    propertyTest();
    actionTest();
    std::cout << "--- FINISHED parser tests ---" << std::endl;
    return 0;
}
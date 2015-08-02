#include "application.h"

SYSTEM_MODE(AUTOMATIC);

/**
* Declaring the variables.
*/
TCPClient client;
bool connected = false;

void setup() {
    Serial.begin(9600);
}

void loop() {
    if (!connected) {
        Serial.println();
        Serial.println("Application>\tStart of Loop.");
        // Request path and body can be set at runtime or at setup.
        String host = "www.apple.com";
        if (client.connect(host.c_str(), 80)) {
            Serial.println("Querying...");
            client.println("GET /library/test/success.html HTTP/1.1");
            client.println("Host: www.apple.com");
            client.println();
            connected = true;
        }
        else {
            Serial.println("Failure! " + host);
            delay(5000);
        }
    }
        
    if (connected && client.available())
    {
        char c = client.read();
        Serial.print(c);
    }
    
    if (connected && !client.connected())
    {
        Serial.println();
        Serial.println("disconnecting.");
        client.stop();
        delay(5000);
        connected = false;
    }
}
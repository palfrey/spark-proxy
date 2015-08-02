#include "application.h"

SYSTEM_MODE(AUTOMATIC);

TCPClient client;
bool connected = false;

const char* breakSequence = "\x1Bstop";
int breakLocation = 0;
int breakLength = strlen(breakSequence);

void setup() {
    Serial.begin(115200);
}

int readSerial() {
    int chr = Serial.read();
    if (connected && breakSequence[breakLocation] == chr) {
        breakLocation ++;
    }
    else {
        breakLocation = 0;
    }
    if (connected && breakLocation == breakLength) {
        client.stop();
        connected = false;
    }
    return chr;
}

const char* readWord() {
    String accumulated = "";
    while (true) {
        if (Serial.available()) {
            char incoming = readSerial();
            if (incoming == '\n' || incoming == ' ') {
                break;
            }
            accumulated += incoming;
        }
        else {
            delay(2); // arbitrary
        }
    }
    return accumulated.c_str();
}

void connect(String host, int port) {
    if (client.connect(host.c_str(), port)) {
        Serial.print("Connected to '");
        Serial.print(host);
        Serial.print("' and '");
        Serial.print(port);
        Serial.println("'");
        connected = true;
    }
    else {
        Serial.println("Failure! " + host);
    }
}

void loop () {
    if (Serial.available()) {
        if (connected) {
            int bytes = Serial.available();
            for (int i=0; i<bytes; i++) {
                client.write(readSerial());
            }
        }
        else {
            int cmd = readSerial();
            String host, stringPort;
            switch (cmd) {
                case 'i':
                    Serial.println("Ready");
                    break;
                case 'c':
                    readWord(); // junk space
                    host = readWord();
                    stringPort = readWord();
                    connect(host, stringPort.toInt());
                    break;
                default:
                    Serial.print("Don't know command: ");
                    Serial.write(cmd);
                    Serial.println();
                    break;
            }
        }
    }
    if (connected) {
        int bytes = client.available();
        for (int i=0; i<bytes; i++) {
            Serial.write(client.read());
        }
        
        if (!client.connected()) {
            Serial.println();
            Serial.println("Disconnecting");
            client.stop();
            connected = false;
        }
    }
}
#include "application.h"
#include "tinker.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

TCPClient client;
bool initialised = false;
bool connected = false;

const char* breakSequence = "\x1Bstop";
int breakLocation = 0;
int breakLength = strlen(breakSequence);

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

void setup() {
    Serial.begin(115200);
    WiFi.on();
    WiFi.connect();
}

void loop () {
    if (!initialised) {
        if (WiFi.ready()) {
            connect("www.apple.com", 80);
            if (connected) {
                client.println("GET /library/test/success.html HTTP/1.1");
                client.println("Host: www.apple.com");
                client.println();
                const char *success = "Success";
                int successLocation = 0;
                while (true) {
                    if (client.available()) {
                        int chr = client.read();
                        if (success[successLocation] == chr) {
                            successLocation ++;
                            if (successLocation == strlen(success)) {
                                initialised = true;
                                Spark.connect();
                                Spark.function("digitalread", tinkerDigitalRead);
                                Spark.function("digitalwrite", tinkerDigitalWrite);
                                Spark.function("analogread", tinkerAnalogRead);
                                Spark.function("analogwrite", tinkerAnalogWrite);
                                break;
                            }
                        }
                        else {
                            successLocation = 0;
                        }
                    }
                    else {
                        delay(2);
                    }
                }
            }
        }
        else {
            return;
        }
    }

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
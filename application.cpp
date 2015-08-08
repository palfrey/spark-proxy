#include "application.h"
#include "tinker.h"

SYSTEM_MODE(SEMI_AUTOMATIC);

TCPClient client;
bool initialised = false;
bool connected = false;

const char* breakSequence = "\x1Bstop";
unsigned int breakLocation = 0;
unsigned int breakLength = strlen(breakSequence);
unsigned long lastCheck = 0;

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

char* readWord() {
    const unsigned int BUFLEN = 255;
    char* accumulated = (char*) malloc(BUFLEN);
    unsigned int location = 0;
    while (true) {
        if (Serial.available()) {
            char incoming = readSerial();
            if (incoming == '\n' || incoming == ';') {
                break;
            }
            accumulated[location] = incoming;
            location ++;
        }
        else {
            delay(2); // arbitrary
        }
    }
    accumulated[location] = '\0';
    return accumulated;
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
        Serial.print("Failure! " + host + " ");
        Serial.println(port);
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.on();
    WiFi.connect();
}

void loop () {
    if (!initialised && !connected && (lastCheck + 5000 < millis())) {
        if (WiFi.ready()) {
            connect("www.apple.com", 80);
            if (connected) {
                client.println("GET /library/test/success.html HTTP/1.1");
                client.println("Host: www.apple.com");
                client.println();
                const char *success = "Success";
                unsigned int successLocation = 0;
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
                                client.stop();
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
            switch (cmd) {
                case 'i':
                    Serial.println("Ready");
                    break;
                case 'c': {
                    char* host;
                    char* stringPort;
                    readWord(); // junk space
                    host = readWord();
                    stringPort = readWord();
                    connect(host, atol(stringPort));
                    free(host);
                    free(stringPort);
                    break;
                }
                case 'w': {
                    char* ssid;
                    char* password;
                    readWord();
                    ssid = readWord();
                    password = readWord();
                    Serial.print("Wifi credentials set to SSID '");
                    Serial.print(ssid);
                    Serial.print("' and password '");
                    Serial.print(password);
                    Serial.println("'");
                    WiFi.setCredentials(ssid, password);
                    break;
                }
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
#include <stdlib.h>
#include <time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

byte inputs[] = { A0, A1, A2, A3, A4, A5 };
byte ioLength = sizeof(inputs) / sizeof(inputs[0]);

EthernetServer server(23);
EthernetClient client;

void setup() {
    for (byte i = 0; i < ioLength; i++) {
        pinMode(inputs[i], INPUT);

        byte output = i + 2;
        pinMode(output, OUTPUT);
        digitalWrite(output, EEPROM.read(output));
    }
    
    byte mac[6];
    srand(time(NULL));
    for (byte i = 0; i < 6; i++) {
        if (EEPROM.read(1000 + i) == 0) {
            EEPROM.update(1000 + i, rand() % 256);
        }

        mac[i] = EEPROM.read(1000 + i);
    }
    Ethernet.begin(mac);

    if (Ethernet.linkStatus() == LinkON) {
        server.begin();
    }
}

void loop() {
    for (byte i = 0; i < ioLength; i++) {
        if (digitalRead(inputs[i]) > LOW) {
            byte output = i + 2;
            byte newState = EEPROM.read(output) == LOW ? HIGH : LOW;
            changeState(output, newState);
      
            // wait until release
            while (digitalRead(inputs[i]) > LOW) {}
        }
    }

    if (Ethernet.linkStatus() == LinkOFF) {
        return;
    }

    if (!server) {
        server.begin();
    }

    EthernetClient newClient = server.accept();
    if (newClient) {
        client = newClient;
    }

    if (client && client.available()) {
        byte requestType = client.read();
        switch (requestType) {
            case 1:
                sendIOLength();
                break;
            case 2:
                sendInputs();
                break;
            case 3:
                sendOutputs();
                break;
            case 4:
                readPin();
                break;
            case 5:
                writePin();
                break;
        }
        client.flush();
        client.stop();
    }

    Ethernet.maintain();
}

void sendIOLength() {
    client.write(ioLength);
}

void sendInputs() {
    client.write(inputs, ioLength);
}

void sendOutputs() {
    byte outputs[ioLength];
    for (byte i = 0; i < ioLength; i++) {
        outputs[i] = i + 2;
    }
    
    client.write(outputs, ioLength);
}

void readPin() {
    byte pin = client.read();
    byte state = EEPROM.read(pin);
    client.write(state);
}

void writePin() {
    byte pin = client.read();
    byte newState = client.read();
    changeState(pin, newState);

    client.write((byte) 0);
}

void changeState(byte pin, byte newState) {
    digitalWrite(pin, newState);
    EEPROM.update(pin, newState);
}

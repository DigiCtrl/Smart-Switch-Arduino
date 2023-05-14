#include <stdlib.h>
#include <time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

const byte inputs[] = { A0, A1, A2, A3, A4, A5 };
const byte ioLength = sizeof(inputs) / sizeof(inputs[0]);
byte eepromLocation[ioLength];

byte mac[6];
EthernetServer server(23);
EthernetClient client;

void setup() {
    for (byte i = 0; i < ioLength; i++) {
        eepromLocation[i] = i + 2;
        byte value = EEPROM.read(eepromLocation[i]);
        for (byte j = 0; j < 5; j++) {
            if (value != EEPROM.read(i + 2)) {
                eepromLocation[i] += NUM_DIGITAL_PINS;
                break;
            }
        }
    }
    
    for (byte i = 0; i < ioLength; i++) {
        pinMode(inputs[i], INPUT);

        byte output = i + 2;
        pinMode(output, OUTPUT);
        digitalWrite(output, EEPROM.read(eepromLocation[i]));
    }

    pinMode(9, OUTPUT);
    digitalWrite(9, HIGH);

    srand(time(NULL));
    for (byte i = 0; i < 6; i++) {
        if (EEPROM.read(1000 + i) == 0) {
            EEPROM.update(1000 + i, rand() % 256);
        }

        mac[i] = EEPROM.read(1000 + i);
    }
    Ethernet.begin(mac);

    server.begin();
}

void loop() {
    for (byte i = 0; i < ioLength; i++) {
        if (digitalRead(inputs[i]) > LOW) {
            byte output = i + 2;
            byte newState = EEPROM.read(eepromLocation[i]) == LOW ? HIGH : LOW;
            changeState(output, newState);

            // wait until release
            while (digitalRead(inputs[i]) > LOW) {}
        }
    }

    if (Ethernet.linkStatus() == LinkOFF) {
        return;
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

    byte maintainResult = Ethernet.maintain();
    if (maintainResult == 1 || maintainResult == 3) {
        digitalWrite(9, LOW);
        delay(1);
        digitalWrite(9, HIGH);

        Ethernet.begin(mac);
        server.begin();
    }
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
    byte state = EEPROM.read(eepromLocation[pin - 2]);
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
    EEPROM.update(eepromLocation[pin - 2], newState);
}

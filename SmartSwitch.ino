#include <stdlib.h>
#include <time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

byte inputs[] = { A0, A1, A2, A3, A4, A5, 9 };
byte outputs[] = { 8, 7, 6, 5, 4, 3, 2 };
byte ioLength = sizeof(inputs) / sizeof(inputs[0]);

EthernetServer server(23);
EthernetClient client;

void setup() {
    Serial.begin(9600);

    for (byte i = 0; i < ioLength; i++) {
        pinMode(inputs[i], INPUT);
        pinMode(outputs[i], OUTPUT);
        digitalWrite(outputs[i], EEPROM.read(outputs[i]));
    }

    byte mac[6];
    for (byte i = 0; i < 6; i++) {
        if (EEPROM.read(1000 + i) == 0) {
            srand(time(NULL));
            EEPROM.update(1000 + i, rand() % 256);
        }

        mac[i] = EEPROM.read(1000 + i);
    }

    Ethernet.begin(mac);
    server.begin();
    Serial.println(Ethernet.localIP());
}

void loop() {
    EthernetClient newClient = server.accept();
    if (newClient) {
        client = newClient;
    }

    if (client && client.available()) {
        byte requestType = client.read();
        switch (requestType) {
            case 0:
                sendIOLength();
                break;
            case 1:
                sendInputs();
                break;
            case 2:
                sendOutputs();
                break;
            case 3:
                readPin();
                break;
            case 4:
                writePin();
                break;
            default:
                client.write(1);
                break;
        }
        client.stop();
    }

    for (byte i = 0; i < ioLength; i++) {
        if (inRead(inputs[i]) > LOW) {
            byte newState = EEPROM.read(outputs[i]) == LOW ? HIGH : LOW;
            changeState(outputs[i], newState);
      
            // wait until release
            while (inRead(inputs[i]) > LOW) {}
        }
    }
}

void sendIOLength() {
    Serial.println(ioLength);
    client.write(ioLength);
}

void sendInputs() {
    client.write(inputs, ioLength);
}

void sendOutputs() {
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

byte inRead(byte pinNumber) {
    if (pinNumber > NUM_DIGITAL_PINS) {
        return analogRead(pinNumber - NUM_DIGITAL_PINS);
    }
  
    return digitalRead(pinNumber);
}

void changeState(byte pin, byte newState) {
    digitalWrite(pin, newState);
    EEPROM.update(pin, newState);
}
#include "led.h"

byte LED_PINS[] = { RED_LED, YELLOW_LED, GREEN_LED };

void setup_led() {
  pinMode(YELLOW_LED_INPUT, INPUT);                             // fix hardware botch
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
}


void allLedOn() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(YELLOW_LED, HIGH);
  digitalWrite(GREEN_LED, HIGH);
}

void allLedOff() {
  digitalWrite(RED_LED, LOW);
  digitalWrite(YELLOW_LED, LOW);
  digitalWrite(GREEN_LED, LOW);
}

void setMailboxLEDBright(byte mailbox_number, boolean off_on) {
  digitalWrite(LED_PINS[mailbox_number], off_on);
}

void setMailboxLEDDim(byte mailbox_number, boolean off_on) {
  analogWrite(LED_PINS[mailbox_number], off_on ? 255 : 128);
}

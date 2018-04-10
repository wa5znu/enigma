#include <Wire.h>
#include <TWILiquidCrystal.h>
#include "Adafruit_FRAM_I2C.h"

#include "message.h"
#include "switch.h"

#define FRAM_MAX (32767)

Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();

void setup_fram() {
  if (! fram.begin(0x50)) {
    lcd.print(F("FRAM 0x51 FAILED"));
  } else if (! is_fram_consistent() || (read_switch() == STARTUP_CLEAR_FRAM_SWITCH)) {
    lcd.print(F("CLEARING FRAM"));
    clear_fram();
  }
}

void write_message_to_fram(byte mailbox_number) {
  byte *bp = (byte *) &message_buffer;
  int mailbox_offset = mailbox_number * sizeof(struct message);
  for (int i = 0; i < sizeof(struct message); i++) {
    fram.write8(i + mailbox_offset, *bp++);
  }
}

void read_message_from_fram(byte mailbox_number) {
  byte *bp = (byte *) &message_buffer;
  int mailbox_offset = mailbox_number * sizeof(struct message);
  for (int i = 0; i < sizeof(struct message); i++) {
    *bp++ = fram.read8(i + mailbox_offset);
  }
}

void write_message_read_flag_to_fram(byte mailbox_number) {
  int read_flag_offset = (mailbox_number * sizeof(struct message)) + ((byte *)(&message_buffer.read_flag) - ((byte *)&message_buffer));
  fram.write8(read_flag_offset, message_buffer.read_flag);
}


// unlike the other routines in this file, read_message_read_flag_from_fram does not read or write the current message_buffer
// it reads directly from FRAM and returns the value.  The current message_buffer may be for another mailbox.
// MESSAGE_FLAG_NONE, MESSAGE_FLAG_UNREAD, MESSAGE_FLAG_READ
char read_message_read_flag_from_fram(byte mailbox_number) {
  int read_flag_offset = (mailbox_number * sizeof(struct message)) + ((byte *)(&message_buffer.read_flag) - ((byte *)&message_buffer));
  return fram.read8(read_flag_offset);
}

void clear_message() {
  byte *bp = (byte *) &message_buffer;
  for (int i = 0; i < sizeof(struct message); i++) {
    *bp++ = '\0';
  }
}

bool message_valid() {
  return message_buffer.phone[0] != '\0' && message_buffer.date[0] != '\0' && message_buffer.text[0] != '\0' && message_buffer.read_flag != '\0';
}

byte unread_message_count() {
  byte k = 0;
  for (byte i = 0; i < NUMBER_OF_MAILBOXES; i++) {
    if (read_message_read_flag_from_fram(i) == MESSAGE_FLAG_UNREAD) {
      k++;
    }
  }
  return k;
   
}

void clear_fram() {
  // clear FRAM consistency flag in case this clear gets interrupted, so we'll do it again on restart.
  fram.write8(FRAM_MAX, 0x00);
  for (uint16_t i = 0; i < FRAM_MAX; i++) {
    fram.write8(i, 0);
  }
  fram.write8(FRAM_MAX, 0xA0);
}

boolean is_fram_consistent() {
  byte b = fram.read8(FRAM_MAX);
  return b == 0xA0;
}

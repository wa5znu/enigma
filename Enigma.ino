#include <Arduino.h>
#include <Wire.h>
#include <TWILiquidCrystal.h>
#include "Adafruit_FRAM_I2C.h"

#include "lcd.h"
#include "message.h"
#include "mailboxes.h"

// Message buffer to hold sender, phone number, and message in RAM on the way to/from FRAM or SMS.
// This buffer is big.  Only make one of them.
struct message message_buffer;

// LCD Message display states
#define MAILBOX_NO_MESSAGES 0
#define MAILBOX_SHOW_HEADER 1
#define MAILBOX_SHOWING_HEADER 2
#define MAILBOX_SHOW_MESSAGE 3
#define MAILBOX_SHOWING_MESSAGE 4
#define MAILBOX_SHOW_HELP 5
#define MAILBOX_SHOWING_HELP 6
#define MAILBOX_LCD_OFF 7

byte displayed_mailbox = NO_MAILBOX;
long last_backlight_on_time = 0;
long mailbox_display_state_time = 0;
byte mailbox_display_state = MAILBOX_SHOWING_HELP;


void setup() {

#if false
  // DEBUG -- not for release because it causes Leonardo to hang until you plug it in to USB
  while (! Serial)
    ;
  Serial.begin(115200);
  Serial.println(F("setup()"));
#endif

  setup_led();
  allLedOn();
  setup_lcd();
  setup_switch();
  clear_message();
  setup_fram();
  setup_gsmsms();
  allLedOff();
  send_boot_sms();
  mailbox_display_state_time = secs();
  lcd_show_help();
}

void loop() {
  loop_read_sms();
  loop_read_switches();
  loop_leds();
  loop_lcd();
}

void loop_read_sms() {
  // check for incoming SMS.
  // accepting_new_messages: if we are about to display a message, hold off until we've displayed the message
  // so that we don't overwrite our FRAM and RAM message.
  //
  // (! any_unread_messages()): since we don't have a queue, we can't read a message if there are any unread messages.
  // i tried adding a queue but ran out of time to make it work
  if (accepting_new_messages() && (! any_unread_messages())) {
    byte sim_position = read_sms();
    // If there is a new SMS, it is now in message_buffer.
    if (sim_position > 0) {
      // Find the mailbox for the message.  If there's an unread message in that box already,
      // leave the new message on the SIM.  Since read_sms will get all messages (whether read or not) and
      // we control transfer by deleting them off the SIM the only after after writing to FRAM, the
      // message will stay queued on the SIM.  Eventually the SIM will fill up if nobody reads the messages,
      // but at least we don't have to buffer multiple mailboxes in FRAM.
      byte mailbox_number = find_mailbox_number();
      if (! mailbox_message_is_new(mailbox_number)) {
        write_message_to_mailbox(mailbox_number);
        delete_sms(sim_position);
      }
    }
  }
}

void loop_read_switches() {
  // OK to read switches and change to a different mailbox unless we're in showing the header.
  if (mailbox_display_state != MAILBOX_SHOW_HEADER &&
      mailbox_display_state != MAILBOX_SHOWING_HEADER) {
    byte switch_number = read_switches();
    if (switch_number > 0) {
      byte mailbox_number = switch_number - 1;
      read_message_from_mailbox(mailbox_number);
      long now = secs();
      mailbox_display_state_time = now;
      lcd.clear();
      lcd_backlight_on();
      if (message_valid()) {
        setMailboxLEDBright(mailbox_number, true);
        displayed_mailbox = mailbox_number;
        mailbox_display_state = MAILBOX_SHOW_HEADER;
      } else {
        displayed_mailbox = NO_MAILBOX;
        mailbox_display_state = MAILBOX_NO_MESSAGES;
        lcd.print(F("No messages from "));
        lcd.println(mailbox_names[mailbox_number]);
        {
          // prevent corrupted messages that have the 'U' flag but no content.
          // not sure how it happens, but if we ever get one, that will make the LED flash.
          // so this will clear it and preserve the invariant that !message_valid implies zeroed out.
          clear_message();
          write_message_to_mailbox(mailbox_number);
        }
      }
    }
  }
}

void mark_message_read(byte mailbox_number) {
  if (message_buffer.read_flag == MESSAGE_FLAG_UNREAD) {
    sms_send_ack(); // acks to the phone_number in in message_buffer
    message_buffer.read_flag = MESSAGE_FLAG_READ;
    write_message_read_flag_to_fram(mailbox_number);
  }
}


void loop_leds() {
  long t = secs();
  bool flash_time = ((t % 2) == 0);

  for (byte mailbox_number = 0; mailbox_number < NUMBER_OF_MAILBOXES; mailbox_number++) {
    const bool flash = mailbox_message_is_new(mailbox_number);
    if (mailbox_number == displayed_mailbox) {
      setMailboxLEDDim(mailbox_number, flash_time && flash);
    } else {
      setMailboxLEDBright(mailbox_number, flash_time && flash);
    }
  }
}

/**
 * There is a window between MAILBOX_SHOW_HEADER and
 * MAILBOX_SHOWING_MESSAGE where a new message can come in
 * overwrite the before it is shown. Don't
 * accept new messages when in the mailbox_showing_header state.
 */
bool accepting_new_messages() {
  return (mailbox_display_state != MAILBOX_SHOW_HEADER &&
          mailbox_display_state != MAILBOX_SHOWING_HEADER &&
          mailbox_display_state != MAILBOX_SHOW_MESSAGE);
}


void loop_lcd() {
  long now = secs();

  {
    switch(mailbox_display_state) {
    case MAILBOX_NO_MESSAGES:
      if ((now - mailbox_display_state_time) >= 5) {
        mailbox_display_state_time = now;
        mailbox_display_state = MAILBOX_SHOW_HELP;
      }
      break;

    case MAILBOX_SHOW_HEADER:
      lcd_backlight_on();
      lcd_print_header();
      mailbox_display_state = MAILBOX_SHOWING_HEADER;
      mailbox_display_state_time = now;
      break;

    case MAILBOX_SHOWING_HEADER:
      if ((now - mailbox_display_state_time) >= 5) {
        lcd_backlight_on();
        mailbox_display_state = MAILBOX_SHOW_MESSAGE;
        mailbox_display_state_time = now;
      }
      break;

    case MAILBOX_SHOW_MESSAGE:
      lcd_print_message();
      mark_message_read(displayed_mailbox);
      mailbox_display_state = MAILBOX_SHOWING_MESSAGE;
      mailbox_display_state_time = now;
      lcd_backlight_on();
      break;

    case MAILBOX_SHOWING_MESSAGE:
      if ((now - mailbox_display_state_time) >= 60) {
        mailbox_display_state_time = now;
        mailbox_display_state = MAILBOX_SHOW_HELP;
      }
      break;

    case MAILBOX_SHOW_HELP:
      mailbox_display_state_time = now;
      mailbox_display_state = MAILBOX_SHOWING_HELP;
      displayed_mailbox = NO_MAILBOX;
      lcd_show_help();
      break;

    case MAILBOX_SHOWING_HELP:
      if ((now - mailbox_display_state_time) >= 10) {
        mailbox_display_state_time = now;
        mailbox_display_state = MAILBOX_LCD_OFF;
        lcd_backlight_off();
      }
      break;
    }
  }

  if (backlight_on && (mailbox_display_state == MAILBOX_SHOWING_HELP || mailbox_display_state == MAILBOX_LCD_OFF)) {
    if ((now - last_backlight_on_time) > 60) {
      lcd_backlight_off();
      displayed_mailbox = NO_MAILBOX;
      lcd_show_help();
    }
  }
}

long secs() {
  return millis() / 1000;
}

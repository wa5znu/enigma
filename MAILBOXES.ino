#include <mailboxes.h>

#include <mailboxes.conf>

// Compare message_buffer.phone to mailbox_phones and if there is number a match return corresponding mailbox_number
// Otherwise, return last mailbox which is the box for "Other".
byte find_mailbox_number() {
  byte i = 0;
  for ( ; i < NUMBER_OF_MAILBOXES; i++) {
    if (strcmp(mailbox_phones[i], message_buffer.phone) == 0) {
      return i;
    }
  }
  // if no match, return last mailbox, which is "Other"
  return NUMBER_OF_MAILBOXES-1;
}


char *get_mailbox_name(byte i) {
  return mailbox_names[i];
}

bool mailbox_message_is_new(byte i) {
  return (read_message_read_flag_from_fram(i) == MESSAGE_FLAG_UNREAD);
}

void write_message_to_mailbox(byte mailbox_number) {
  write_message_to_fram(mailbox_number);
}

void read_message_from_mailbox(byte mailbox_number) {
  read_message_from_fram(mailbox_number);
}

bool any_unread_messages() {
  for (byte mailbox_number = 0; mailbox_number < NUMBER_OF_MAILBOXES; mailbox_number++) {
    if (mailbox_message_is_new(mailbox_number)) {
        return true;
      }
  }
  return false;
}

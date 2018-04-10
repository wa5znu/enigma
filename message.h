/* FRAM Memory layout for message
 * : Phone numbers and one message each for three specified senders.
 * 1. 21 char null-terminated phone number (including null) 
 * 2. 25 char null-terminated date (including null)
 * 3. 161-char null-terminated message (including null)
 *
 */ 

#ifndef __local_message_h__
#define __local_message_h__

// MESSAGE_FLAG_NONE must be \0 to align with clear FRAM
#define MESSAGE_FLAG_NONE '\0'
#define MESSAGE_FLAG_UNREAD 'U'
#define MESSAGE_FLAG_READ 'R'

struct message {
  char phone[21];
  char date[25];
  char text[161];
  char read_flag;
};

#endif

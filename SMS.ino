#include "SIM900.h"
#include <avr/pgmspace.h>
#include <sms.h>

#include "switch.h"


SMSGSM sms;

void setup_gsmsms()
{
  lcd.clear();
  lcd_print_copyright();
  lcd.print(F("ENIGMA Connecting..."));
  while (! (gsm.begin(115200))) {
    //
  }
  gsm.forceON();	//To ensure that SIM908 is not only in charge mode

  {
    byte c;
    do {
      c = gsm.CheckRegistration();
      delay(1000);
    } while (c != REG_REGISTERED);
  }
  if (read_switch() == STARTUP_CLEAR_SIM_SWITCH) {
    lcd.print(F("\nDeleting SMS from SIM..."));
    delete_all_sms();
    lcd.println(F("done!"));
  }
  lcd.println(F("Online!\nPlease wait..."));
}

// If there are messages on SIM card, read the earliest one.
// Don't trust the SIM's "unread" flag since it gets set when we read the message, and we might not have written it to FRAM.
// So instead, once a message is in FRAM, we delete it from the SMS.  Our caller does that.
// Detect error conditions where it looks like there is an SMS but we failed to read it, and return 0 in those cases as well.
// Hopefully those conditions are transient, because if they are not and require deleting something from the SIM, this isn't going to do it
// and polling will get slow.
byte read_sms() {
  byte sim_position=sms.IsSMSPresent(SMS_ALL);
  if (sim_position == 0) {
    // no messages
  } else if (sim_position < 0) {
    lcd.print(F("SMS fail "));
    lcd.println(sim_position);
  } else {
    // read new SMS
    char ret = sms.GetSMS(sim_position, message_buffer.phone, message_buffer.date, message_buffer.text, sizeof(message_buffer.text)-1);
    if ((ret == GETSMS_UNREAD_SMS) || (ret == GETSMS_READ_SMS)) {
      message_buffer.read_flag = MESSAGE_FLAG_UNREAD;
    } else {
      // some error condition -1, -2, -3 or some odd condition GETSMS_NO_SMS | GETSMS_OTHER_SMS (queued for sending)
      // let's hope this is transient since we can't delete a "NO_SMS" and we shouldn't delete an "OTHER_SMS" -- hopefully it goes away.
      sim_position = 0;
    }
  }     
  return sim_position;
}

// When a message is read, send an ack consiting of the message's date and time.
// Sending an ack to bogus numbers may cause a loop, if the number responds.
// 1. Don't send acks to bogus numbers that begin "+11". These numbers come from Apple's iMessage, for example. 
// 2. "+0" are international so don't answer those either.  
// 3. Don't ack numbers that aren't of strlen("+1NNNNNNNNNN"), or 12.
void sms_send_ack() {
  if (strlen(message_buffer.phone) != 12) {
    return;
  }
  if ((message_buffer.phone[1] == '1' && message_buffer.phone[2] == '1') || (message_buffer.phone[1] == '0')) {
    return;
  }

  byte c = sms.SendSMS(message_buffer.phone, message_buffer.date);
  // nothing to do with returned value.  we could mark the message as un-acked in FRAM
  // and try again later, but the risk of runaway SMS sending is too high.
}

void delete_sms(byte i) {
  sms.DeleteSMS(i);
}

//if NO SPACE or SIM gets horked in some way, you need delete SMS from position 1 to position 20
void delete_all_sms() {
  for (byte i = 1; i < 21; i++) {
    delete_sms(i);
  }
}

void send_boot_sms() {
  sms.SendSMS(mailbox_phones[0], "Boot");
}

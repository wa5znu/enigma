#include "LCD.h"

#define LCD_ON_BRIGHTNESS (255)
#define LCD_OFF_BRIGHTNESS (0)
#define LCD_CONTRAST (16)

LiquidCrystal lcd(0x32);    // AKAFUGU: TWILiquidCrystal 

bool backlight_on = false;

void setup_lcd() {
  lcd.begin(40, 4);
  lcd_backlight_on();
  lcd.setContrast(LCD_CONTRAST);
  lcd.saveContrast(LCD_CONTRAST);
}

void lcd_backlight_off() {
  lcd.setBrightness(LCD_OFF_BRIGHTNESS);
  backlight_on = false;
}

void lcd_backlight_on() {
  lcd.setBrightness(LCD_ON_BRIGHTNESS);
  backlight_on = true;
  last_backlight_on_time = secs();
}

void lcd_print_date() {
  char *time = strchr(message_buffer.date, ',');
  *time++ = '\0';
  char *tzm = strchr(time, '-');                             // won't work in europe, asia, etc.
  *tzm++ = '\0';
  byte offset = atoi(tzm) / 4;
  lcd.print(F("on 20"));
  lcd.print(message_buffer.date);
  lcd.print(F(" at "));
  lcd.print(time);
  lcd.print(F(" "));
  char *tz;
  switch(offset) {
    // we don't know if it's DST or not so just try to get it right for central time zone, ignore mountain, and fudge on eastern.
  case 8: tz = "PST"; break;
  case 7: tz = "PDT"; break;
  case 6: tz = "CST"; break;
  case 5: tz = "CDT"; break;
  case 4: tz = "EDT"; break;
  default:
    tz = tzm;
  }
  lcd.println(tz);
  // restore what we trashed so message_buffer.date is pristine again for use with ack.
  time[-1] = ',';
  tzm[-1] = '-';
}

void lcd_show_help() {
  lcd.clear();
  lcd.println(F("LED flashes when there is a new message"));
  lcd.println(F("Push button above flashing LED to read."));
  lcd.println(F("Push button above dark LEDs to read"));
  lcd.println(F("old message again.  ENIGMA (LEIGH KLOTZ)"));
}

/*
 * Clear LCD and display message old/new flag, sender name, date, and phone number from buffer.
 * Don't display plus character in phone number; it's confusing.
 */
boolean lcd_print_header() {
  byte mailbox_number = find_mailbox_number();
  boolean is_new = mailbox_message_is_new(mailbox_number);
  lcd_print_old_new_message(is_new);                            // line 1
  lcd.print(F(" message from "));
  lcd.print(get_mailbox_name(mailbox_number));
  lcd.print(F(" ("));
  lcd_print_phone_number();
  lcd.println(F(")"));
  lcd_print_date();                                             // line 2
  lcd_print_old_new_message(is_new);
  lcd.print(F(" message follows..."));                          // line 3
}

void lcd_print_old_new_message(bool is_new) {
  if (is_new) {
    lcd.print(F("New"));
  } else {
    lcd.print(F("Old"));
  }
}

void lcd_print_phone_number() {
  for (uint16_t i = 0; i <= sizeof(message_buffer.phone); i++) {
    char c = message_buffer.phone[i];
    if (c == '\0') {
      break;
    }
    if (c != '+') {
      lcd.print(c);
    }
  }
}

/*
 * Clear LCD and display message from buffer.  Wrap lines since the Akafugu LCD and 40x4 display can't wrap from 1-2 to 3-4.
 */
void lcd_print_message() {
  lcd.clear();
  byte row = 0;
  byte len = strlen(message_buffer.text);

  // have to wrap the lines here because the 40x4 LCD wraps line 1-2 and lines 3-4, but not 1-2-3-4.
  byte col = 0;
  for (byte i = 0; i < len; i++) {
    char c = message_buffer.text[i];
    if (col == 40 || c == '\n') {
      lcd.setCursor(0, ++row);
      col = 0;
    }
    if (c != '\n') {
      lcd.print(c);
      col++;
    }
  }
}


void lcd_print_copyright() {
  lcd.setCursor(0, 3);
  lcd.print(F("ENIGMA Copyright 2014 Leigh L Klotz, Jr."));
  lcd.setCursor(0, 0);
}

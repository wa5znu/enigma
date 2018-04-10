#define SWITCH_PORT (A0)

#define SWITCH1_VALUE 50
#define SWITCH2_VALUE 100
#define SWITCH3_VALUE 200


void setup_switch() {
  pinMode(SWITCH_PORT, INPUT_PULLUP);
}

byte read_switch() {
  int val = analogRead(SWITCH_PORT);
  if (val <= SWITCH1_VALUE) return 1;
  if (val <= SWITCH2_VALUE) return 2;
  if (val <= SWITCH3_VALUE) return 3;
  return 0;
}

byte read_switches() {
  byte s = read_switch();
  if (s >= 1 && s <= 3) {
    byte mailbox_number = s - 1;
#if 0
    lcd.clear();
    lcd_backlight_on();
    lcd.print(F("Checking Messages from "));
    lcd.println(mailbox_names[mailbox_number]);
#endif
    setMailboxLEDBright(mailbox_number, true);
    while (read_switch() != 0) {
      // wait
    }
    setMailboxLEDBright(mailbox_number, false);
#if 0
    delay(250);
#endif
  }
  return s;
}

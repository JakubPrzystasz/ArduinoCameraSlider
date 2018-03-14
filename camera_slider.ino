#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <EEPROM.h>

uint8_t arrow_up[8] = {0b00100, 0b01110, 0b10101, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000};
uint8_t check[8] = {0x0, 0x1, 0x3, 0x16, 0x1c, 0x8, 0x0};
uint8_t cross[8] = {0x0, 0x1b, 0xe, 0x4, 0xe, 0x1b, 0x0};

struct servo
{
  unsigned long speed;
  unsigned long time;
  //true - right false - left
  unsigned long direction;
};

struct slot_
{
  //32 bytes
  //32 free slots
  unsigned long delay;
  unsigned long time;
  servo srv[3];
};

LiquidCrystal_I2C lcd(0x27, 20, 4);

Servo s[3];

const char left_arrow = 126;
const char right_arrow = 127;
const char ok = 5;
const char cancel = 6;
const char up_arrow = 125;
const byte s1_pin = 13, s2_pin = 12, s3_pin = 11, cam_pin = 10,
           plus_btn = 2, minus_btn = 3, clear_btn = 4, ok_btn = 5, a_btn = 6;

/*
  0 - main
  1 - in slot
  2 - settings
  3 - setting
  4 - work
*/
byte mode = 0;
byte slot = 1;
byte busy_slots = 0;
byte option = 0;
byte option_in = 0;
byte pointer = 0;
unsigned long val__;
unsigned long *wsk;
unsigned long camera_delay;

slot_ current;

byte val[10];

void setup()
{
  //DEBUG
  Serial.begin(9600);
  //DEBUG

  //setup pins
  pinMode(cam_pin, OUTPUT);
  pinMode(plus_btn, INPUT_PULLUP);
  pinMode(minus_btn, INPUT_PULLUP);
  pinMode(clear_btn, INPUT_PULLUP);
  pinMode(ok_btn, INPUT_PULLUP);
  //limit sensors
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  pinMode(A1, INPUT_PULLUP);
  //setup servos
  s[0].attach(s1_pin);
  s[1].attach(s2_pin);
  s[2].attach(s3_pin);

  //setup lcd
  lcd.begin();
  lcd.createChar(0, arrow_up);
  lcd.createChar(1, check);
  lcd.createChar(2, cross);
  lcd.noCursor();
  lcd.noBlink();
  lcd.setCursor(5, 1);
  lcd.print(F("Powered by"));
  lcd.setCursor(4, 2);
  lcd.print(F("GrzesiuTravel"));
  delay(1500);
  lcd.home();
  lcd.clear();
  read_slots();
  print_lcd(0);
}
void loop()
{
  char key;
  key = get_key();
  if (key != 0)
  {
    print_lcd(key);
    if(key == 'C' && mode == 4){
      mode = 1;
      in_slot(0);
      return;
    }
  }
  else if (mode == 4)
  {
    work_mode();
  }
}

void print_lcd(char key)
{
  if (mode == 0)
  {
    choose_slot(key);
    return;
  }
  if (mode == 1)
  {
    in_slot(key);
    return;
  }
  if (mode == 2)
  {
    change_option(key);
    return;
  }
  if (mode == 3)
  {
    set_val(key);
    return;
  }
}

void choose_slot(char key)
{
  read_slots();
  if (key == '+')
  {
    ++slot;
    if (slot == busy_slots + 2)
    {
      slot = 1;
    }
  }
  if (key == '-')
  {
    --slot;
    if (slot == 0)
    {
      slot = busy_slots;
      ++slot;
    }
  }
  if (key == '=')
  {
    mode = 1;
    in_slot(0);
    return;
  }
  if (slot == busy_slots + 1)
  {
    lcd.clear();
    lcd.setCursor(2, 1);
    lcd.print(F("Create new config"));
    lcd.setCursor(1, 2);
    lcd.print(F("Available slots: "));
    lcd.print((1024 / sizeof(slot_)) - busy_slots);
    return;
  }
  mode = 0;
  EEPROM.get(sizeof(slot_) * --slot, current);
  ++slot;
  lcd.clear();
  //1st row
  lcd.setCursor(7, 0);
  lcd.print(F("Slot #"));
  lcd.print(slot);
  //2nd row
  lcd.setCursor(0, 1);
  lcd.print(F("Interval: "));
  lcd.print(current.delay);
  lcd.print(F("ms"));
  //3nd row
  lcd.setCursor(0, 2);
  lcd.print(F("Shutter:  "));
  lcd.print(current.time);
  lcd.print(F("ms"));
  //4nd row
  lcd.setCursor(0, 3);
  lcd.print(F("Servo #1: "));
  lcd.print(current.srv[0].time);
  lcd.print(F("ms"));
}

void in_slot(char key)
{
  if (key == 'A')
  {
    EEPROM.put(sizeof(slot_) * --slot, current);
    ++slot;
    EEPROM.get(sizeof(slot_) * --slot, current);
    ++slot;
    lcd.clear();
    lcd.home();
    lcd.print(F("WORK MODE"));
    lcd.setCursor(0,1);
    lcd.print(F("Press C to exit"));
    lcd.setCursor(0,2);
    lcd.print(F("Next trun: "));
    mode = 4;
    work_mode();
    return;
  }
  if (key == 'C')
  {
    option = 0;
    mode = 0;
    choose_slot(0);
    return;
  }
  if (key == '=')
  {
    mode = 2;
    change_option(0);
    return;
  }
  if (key == '+')
  {
    ++option;
    if (option > 5)
    {
      option = 0;
    }
  }
  if (key == '-')
  {
    --option;
    if (option > 5)
    {
      option = 5;
    }
  }
  mode = 1;
  lcd.clear();
  lcd.setCursor(7, 0);
  lcd.print(F("Slot #"));
  lcd.print(slot);
  lcd.setCursor(1, 1);
  lcd.print(F("Save"));
  lcd.setCursor(11, 1);
  lcd.print(F("Servo #1"));
  lcd.setCursor(1, 2);
  lcd.print(F("Delete"));
  lcd.setCursor(11, 2);
  lcd.print(F("Servo #2"));
  lcd.setCursor(1, 3);
  lcd.print(F("Camera"));
  lcd.setCursor(11, 3);
  lcd.print(F("Servo #3"));
  if (option == 0)
  {
    //save
    lcd.setCursor(0, 1);
    lcd.print(left_arrow);
  }
  else if (option == 1)
  {
    //delete
    lcd.setCursor(0, 2);
    lcd.print(left_arrow);
  }
  else if (option == 2)
  {
    //camera
    lcd.setCursor(0, 3);
    lcd.print(left_arrow);
  }
  else if (option == 3)
  {
    //servo 1
    lcd.setCursor(10, 1);
    lcd.print(left_arrow);
  }
  else if (option == 4)
  {
    //servo 2
    lcd.setCursor(10, 2);
    lcd.print(left_arrow);
  }
  else if (option == 5)
  {
    //servo 3
    lcd.setCursor(10, 3);
    lcd.print(left_arrow);
  }
}

void change_option(char key)
{
  if (key == 'C')
  {
    mode = 1;
    in_slot(0);
    option_in = 0;
    return;
  }
  // 2 camera 3..5 servo
  if (key == '+')
  {
    ++option_in;
    if (option == 2)
    {
      if (option_in > 1)
      {
        option_in = 0;
      }
    }
    if (option > 2 && option < 6)
    {
      if (option_in > 2)
      {
        option_in = 0;
      }
    }
  }
  if (key == '-')
  {
    --option_in;
    if (option == 2)
    {
      if (option_in > 1)
      {
        option_in = 1;
      }
    }
    if (option > 2 && option < 6)
    {
      if (option_in > 2)
      {
        option_in = 2;
      }
    }
  }
  if (key == '=')
  {
    mode = 3;
    set_val(0);
    return;
  }
  mode = 2;
  lcd.clear();
  if (option == 0)
  {
    //save
    EEPROM.put(sizeof(slot_) * --slot, current);
    ++slot;
    option = 0;
    choose_slot(0);
    Serial.print("Camera ");Serial.print(current.delay);Serial.print(' ');Serial.print(current.time);Serial.print('\n');
    Serial.print("Servo#1 ");Serial.print(current.srv[0].direction);Serial.print(' ');Serial.print(current.srv[0].speed);Serial.print(' ');Serial.print(current.srv[0].time);Serial.print('\n');
    Serial.print("Servo#2 ");Serial.print(current.srv[1].direction);Serial.print(' ');Serial.print(current.srv[1].speed);Serial.print(' ');Serial.print(current.srv[1].time);Serial.print('\n');
    Serial.print("Servo#3 ");Serial.print(current.srv[2].direction);Serial.print(' ');Serial.print(current.srv[2].speed);Serial.print(' ');Serial.print(current.srv[2].time);Serial.print('\n');
    return;
  }
  else if (option == 1)
  {
    lcd.setCursor(4, 1);
    lcd.print(F("Deleting..."));
    //delete
    //slot is on top of stack
    read_slots();
    if (busy_slots == slot)
    {
      for (int i = sizeof(slot_) * --slot; i < sizeof(slot_); i++)
      {
        EEPROM.write(i, 0);
      }
    }
    else
    {
      slot_ var;
      for (byte i = slot; i <= busy_slots; ++i)
      {
        byte index = i;
        EEPROM.get(sizeof(slot_) * index, current);
        EEPROM.put(sizeof(slot_) * --index, current);
      }
    }
    option = 0;
    choose_slot(0);
    return;
  }
  else if (option == 2)
  {
    //camera
    lcd.setCursor(7, 0);
    lcd.print(F("Camera"));
    lcd.setCursor(1, 1);
    lcd.print(F("Delay"));
    lcd.setCursor(1, 2);
    lcd.print(F("Exposure time"));
    switch (option_in)
    {
    case 0:
      lcd.setCursor(0, 1);
      lcd.print(left_arrow);
      wsk = &current.delay;
      break;
    case 1:
      lcd.setCursor(0, 2);
      lcd.print(left_arrow);
      wsk = &current.time;
      break;
    }
  }
  else if (option > 2 && option < 6)
  {
    //servo
    lcd.setCursor(6, 0);
    lcd.print(F("Servo #"));
    lcd.print(option - 2);
    lcd.setCursor(1, 1);
    lcd.print(F("Time"));
    lcd.setCursor(1, 2);
    lcd.print(F("Speed"));
    lcd.setCursor(1, 3);
    lcd.print(F("Direction"));
    switch (option_in)
    {
    case 0:
      lcd.setCursor(0, 1);
      lcd.print(left_arrow);
      wsk = &current.srv[option - 3].time;
      break;
    case 1:
      lcd.setCursor(0, 2);
      lcd.print(left_arrow);
      wsk = &current.srv[option - 3].speed;
      break;
    case 2:
      lcd.setCursor(0, 3);
      lcd.print(left_arrow);
      wsk = &current.srv[option - 3].direction;
      break;
    }
  }
  val__ = *wsk;
}

void set_val(char key)
{
  if (key == '+')
  {
    if (pointer == 0)
    {
      //save
      *wsk = val__;
      //discard value
      //exit
      pointer = 0;
      mode = 2;
      change_option(0);
      return;
    }
    if (option == 2)
    {
      //camera
      if (pointer == 9)
      {
        pointer = 0;
        mode = 2;
        change_option(0);
        return;
      }
      else
      {
        change_val(1);
      }
    }
    if (option > 2 && option < 6)
    {
      //servos
      if (option_in == 2)
      {
        //direction
        if (pointer == 2)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_flag();
        }
      }
      if (option_in == 1)
      {
        //speed
        if (pointer == 3)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_val_spd(1);
        }
      }
      if (option_in == 0)
      {
        //time
        if (pointer == 9)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_val(1);
        }
      }
    }
  }
  if (key == '-')
  {
    if (pointer == 0)
    {
      //save
      *wsk = val__;
      //exit
      pointer = 0;
      mode = 2;
      change_option(0);
      return;
    }
    if (option == 2)
    {
      //camera
      if (pointer == 9)
      {
        pointer = 0;
        mode = 2;
        change_option(0);
        return;
      }
      else
      {
        change_val(0);
      }
    }
    if (option > 2 && option < 6)
    {
      //servos
      if (option_in == 2)
      {
        //direction
        if (pointer == 2)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_flag();
        }
      }
      if (option_in == 1)
      {
        //speed
        if (pointer == 3)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_val_spd(0);
        }
      }
      if (option_in == 0)
      {
        //time
        if (pointer == 9)
        {
          pointer = 0;
          mode = 2;
          change_option(0);
          return;
        }
        else
        {
          change_val(0);
        }
      }
    }
  }
  if (key == '=')
  {
    ++pointer;
    if (option == 2)
    {
      //camera
      if (pointer > 9)
      {
        pointer = 0;
      }
    }
    if (option > 2 && option < 6)
    {
      //servos
      if (option_in == 2)
      {
        //direction
        if (pointer > 2)
        {
          pointer = 0;
        }
      }
      if (option_in == 1)
      {
        //speed
        if (pointer > 3)
        {
          pointer = 0;
        }
      }
      if (option_in == 0)
      {
        //time
        if (pointer > 9)
        {
          pointer = 0;
        }
      }
    }
  }

  if (key == 'C')
  {
    --pointer;
    if (option == 2)
    {
      //camera
      if (pointer > 9)
      {
        pointer = 9;
      }
    }
    if (option > 2 && option < 6)
    {
      //servos
      if (option_in == 2)
      {
        //direction
        if (pointer > 2)
        {
          pointer = 2;
        }
      }
      if (option_in == 1)
      {
        //speed
        if (pointer > 3)
        {
          pointer = 3;
        }
      }
      if (option_in == 0)
      {
        //time
        if (pointer > 9)
        {
          pointer = 9;
        }
      }
    }
  }

  mode = 3;
  lcd.clear();
  //do case
  if (option == 2)
  {
    //for camera
    lcd.setCursor(2, 0);
    lcd.print(F("Camera - "));
    switch (option_in)
    {
    //delay
    case 0:
      lcd.print(F("delay"));
      break;
    //explosure time
    case 1:
      lcd.print(F("exp time"));
      break;
    }
    lcd.setCursor(6, 1);
    print_val(val__);
    lcd.setCursor(3, 3);
    lcd.print(convert_val(val__));
    //print pointer
    lcd.setCursor(2, 1);
    lcd.write(1);
    lcd.setCursor(15, 1);
    lcd.print(F("ms"));
    lcd.setCursor(18, 1);
    lcd.write(2);
    switch (pointer)
    {
    case 0:
      lcd.setCursor(2, 2);
      lcd.write(0);
      break;
    case 9:
      lcd.setCursor(18, 2);
      lcd.write(0);
      break;
    default:
      lcd.setCursor(5 + pointer, 2);
      lcd.write(0);
      break;
    }
  }
  if (option > 2 && option < 6)
  {
    //for servos
    lcd.setCursor(2, 0);
    lcd.print(F("Servo #"));
    lcd.print(option - 2);
    lcd.print(F(" - "));
    switch (option_in)
    {
    case 0:
      lcd.print(F("time"));
      lcd.setCursor(6, 1);
      print_val(val__);
      lcd.setCursor(3, 3);
      lcd.print(convert_val(val__));
      //print pointer
      lcd.setCursor(2, 1);
      lcd.write(1);
      lcd.setCursor(15, 1);
      lcd.print(F("ms"));
      lcd.setCursor(18, 1);
      lcd.write(2);
      switch (pointer)
      {
      case 0:
        lcd.setCursor(2, 2);
        lcd.write(0);
        break;
      case 9:
        lcd.setCursor(18, 2);
        lcd.write(0);
        break;
      default:
        lcd.setCursor(5 + pointer, 2);
        lcd.write(0);
        break;
      }
      break;
    case 1:
      lcd.print(F("speed"));
      lcd.setCursor(9, 1);
      print_val_speed(val__);
      lcd.setCursor(7, 1);
      lcd.write(1);
      lcd.setCursor(12, 1);
      lcd.write(2);
      switch (pointer)
      {
      case 0:
        lcd.setCursor(7, 2);
        lcd.write(0);
        break;
      case 3:
        lcd.setCursor(12, 2);
        lcd.write(0);
        break;
      default:
        lcd.setCursor(8 + pointer, 2);
        lcd.write(0);
        break;
      }
      break;
    case 2:
      lcd.print(F("direc"));
      lcd.setCursor(7, 1);
      lcd.write(1);
      lcd.setCursor(9, 1);
      if (val__ == 0)
      {
        lcd.print(left_arrow);
      }
      else
      {
        lcd.print(right_arrow);
      }
      lcd.setCursor(11, 1);
      lcd.write(2);
      switch (pointer)
      {
      case 0:
        lcd.setCursor(7, 2);
        lcd.write(0);
        break;
      case 1:
        lcd.setCursor(9, 2);
        lcd.write(0);
        break;
      case 2:
        lcd.setCursor(11, 2);
        lcd.write(0);
        break;
      }
      break;
    }
  }
}

void print_val(unsigned long val)
{
  String str = String(val);
  String tmp;
  for (int i = str.length() - 1; i >= 0; i--)
  {
    tmp += str[i];
  }
  str = tmp;
  char c;
  int c_ascii;
  for (int i = 7; i >= 0; i--)
  {
    c = str[i];
    c_ascii = c;
    if (c_ascii > 47 && c_ascii < 58)
    {
      lcd.print(str[i]);
    }
    else
    {
      lcd.print(F("0"));
    }
  }
}

void print_val_speed(unsigned long val)
{
  String str = String(val);
  String tmp;
  for (int i = str.length() - 1; i >= 0; i--)
  {
    tmp += str[i];
  }
  str = tmp;
  char c;
  int c_ascii;
  for (int i = 1; i >= 0; i--)
  {
    c = str[i];
    c_ascii = c;
    if (c_ascii > 47 && c_ascii < 58)
    {
      lcd.print(str[i]);
    }
    else
    {
      lcd.print(F("0"));
    }
  }
}

String convert_val(unsigned long val)
{
  String message = "";
  String hrs, mins, sec;
  unsigned int hours = val / 3600000;
  unsigned int minutes = (val % 3600000) / 60000;
  unsigned int seconds = ((val % 3600000) % 60000) / 1000;
  if (hours > 0)
  {
    hrs = String(hours);
  }
  else
  {
    hrs = "0" + String(hours);
  }
  if (minutes > 9)
  {
    mins = String(minutes);
  }
  else
  {
    mins = "0" + String(minutes);
  }
  if (seconds > 9)
  {
    sec = String(seconds);
  }
  else
  {
    sec = "0" + String(seconds);
  }

  message = hrs + " h " + mins + " min " + sec + " s";
  return message;
}

void change_val(bool f)
{
  String str = String(val__);
  String tmp;
  for (int i = str.length() - 1; i >= 0; i--)
  {
    tmp += str[i];
  }
  str = tmp;
  tmp = "";
  char c;
  unsigned int c_ascii;
  for (int i = 7; i >= 0; i--)
  {
    c = str[i];
    c_ascii = c;
    if (c_ascii > 47 && c_ascii < 58)
    {
      tmp += str[i];
    }
    else
    {
      tmp += "0";
    }
  }
  str = tmp;
  unsigned int x = str[pointer - 1];
  x -= 48;
  if (f == true)
  {
    x++;
    if (x > 9)
    {
      x = 0;
    }
  }
  else
  {
    x--;
    if (x > 9)
    {
      x = 9;
    }
  }
  x += 48;
  str[pointer - 1] = (char)x;
  val__ = str.toInt();
}

void change_val_spd(bool f)
{
  String str = String(val__);
  String tmp;
  for (int i = str.length() - 1; i >= 0; i--)
  {
    tmp += str[i];
  }
  str = tmp;
  tmp = "";
  char c;
  unsigned int c_ascii;
  for (int i = 1; i >= 0; i--)
  {
    c = str[i];
    c_ascii = c;
    if (c_ascii > 47 && c_ascii < 58)
    {
      tmp += str[i];
    }
    else
    {
      tmp += "0";
    }
  }
  str = tmp;
  unsigned int x = str[pointer - 1];
  x -= 48;
  if (f == true)
  {
    x++;
    if (x > 9)
    {
      x = 0;
    }
  }
  else
  {
    x--;
    if (x > 9)
    {
      x = 9;
    }
  }
  x += 48;
  str[pointer - 1] = (char)x;
  val__ = str.toInt();
}

void work_mode()
{
  lcd.setCursor(0,3);
  lcd.print(current.delay-(millis() - camera_delay)); 
  if ((millis() - camera_delay) > current.delay)
  {
    lcd.clear();
    lcd.home();
    lcd.print(F("WORK MODE"));
    lcd.setCursor(0,1);
    lcd.print(F("Press C to exit"));
    lcd.setCursor(0,2);
    lcd.print(F("Next trun: "));
    int ms;
    camera_delay = millis();
    for (byte i = 0; i < 3; i++)
    {
      int speed_value = current.srv[i].speed;
      //reverse value is rotating left and round values
      if (current.srv[i].direction == 0)
      {
        //rotate left
        speed_value = 100 - speed_value;
        speed_value = RoundDown(speed_value);
      }
      else
      {
        speed_value += 90;
        speed_value = RoundUp(speed_value);
      }
      //servo 1; speed 0-89 left; 91 - 179 right
      //0 -left 1- right
      //rotate right
      ms = current.srv[i].time;
      s[i].write(speed_value);
      if (digitalRead(7) == LOW){
          mode == 0;
          choose_slot(0);
          return;
        }
        if (digitalRead(8) == LOW){
          mode == 0;
          choose_slot(0);
          return;
        }
        if (digitalRead(9) == LOW){
          mode == 0;
          choose_slot(0);
          return;
        }
        if (analogRead(A0) < 50){
          mode == 0;
          choose_slot(0);
          return;
        }
        if (digitalRead(A1) < 50){
          mode == 0;
          choose_slot(0);
          return;
        }
      delay(ms);
      s[i].write(90);
    }
    //make photo
    digitalWrite(cam_pin, HIGH);
    ms = (int)current.time;
    delay(ms);
    digitalWrite(cam_pin, LOW);
  }
}

void change_flag()
{
  bool x;
  x = (bool)val__;
  val__ = !val__;
}

char get_key()
{
  char key;
    if (digitalRead(plus_btn) == LOW)
    {
      delay(20);
      key = '+';
      while (digitalRead(plus_btn) == LOW);
      delay(20);
    }
    else if (digitalRead(minus_btn) == LOW)
    {
      delay(20);
      key = '-';
      while (digitalRead(minus_btn) == LOW);
      delay(20);
    }
    else if (digitalRead(clear_btn) == LOW)
    {
      delay(20);
      key = 'C';
      while (digitalRead(clear_btn) == LOW);
      delay(20);
    }
      else if (digitalRead(ok_btn) == LOW)
      {
        delay(20);
        key = '=';
        while (digitalRead(ok_btn) == LOW);
        delay(20);
      }
      else if (digitalRead(a_btn) == LOW)
      {
        delay(20);
        key = 'a';
        while (digitalRead(a_btn) == LOW);
        delay(20);
    } else {
      key = 0;
    }
  return key;
}

void read_slots(void)
{
  slot_ actual;
  byte bs_slots = 0;
  byte size = sizeof(slot_);
  while (true)
  {
    if (bs_slots == 0)
    {
      EEPROM.get(0, actual);
    }
    else
    {
      EEPROM.get(bs_slots * size, actual);
    }
    if (actual.delay == 0)
    {
      break;
    }
    ++bs_slots;
  }
  busy_slots = bs_slots;
}

int RoundUp(int toRound)
{
  if (toRound % 10 == 0)
    return toRound;
  return (10 - toRound % 10) + toRound;
}

int RoundDown(int toRound)
{
  return toRound - toRound % 10;
}

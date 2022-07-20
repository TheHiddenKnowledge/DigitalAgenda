#include <DS3231.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <GUI.h>
// Motor Pins
#define MIN1 10
#define MIN2 11
// EEPROM I2C Pins
#define WC 12
#define SDA 2
#define SCK 3
// Buzzer Pin
#define bzr 27
// OLED SPI Pins
#define cs 8
#define rst 8
#define dc 9
#define mosi 4
#define miso 5
#define sclk 6
// MOSFET pin
#define poweroff 23
// Battery read pin
#define battery 22
// Interrupt pins
#define powerinter 28
#define alarminter 29
// Address for the EEPROM chip
#define memadd 0xA2 >> 1
// Size of the OLED screen
#define width 320
#define height 240
// Maximum size for a text entry
#define MAXTEXT 30
// Maximum amount of task entries
#define MAXTASKS 400
// Maximum storage capacity for EEPROM
#define MAXMEM 32000
#define TIMELEN 6
// Colors used in the GUI
#define navcolor 0x1C37
#define deftextcolor 0xFFFF
#define highlight 0x075D
#define popupcolor 0xFF8C
// Text sizes for GUI elements
#define normaltext 2
#define headertext 3
// OLED display driver
Adafruit_ILI9341 tft = Adafruit_ILI9341(cs, dc, mosi, sclk, rst, miso);
// Prototype needed beforehand
void drawBG();
// GUI Controller
GUI myGUI(width, height, 6, 3, 0, &drawBG, 0, &tft);
// RTC module
DS3231 myRTC;
// Data structure to hold the date and time
DateTime datetime;
// Flag for popups
bool popUp;
// User name storage
char *name;
// Booleans for RTC
bool century = false;
bool h12Flag = true;
bool pmFlag;
// Time tracker for alarm functionality
long alarmtime;
// Index of the task that is set to the alarm
int alarmidx;
// Upcoming task information
byte upcomingdate[3][TIMELEN];
char *upcomingtask[3];
int upcomingidx[3];
// Keeps track of the current task count for a day
byte taskcount;
// Determines the sleep states of the device
bool sleep = false;
// Page trackers
byte prevpage = 0;
byte page = 0;
// Booleans for power button
bool prevbutton = LOW;
bool button = LOW;
// Booleans for alarm interrupt
bool prevalarmint;
bool alarmint;
// User settings container
byte usersettings[3];
// Accent colors list
short accents[3] = {0x84BF, 0xC965, 0xAB58};
// Current accent color
short defbackcolor;
// Global array for getting entry values
ENTRY *responses[5];
// Global array for getting carousel values
CAROUSEL *choices[5];
// Calendar lookup date
byte lookupdate[3];
// Keyboard keycode
byte prevkey;
byte key;
// Initializes the objects required during runtime
void intializeDevice()
{
  // Turning on all peripherals
  digitalWrite(poweroff, HIGH);
  // Initializing motor pins
  pinMode(MIN1, OUTPUT);
  pinMode(MIN2, OUTPUT);
  digitalWrite(MIN1, LOW);
  digitalWrite(MIN2, LOW);
  // Setting the write control for the EEPROM
  pinMode(WC, OUTPUT);
  digitalWrite(WC, HIGH);
  // Interrupt pins must be pulled high
  pinMode(powerinter, INPUT_PULLUP);
  pinMode(alarminter, INPUT_PULLUP);
  // Beginning necessary libraries
  Wire.begin();
  tft.begin();
  tft.setRotation(1);
  // Reading the users name from EEPROM
  name = readText(0);
  // Reading user settings from EEPROM
  for (int i = 0; i < 3; i++)
  {
    usersettings[i] = readEEPROM(MAXTEXT + i);
  }
  // Setting the accent color
  defbackcolor = accents[usersettings[2]];
  // Enabling the alarm
  myRTC.turnOnAlarm(1);
  // Datetime object initialization
  datetime = RTClib::now();
  // Default lookup date for calendar page is the current date
  lookupdate[0] = datetime.year();
  lookupdate[1] = datetime.month();
  lookupdate[2] = datetime.day();
  // Begin on the welcome page
  changePage(0, 0);
}
// Reads a character from the keyboard
byte readKey()
{
  // Keyboard I2C address defined as 0x09
  Wire.requestFrom(9, 1);
  if (Wire.available())
  {
    byte c = Wire.read();
    return c;
  }
}
// Reads a byte from the EEPROM storage
byte readEEPROM(short addr)
{
  Wire.beginTransmission(memadd);
  Wire.write(addr >> 8);
  Wire.write(addr && 0x00FF);
  Wire.requestFrom(memadd, 1);
  if (Wire.available())
  {
    byte c = Wire.read();
    return c;
  }
}
// Writes a byte to the EEPROM storage
void writeEEPROM(short addr, byte data)
{
  digitalWrite(WC, LOW);
  Wire.beginTransmission(memadd);
  Wire.write(addr >> 8);
  Wire.write(addr && 0x00FF);
  Wire.write(data);
  digitalWrite(WC, HIGH);
}
// Reads a string from the EEPROM storage
char *readText(short addr)
{
  // Note that memory is allocated and must be properly managed
  char *text = new char[MAXTEXT];
  for (int i = 0; i < MAXTEXT; i++)
  {
    text[i] = readEEPROM(addr + i);
  }
  return text;
}
// Writes a string to the EEPROM storage
void writeText(short addr, char *text)
{
  for (int i = 0; i < MAXTEXT; i++)
  {
    // For uninitialized chars, the EEPROM default should be 0xFF
    if (isAscii(text[i]))
    {
      writeEEPROM(addr + i, 0xFF);
    }
    else
    {
      writeEEPROM(addr + i, text[i]);
    }
  }
}
// Drives the vibration motor
void runMotor(long t)
{
  // On for 1 second then off for 1 second
  if (t % 1000 == 0)
  {
    if ((t / 1000) % 2 != 0)
    {
      digitalWrite(MIN1, HIGH);
      digitalWrite(MIN2, LOW);
    }
    else
    {
      digitalWrite(MIN1, LOW);
      digitalWrite(MIN2, LOW);
    }
  }
}
// Drives the piezo disk buzzer
void runBuzzer(long t)
{
  // Pulses every other quarter second
  if (usersettings[0] == 0)
  {
    if (t % 250 == 0)
    {
      if ((t / 250) % 2 != 0)
      {
        analogWrite(bzr, 123);
      }
      else
      {
        analogWrite(bzr, 0);
      }
    }
  }
  // Pulses every other half second
  else if (usersettings[1] == 0)
  {
    if (t % 500 == 0)
    {
      if ((t / 500) % 2 != 0)
      {
        analogWrite(bzr, 123);
      }
      else
      {
        analogWrite(bzr, 0);
      }
    }
  }
  // Pulses in triplets
  else if (usersettings[2] == 0)
  {
    if (t % 2000 == (0 || 500 || 1000))
    {
      analogWrite(bzr, 123);
    }
    else if (t % 2000 == (250 || 750 || 1250))
    {
      analogWrite(bzr, 0);
    }
  }
}
// Interrupt function for the RTC alarm
void triggerAlarm()
{
  // Getting the date
  byte month = readEEPROM(MAXTEXT + 3 + alarmidx * 6 + 1);
  byte day = readEEPROM(MAXTEXT + 3 + alarmidx * 6 + 2);
  byte hour = readEEPROM(MAXTEXT + 3 + alarmidx * 6 + 3);
  byte minute = readEEPROM(MAXTEXT + 3 + alarmidx * 6 + 4);
  byte ampm = readEEPROM(MAXTEXT + 3 + alarmidx * 6 + 5);
  char *task = readText(MAXTEXT + 3 + MAXTASKS * 6 + MAXTEXT * alarmidx);
  if (sleep)
  {
    wakeUp();
  }
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 2 * height / 6);
  myGUI.grid(back, 3, 1, 2, 1);
  String Time = String(hour) + ":" + String(minute) + (ampm) ? "PM" : "AM";
  String Date = String(month) + "/" + String(day);
  LABEL *datelbl = myGUI.Label(Date, deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(datelbl, 3, 0, 1, 2);
  LABEL *timelbl = myGUI.Label(Time, deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(timelbl, 3, 1, 1, 2);
  LABEL *tasklbl = myGUI.Label(String(task), deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(tasklbl, 3, 1);
  myGUI.updateElements();
  while (alarmtime < 10000)
  {
    delay(1);
    runMotor(alarmtime);
    runBuzzer(alarmtime);
    alarmtime += 1;
  }
  analogWrite(bzr, 0);
  digitalWrite(MIN1, LOW);
  digitalWrite(MIN2, LOW);
  delete datelbl;
  delete timelbl;
  delete task;
  delete tasklbl;
  myGUI.updateElements();
}
// Gets the current battery percentage
float getBattery()
{
  short raw = analogRead(battery);
  short final = (raw - 6 * 1023 / 9) / (1023 - 6 * 1023 / 9);
  return final;
}
// Popup to warn user of low battery percentage
void batteryPopup()
{
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 6);
  myGUI.grid(back, 3, 1);
  LABEL *bat = myGUI.Label("Battery percentage below 15%!", deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(bat, 3, 1);
  myGUI.updateElements();
  delay(5000);
  delete back;
  delete bat;
  myGUI.updateElements();
}
// Draws the battery icon based on the battery percentage
void drawBattery()
{
  float percent = getBattery();
  int width_ = 10;
  int height_ = 16;
  tft.fillRect(20 - width_ / 2, 20 + height_, width_, -height_ * percent, 0x1662);
  tft.drawRect(20 - width_ / 2, 20 + height_, width_, height_, 0x0000);
  tft.drawLine(15, 14, 25, 20, 0x0000);
  tft.drawLine(15, 22, 25, 28, 0x0000);
  tft.drawRect(20 - height_ / 2, 20 - 3, 6, -4, 0x0000);
}
// Functions to change pages
void changePage(int *p, uint8_t pg)
{
  myGUI.deleteElements();
  clearGUIHolders();
  if (page == 0)
  {
    prevpage = pg;
  }
  else
  {
    if (page != pg)
    {
      prevpage = page;
    }
  }
  if (pg == 0)
  {
    welcomePage();
  }
  else if (pg == 1)
  {
    homePage();
  }
  else if (pg == 2)
  {
    calendarPage();
  }
  else if (pg == 3)
  {
    settingsPage();
  }
  else if (pg == 4)
  {
    profilePage();
  }
  page = pg;
}
// Clears the pointer lists for responses and choices;
void clearGUIHolders()
{
  for (int i = 0; i < 5; i++)
  {
    responses[i] = 0;
    choices[i] = 0;
  }
}
// Searches through the EEPROM and sets the alarm
void setAlarm(bool Day)
{
  int taskidx;
  byte t[TIMELEN];
  byte minimum[TIMELEN] = {100, 100, 100, 100, 100, 100};
  for (int i = 0; i < MAXTASKS; i++)
  {
    bool check = true;
    long t_tot;
    long min_tot;
    for (int j = 0; j < TIMELEN; j++)
    {
      t[j] = readEEPROM(MAXTEXT + 3 + i * 6 + j);
      t_tot += t[j] << 8 * (TIMELEN - j - 1);
      min_tot += minimum[j] << 8 * (TIMELEN - j - 1);
    }
    if (min_tot > t_tot)
    {
      for (int k = 0; k < TIMELEN; k++)
      {
        minimum[k] = t[k];
      }
      taskidx = i;
    }
  }
  if (minimum[0] == datetime.year() && minimum[1] == datetime.month())
  {
    alarmidx = taskidx;
    myRTC.setA1Time(minimum[2], minimum[3], minimum[4], 0, 0, Day, h12Flag, minimum[5]);
  }
}
// Puts the device into sleep mode
void goSleep()
{
  digitalWrite(poweroff, LOW);
  setAlarm(0);
  attachInterrupt(digitalPinToInterrupt(powerinter), wakeUp, FALLING);
  attachInterrupt(digitalPinToInterrupt(alarminter), triggerAlarm, FALLING);
  SLPCTRL_CTRLA |= 0x1 << 1;
  SLPCTRL_CTRLA |= 0x1;
  sleep = true;
}
// Wakeup routine
void wakeUp()
{
  digitalWrite(poweroff, HIGH);
  detachInterrupt(digitalPinToInterrupt(powerinter));
  detachInterrupt(digitalPinToInterrupt(alarminter));
  SLPCTRL_CTRLA |= 0x1 << 0;
  SLPCTRL_CTRLA |= 0x0;
  changePage(0, 1);
  sleep = false;
}
// Creates the GUI elements at the top of the screen
void drawBG()
{
  // Stars
  if (usersettings[1] = 0)
  {
    tft.fillScreen(0x18E8);
    short colors[3] = {0x79DA, 0x4E3F, 0x5C3F};
    for (int i = 0; i < 30; i++)
    {
      uint16_t x = random(0, 320);
      uint16_t y = random(0, 240);
      uint16_t s = random(5, 10);
      uint16_t idx = random(0, 2);
      tft.fillRect(x, y, s, s, colors[idx]);
    }
  }
  // Lines
  else if (usersettings[1] = 1)
  {
    tft.fillScreen(0xE757);
    short colors[3] = {0x1F55, 0x04FF, 0xFDCC};
    for (int i = 0; i < 30; i++)
    {
      uint16_t type = random(0, 1);
      uint16_t idx = random(0, 2);
      if (type)
      {
        tft.drawLine(0, random(0, height), 0, random(0, height), colors[idx]);
      }
      else
      {
        tft.drawLine(random(0, height), 0, random(0, height), 0, colors[idx]);
      }
    }
  }
  // Circles
  else if (usersettings[1] = 2)
  {
    tft.fillScreen(0xFE79);
    short colors[3] = {0xA55F, 0xA7F6, 0xFD14};
    for (int i = 0; i < 30; i++)
    {
      uint16_t x = random(0, 320);
      uint16_t y = random(0, 240);
      uint16_t r = random(20, 40);
      uint16_t idx = random(0, 2);
      tft.fillCircle(x, y, r, colors[idx]);
    }
  }
}
// Draws the info bar at the top of the screen
void displayNav()
{
  BACKDROP *back = myGUI.Backdrop(navcolor, width, height / 5);
  myGUI.grid(back, 0, 1);
  drawBattery();
  LABEL *namelbl = myGUI.Label(name, deftextcolor, normaltext);
  myGUI.grid(namelbl, 0, 2);
  String Time = String(datetime.hour()) + ":" + String(datetime.minute()) + (pmFlag) ? "PM" : "AM";
  String Date = String(datetime.month()) + "/" + String(datetime.day()) + "/" + String(datetime.year());
  LABEL *timelbl = myGUI.Label(Time, deftextcolor, normaltext);
  myGUI.grid(timelbl, 0, 0);
  LABEL *datelbl = myGUI.Label(Date, deftextcolor, normaltext);
  myGUI.grid(datelbl, 0, 1);
}
// Creates a new user profile
void createNewUser(int *p, uint8_t s)
{
  name = new char[MAXTEXT];
  strcpy(responses[0]->value, name);
  for (int i = 0; i < MAXTEXT; i++)
  {
    writeEEPROM(i, name[i]);
  }
  clearGUIHolders();
  myGUI.deleteElements();
  LABEL *lbl1 = myGUI.Label("Welcome " + String(name), deftextcolor, normaltext);
  myGUI.grid(lbl1, 3, 1);
  delay(3000);
  changePage(NULL, 1);
}
// The first page that creates the user when the device is turned on
void welcomePage()
{
  char tempname[MAXTEXT];
  // Getting the name from EEPROM
  for (short i = 0; i < MAXTEXT; i++)
  {
    tempname[i] = readEEPROM(i);
  }
  if (strcmp(tempname, "") == 0)
  {
    LABEL *lbl1 = myGUI.Label("Hello, who are you?", deftextcolor, headertext);
    myGUI.grid(lbl1, 2, 1);
    responses[0] = myGUI.Entry("Name: ", deftextcolor, normaltext, MAXTEXT, navcolor, highlight, 3);
    myGUI.grid(responses[0], 3, 1);
    BUTTON *btn = myGUI.Button("Save Name", deftextcolor, normaltext, &createNewUser, 0, 0, navcolor, highlight, 3);
    myGUI.grid(btn, 4, 1);
  }
  else
  {
    LABEL *lbl1 = myGUI.Label("Welcome back " + String(name), deftextcolor, normaltext);
    myGUI.grid(lbl1, 3, 1);
    delay(3000);
    page = 1;
  }
}
// Gets the top most immediate tasks
void getUpcoming()
{
  short idx[3] = {500, 500, 500};
  byte t[TIMELEN];
  byte minimum[TIMELEN] = {100, 100, 100, 100, 100};
  for (int i = 0; i < 3; i++)
  {
    memset(upcomingdate[i], 0, 6);
    delete upcomingtask[i];
    upcomingidx[i] = 0;
  }
  for (int i = 0; i < 3; i++)
  {
    for (int j = 0; j < MAXTASKS; j++)
    {
      bool check = true;
      int t_tot;
      int min_tot;
      for (int k = 0; k < TIMELEN; k++)
      {
        t[k] = readEEPROM(MAXTEXT + 3 + j * 6 + k);
        t_tot += t[k] << 8 * (TIMELEN - k - 1);
        min_tot += minimum[k] << 8 * (TIMELEN - k - 1);
      }
      check &= min_tot > t_tot;
      for (int k = 0; k < 3; k++)
      {
        check &= (idx[k] != j);
      }
      if (check)
      {
        for (int k = 0; k < TIMELEN; k++)
        {
          minimum[k] = t[k];
        }
        idx[i] = j;
      }
    }
    upcomingtask[i] = readText(MAXTEXT + 3 + MAXTASKS * TIMELEN + MAXTASKS * idx[i]);
  }
}
// The homescreen that has buttons to navigate the device
void homePage()
{
  getUpcoming();
  displayNav();
  LABEL *head = myGUI.Label("Upcoming tasks", deftextcolor, headertext, defbackcolor, 3);
  myGUI.grid(head, 1, 1);
  for (int i = 0; i < 3; i++)
  {
    String text = (upcomingdate[i][0] != 0) ? (String(upcomingdate[i][1]) + "/" + String(upcomingdate[i][2])) : "";
    LABEL *upd = myGUI.Label(text, deftextcolor, normaltext, defbackcolor, 3);
    myGUI.grid(upd, 2 + i, 0);
    LABEL *upt = myGUI.Label(upcomingtask[0], deftextcolor, normaltext, defbackcolor, 3);
    myGUI.grid(upt, 2 + i, 1, 1, 2);
  }
  BUTTON *btn1 = myGUI.Button("Calendar", deftextcolor, normaltext, &changePage, 0, 2, navcolor, highlight, 3);
  myGUI.grid(btn1, 5, 0);
  BUTTON *btn2 = myGUI.Button("Settings", deftextcolor, normaltext, &changePage, 0, 3, navcolor, highlight, 3);
  myGUI.grid(btn2, 5, 1);
  BUTTON *btn3 = myGUI.Button("Profile", deftextcolor, normaltext, &changePage, 0, 4, navcolor, highlight, 3);
  myGUI.grid(btn3, 5, 2);
}
// Gets all tasks from chosen date
void getFromDate()
{
  taskcount = 0;
  for (int i = 0; i < 3; i++)
  {
    memset(upcomingdate[i], 0, 6);
    delete upcomingtask[i];
    upcomingidx[i] = 0;
  }
  for (int i = 0; i < MAXTASKS; i++)
  {
    byte year = readEEPROM(MAXTEXT + 3 + i * 6);
    byte month = readEEPROM(MAXTEXT + 3 + i * 6 + 1);
    byte date = readEEPROM(MAXTEXT + 3 + i * 6 + 2);
    byte hour = readEEPROM(MAXTEXT + 3 + i * 6 + 3);
    byte minute = readEEPROM(MAXTEXT + 3 + i * 6 + 4);
    byte ampm = readEEPROM(MAXTEXT + 3 + i * 6 + 5);
    if (year == lookupdate[0] && month == lookupdate[1] && date == lookupdate[2])
    {
      upcomingdate[taskcount][0] = year;
      upcomingdate[taskcount][1] = month;
      upcomingdate[taskcount][2] = date;
      upcomingdate[taskcount][3] = hour;
      upcomingdate[taskcount][4] = minute;
      upcomingdate[taskcount][5] = ampm;
      upcomingtask[taskcount] = readText(MAXTEXT + 3 + 6 * MAXTASKS + i * MAXTEXT);
      upcomingidx[taskcount] = i;
      taskcount += 1;
    }
  }
}
// Adds a task
void addTask(int *p, uint8_t s)
{
  if (atoi(responses[0]->value) <= 12 && atoi(responses[1]->value) <= 59)
  {
    if (taskcount < 3)
    {
      for (int i = 0; i < MAXTASKS; i++)
      {
        if (readEEPROM(MAXTEXT + 3 + i * 6) == 0xFF)
        {
          writeEEPROM(MAXTEXT + 3 + i * 6, lookupdate[0]);
          writeEEPROM(MAXTEXT + 3 + i * 6 + 1, lookupdate[1]);
          writeEEPROM(MAXTEXT + 3 + i * 6 + 2, lookupdate[2]);
          writeEEPROM(MAXTEXT + 3 + i * 6 + 3, atoi(responses[0]->value));
          writeEEPROM(MAXTEXT + 3 + i * 6 + 4, atoi(responses[1]->value));
          writeEEPROM(MAXTEXT + 3 + i * 6 + 5, choices[0]->idx);
          writeText(MAXTEXT + 3 + MAXTASKS * 6 + i * MAXTEXT, responses[2]->value);
          break;
        }
      }
      setAlarm(0);
    }
    else
    {
      BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 5);
      myGUI.grid(back, 3, 1);
      LABEL *warning = myGUI.Label("Too many task entries for this day!", deftextcolor, normaltext);
      myGUI.grid(warning, 3, 1);
      myGUI.updateElements();
      delay(3000);
      myGUI.deleteElements();
      changePage(0, 3);
      addTaskPopup(0, 0);
      myGUI.updateElements();
      return;
    }
    popUp = false;
  }
  else
  {
    BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 5);
    myGUI.grid(back, 3, 1);
    LABEL *warning = myGUI.Label("Please use proper date/time values!", deftextcolor, normaltext);
    myGUI.grid(warning, 3, 1);
    myGUI.updateElements();
    delay(3000);
    myGUI.deleteElements();
    changePage(0, 3);
    setCurrentDTPopup(0, 0);
    myGUI.updateElements();
    return;
  }
  changePage(0, 2);
}
// Sets the date to be viewed
void viewDate(int *p, uint8_t s)
{
  if (atoi(responses[0]->value) <= 12 && atoi(responses[1]->value) <= 31)
  {
    lookupdate[0] = atoi(responses[2]->value);
    lookupdate[1] = atoi(responses[0]->value);
    lookupdate[2] = atoi(responses[1]->value);
    popUp = false;
  }
  else
  {
    BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 2 * height / 5);
    myGUI.grid(back, 3, 1, 2, 1);
    LABEL *warning = myGUI.Label("Please use proper date/time values!", deftextcolor, normaltext);
    myGUI.grid(warning, 3, 1);
    myGUI.updateElements();
    delay(3000);
    myGUI.deleteElements();
    changePage(0, 3);
    viewDatePopup(0, 0);
    myGUI.updateElements();
    return;
  }
  changePage(0, 2);
}
// Popup for adding a task
void addTaskPopup(int *p, uint8_t s)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 5);
  myGUI.grid(back, 3, 1);
  responses[0] = myGUI.Entry("Hour: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[0], 2, 0);
  responses[1] = myGUI.Entry("Minute: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[1], 2, 1);
  char ampm[2][MAXTEXT] = {"AM", "PM"};
  choices[0] = myGUI.Carousel(deftextcolor, normaltext, ampm, 2, navcolor, highlight, 3);
  myGUI.grid(choices[0], 2, 2);
  responses[2] = myGUI.Entry("Task: ", deftextcolor, normaltext, MAXTEXT, navcolor, highlight, 3);
  myGUI.grid(responses[2], 3, 1);
  BUTTON *add = myGUI.Button("Add Task", deftextcolor, normaltext, &addTask, 0, 0, navcolor, highlight, 3);
  myGUI.grid(add, 4, 1);
  myGUI.updateElements();
}
// Popup for viewing a date
void viewDatePopup(int *p, uint8_t s)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 2 * height / 5);
  myGUI.grid(back, 2, 1, 2, 1);
  responses[0] = myGUI.Entry("Month: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[0], 2, 0);
  responses[1] = myGUI.Entry("Date: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[1], 2, 1);
  responses[2] = myGUI.Entry("Year: ", deftextcolor, normaltext, 5, navcolor, highlight, 3);
  myGUI.grid(responses[2], 2, 2);
  BUTTON *view = myGUI.Button("Select Date", deftextcolor, normaltext, &viewDate, 0, 0, navcolor, highlight, 3);
  myGUI.grid(view, 3, 1);
  myGUI.updateElements();
}
// Saves changes made to a task
void saveTask(int *p, uint8_t i)
{
  if (atoi(responses[0]->value) <= 12 && atoi(responses[1]->value) <= 59)
  {
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6, lookupdate[0]);
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + 1, lookupdate[1]);
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + 2, lookupdate[2]);
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + 3, atoi(responses[0]->value));
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + 4, atoi(responses[1]->value));
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + 5, choices[0]->idx);
    writeText(MAXTEXT + 3 + MAXTASKS * 6 + upcomingidx[i] * MAXTEXT, responses[2]->value);
    setAlarm(0);
    popUp = false;
  }
  else
  {
    BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 5);
    myGUI.grid(back, 3, 1);
    LABEL *warning = myGUI.Label("Please use proper date/time values!", deftextcolor, normaltext);
    myGUI.grid(warning, 3, 1);
    myGUI.updateElements();
    delay(3000);
    myGUI.deleteElements();
    changePage(0, 3);
    alterTaskPopup(0, 0);
    editTaskPopup(0, 0);
    myGUI.updateElements();
    return;
  }
  changePage(0, 2);
}
// Popup for editing a task
void editTaskPopup(int *p, uint8_t i)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 3 * height / 5);
  myGUI.grid(back, 3, 1);
  responses[0] = myGUI.Entry("Hour: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[0], 2, 0);
  responses[1] = myGUI.Entry("Minute: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[1], 2, 1);
  char ampm[2][MAXTEXT] = {"AM", "PM"};
  choices[0] = myGUI.Carousel(deftextcolor, normaltext, ampm, 2, navcolor, highlight, 3);
  myGUI.grid(choices[0], 2, 2);
  responses[2] = myGUI.Entry("Task: ", deftextcolor, normaltext, MAXTEXT, navcolor, highlight, 3);
  myGUI.grid(responses[2], 3, 1);
  BUTTON *save = myGUI.Button("Save Task", deftextcolor, normaltext, &saveTask, 0, i, navcolor, highlight, 3);
  myGUI.grid(save, 4, 1);
  char buffer[MAXTEXT];
  itoa(upcomingdate[i][3], buffer, 10);
  strcpy(responses[0]->value, buffer);
  responses[0]->idx = strlen(buffer);
  itoa(upcomingdate[i][4], buffer, 10);
  strcpy(responses[1]->value, buffer);
  responses[1]->idx = strlen(buffer);
  choices[0]->idx = upcomingdate[i][5];
  strcpy(responses[2]->value, upcomingtask[i]);
  responses[2]->idx = strlen(upcomingtask[i]);
  myGUI.updateElements();
}
// Deletes the current task
void deleteTask(int *p, uint8_t i)
{
  for (int j = 0; j < 6; j++)
  {
    writeEEPROM(MAXTEXT + 3 + upcomingidx[i] * 6 + j, 0xFF);
  }
  for (int j = 0; j < MAXTEXT; j++)
  {
    writeEEPROM(MAXTEXT + 3 + MAXTASKS * 6 + upcomingidx[i] * MAXTEXT + j, 0xFF);
  }
  popUp = false;
  changePage(0, 2);
}
// Popup for editing/deleting selected task
void alterTaskPopup(int *p, uint8_t i)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BUTTON *edit = myGUI.Button("Edit", deftextcolor, normaltext, &editTaskPopup, 0, i, defbackcolor, highlight, 3);
  myGUI.grid(edit, 2 + i, 1);
  BUTTON *del = myGUI.Button("Delete", deftextcolor, normaltext, &deleteTask, 0, i, defbackcolor, highlight, 3);
  myGUI.grid(del, 2 + i, 2);
  myGUI.updateElements();
}
// Displays and allows for manipulation of tasks
void calendarPage()
{
  getFromDate();
  displayNav();
  LABEL *head = myGUI.Label("Tasks for " + String(lookupdate[1]) + "/" + String(lookupdate[2]), deftextcolor, headertext, defbackcolor, 3);
  myGUI.grid(head, 1, 1);
  for (int i = 0; i < 3; i++)
  {
    if (upcomingdate[i][0] != 0)
    {
      String text = String(upcomingdate[0][3]) + ":" + String(upcomingdate[0][4]) + (upcomingdate[0][5]) ? "PM" : "AM";
      BUTTON *upd = myGUI.Button(text, deftextcolor, normaltext, &alterTaskPopup, 0, i, defbackcolor, highlight, 3);
      myGUI.grid(upd, 2 + i, 0);
      LABEL *upt = myGUI.Label(upcomingtask[0], deftextcolor, normaltext, defbackcolor, 3);
      myGUI.grid(upt, 2 + i, 1, 1, 2);
    }
  }
  BUTTON *btn1 = myGUI.Button("Add Task", deftextcolor, normaltext, &addTaskPopup, 0, 2, navcolor, highlight, 3);
  myGUI.grid(btn1, 5, 0, 1, 2);
  BUTTON *btn2 = myGUI.Button("View Date", deftextcolor, normaltext, &viewDatePopup, 0, 3, navcolor, highlight, 3);
  myGUI.grid(btn2, 5, 1, 1, 2);
}
// Sets the date of the device
void setCurrentDT()
{
  if (atoi(responses[0]->value) <= 12 && atoi(responses[1]->value) <= 31 && atoi(responses[3]->value) <= 12 && atoi(responses[4]->value) <= 59)
  {
    byte hour = 0;
    if (!choices[1]->idx && atoi(responses[3]->value) != 12)
    {
      hour = (choices[1]->idx) ? (atoi(responses[3]->value)) : (atoi(responses[3]->value) + 12);
    }
    myRTC.setDoW(choices[0]->idx + 1);
    myRTC.setSecond(0);
    myRTC.setMinute(atoi(responses[4]->value));
    myRTC.setHour(hour);
    myRTC.setDate(atoi(responses[1]->value));
    myRTC.setMonth(atoi(responses[0]->value));
    myRTC.setYear(atoi(responses[2]->value) - 2000);
    popUp = false;
  }
  else
  {
    BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 4 * height / 5);
    myGUI.grid(back, 3, 1, 2, 1);
    LABEL *warning = myGUI.Label("Please use proper date/time values!", deftextcolor, normaltext);
    myGUI.grid(warning, 3, 1);
    myGUI.updateElements();
    delay(3000);
    myGUI.deleteElements();
    changePage(0, 3);
    setCurrentDTPopup(0, 0);
    myGUI.updateElements();
    return;
  }
  changePage(0, 3);
}
// Changes the users name
void changeName()
{
  popUp = false;
  strcpy(name, responses[0]->value);
  writeText(0, name);
  changePage(0, 3);
}
// Resets the device
void resetDevice()
{
  popUp = false;
  for (short i = 0; i < MAXMEM; i++)
  {
    writeEEPROM(i, 0xFF);
  }
  myRTC.setDoW(1);
  myRTC.setSecond(0);
  myRTC.setMinute(0);
  myRTC.setHour(12);
  myRTC.setDate(25);
  myRTC.setMonth(12);
  myRTC.setYear(0);
  delete name;
  name = readText(0);
  memset(usersettings, 0, 3);
  changePage(0, 0);
}
// Pop up for setting the date
void setCurrentDTPopup(int *p, uint8_t s)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, 3 * width / 4, 4 * height / 5);
  myGUI.grid(back, 3, 1, 2, 1);
  responses[0] = myGUI.Entry("Month: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[0], 2, 0);
  responses[1] = myGUI.Entry("Date: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[1], 2, 1);
  responses[2] = myGUI.Entry("Year: ", deftextcolor, normaltext, 5, navcolor, highlight, 3);
  myGUI.grid(responses[2], 2, 2);
  LABEL *daylbl = myGUI.Label("Day: ", deftextcolor, normaltext);
  myGUI.grid(daylbl, 3, 0);
  char days[7][MAXTEXT] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
  choices[0] = myGUI.Carousel(deftextcolor, normaltext, days, 7, navcolor, highlight, 3);
  myGUI.grid(choices[0], 3, 1, 1, 2);
  responses[3] = myGUI.Entry("Hour: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[3], 4, 0);
  responses[4] = myGUI.Entry("Minute: ", deftextcolor, normaltext, 3, navcolor, highlight, 3);
  myGUI.grid(responses[4], 4, 1);
  char ampm[2][MAXTEXT] = {"AM", "PM"};
  choices[1] = myGUI.Carousel(deftextcolor, normaltext, ampm, 2, navcolor, highlight, 3);
  myGUI.grid(choices[1], 4, 2);
  BUTTON *set = myGUI.Button("Set Date", deftextcolor, normaltext, &setCurrentDT, 0, 0, navcolor, highlight, 3);
  myGUI.grid(set, 5, 1);
  myGUI.updateElements();
}
// Pop up for changing the users name
void changeNamePopup(int *p, uint8_t s)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, width / 2, 2 * height / 5);
  myGUI.grid(back, 2, 1, 2, 1);
  responses[0] = myGUI.Entry("Name: ", deftextcolor, normaltext, MAXTEXT, navcolor, highlight, 3);
  myGUI.grid(responses[0], 2, 1);
  BUTTON *save = myGUI.Button("Save Name", deftextcolor, normaltext, &changeName, 0, 0, navcolor, highlight, 3);
  myGUI.grid(save, 3, 1);
  myGUI.updateElements();
}
// Pop up for resetting the device
void resetDevicePopup(int *p, uint8_t s)
{
  popUp = true;
  myGUI.disableElements();
  myGUI.selection = 0;
  BACKDROP *back = myGUI.Backdrop(popupcolor, width / 2, 3 * height / 5);
  myGUI.grid(back, 4, 1);
  LABEL *lbl1 = myGUI.Label("This will clear all user settings and data.", deftextcolor, normaltext);
  myGUI.grid(lbl1, 3, 1);
  LABEL *lbl2 = myGUI.Label("Are you sure?", deftextcolor, normaltext);
  myGUI.grid(lbl2, 4, 1);
  BUTTON *y = myGUI.Button("Yes", deftextcolor, normaltext, &resetDevice, 0, 0, navcolor, highlight, 3);
  myGUI.grid(y, 5, 0, 1, 2);
  BUTTON *n = myGUI.Button("No", deftextcolor, normaltext, &changePage, 0, 3, navcolor, highlight, 3);
  myGUI.grid(n, 5, 1, 1, 2);
  myGUI.updateElements();
}
// Allows for manipulation of device settings
void settingsPage()
{
  displayNav();
  BUTTON *btn1 = myGUI.Button("Set Date", deftextcolor, normaltext, &setCurrentDTPopup, 0, 0, navcolor, highlight, 3);
  myGUI.grid(btn1, 2, 1);
  BUTTON *btn2 = myGUI.Button("Change Name", deftextcolor, normaltext, &changeNamePopup, 0, 0, navcolor, highlight, 3);
  myGUI.grid(btn2, 3, 1);
  BUTTON *btn3 = myGUI.Button("Reset Device", deftextcolor, normaltext, &resetDevicePopup, 0, 0, navcolor, highlight, 3);
  myGUI.grid(btn3, 4, 1);
}
// Saves the user settings
void saveSettings(int *p, uint8_t s)
{
  for (int i = 0; i < 3; i++)
  {
    usersettings[i] = choices[i]->idx;
    writeEEPROM(MAXTEXT + i, choices[i]->idx);
  }
  defbackcolor = accents[usersettings[2]];
  changePage(0, 4);
}
// Resets the user settings selectors
void resetSettings(int *p, uint8_t s)
{
  for (int i = 0; i < 3; i++)
  {
    choices[i]->idx = 0;
    usersettings[i] = 0;
  }
  defbackcolor = accents[usersettings[2]];
  changePage(0, 4);
}
// Allows for manipulation of profile settings
void profilePage()
{
  displayNav();
  LABEL *tonelbl = myGUI.Label("Alarm Tone", deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(tonelbl, 1, 0);
  char tones[3][MAXTEXT] = {"Tone 1", "Tone 2", "Tone 3"};
  choices[0] = myGUI.Carousel(deftextcolor, normaltext, tones, 3, navcolor, highlight, 3);
  myGUI.grid(choices[0], 1, 1, 1, 2);
  LABEL *bglbl = myGUI.Label("Background", deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(bglbl, 2, 0);
  char bgs[3][MAXTEXT] = {"Stars", "Lines", "Circles"};
  choices[1] = myGUI.Carousel(deftextcolor, normaltext, bgs, 3, navcolor, highlight, 3);
  myGUI.grid(choices[1], 2, 1, 1, 2);
  LABEL *accentlbl = myGUI.Label("Accent Color", deftextcolor, normaltext, defbackcolor, 3);
  myGUI.grid(accentlbl, 3, 0);
  char clrs[3][MAXTEXT] = {"Blue", "Red", "Purple"};
  choices[2] = myGUI.Carousel(deftextcolor, normaltext, clrs, 3, navcolor, highlight, 3);
  myGUI.grid(choices[2], 3, 1, 1, 2);
  BUTTON *btn1 = myGUI.Button("Save", deftextcolor, normaltext, &saveSettings, 0, 0, navcolor, highlight, 3);
  myGUI.grid(btn1, 5, 0);
  BUTTON *btn2 = myGUI.Button("Reset", deftextcolor, normaltext, &resetSettings, 0, 0, navcolor, highlight, 3);
  myGUI.grid(btn2, 5, 2);
}
void setup()
{
  intializeDevice();
}
void loop()
{
  datetime = RTClib::now();
  myRTC.getHour(h12Flag, pmFlag);
  key = readKey();
  if (key == 0xF3) // Home
  {
    changePage(0, 1);
  }
  else if (key == 0xF5) // Back
  {
    if (popUp)
    {
      changePage(0, page);
    }
    else
    {
      changePage(0, prevpage);
    }
  }
  if (prevkey != key)
  {
    myGUI.eventManager(key);
    myGUI.updateElements();
  }
  prevkey = key;
  button = digitalRead(powerinter);
  if (prevbutton != button && prevbutton == HIGH)
  {
    goSleep();
  }
  prevbutton = button;
  alarmint = digitalRead(alarminter);
  if (prevalarmint != alarmint && prevalarmint == HIGH)
  {
    triggerAlarm();
  }
  prevalarmint = alarmint;
  if (getBattery() < .15)
  {
    batteryPopup();
  }
  else if (getBattery() < .01)
  {
    goSleep();
  }
}
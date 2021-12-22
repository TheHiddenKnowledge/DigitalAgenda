//Initializing modules
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
int index = 0; //Index for character combination list
int nameindex = 0; //Index for the users name
int taskindex = 0; //Index for the task being edited
const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char charlist[5]; //List for combination of characters
char Name[12]; //List for characters of users name
String chosenDay; //The chosen day for the set of tasks
char category; //The chosen task for a given day
int dayIndex; //Index for the day of a task
int catIndex; //Index for the category of a task
int NoName = 0; //Boolean for whether a name has been entered
int chooseDay = 0; //Boolean for whether a day has been chosen
int t = 0; //Time constant for read/write character combination
byte save[4][8] = { //Special characters for save animation
  {0b00100, 0b00101, 0b00110, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000},
  {0b00100, 0b00101, 0b00110, 0b00111, 0b00110, 0b00001, 0b00000, 0b00000},
  {0b00100, 0b00101, 0b00110, 0b00111, 0b01110, 0b10101, 0b00100, 0b00100},
  {0b00100, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00100, 0b00100}
};
char keys[ROWS][COLS] = { //Array for the characters of the matrix
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
char dayKeys[7] = {'1','2','3','4','5','6','7'};
char tasks[7][4][17] = { //Array for the tasks
  {"", "", "", ""},
  {"", "", "", ""},
  {"", "", "", ""},
  {"", "", "", ""},
  {"", "", "", ""},
  {"", "", "", ""},
  {"", "", "", ""}
};
int addresses[7][4] = { //Array for the addresses assigned to each task
  {12, 29, 46, 63},
  {80, 97, 114, 131},
  {148, 165, 182, 199},
  {216, 233, 250, 267},
  {284, 301, 318, 335},
  {352, 369, 386, 403},
  {420, 437, 454, 471}
};
String combs[36] = {"1", "11", "111", "2", "22", "222", "3", "33", "333", "4", "44", "444", "5", "55", "555",
                    "6", "66", "666", "7", "77", "777", "8", "88", "888", "9", "99", "999", "1111", "2222", "3333", "4444", "5555",
                    "6666", "7777", "8888", "9999"
                   }; //Array for all the possible combinations
char letters[36] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9'
                   }; //Array for the characters corresponding to the keypad combinations
String Days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"}; //Array for days of the week
char categories[4] = {'A', 'B', 'C', 'D'}; //Array for the possible task categories
byte rowPins[ROWS] = {A3, A2, A1, A0}; //Pins for the row pins
byte colPins[COLS] = {2, 3, 4, 6}; //Pins for the column pins
Keypad customKeypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS); //Define the keypad
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12; //Pins for the lcd screen
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //Define the lcd

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2); //Define lcd size
  for (int i = 0; i < 4; i++) { //Creates the characters for save animation
    lcd.createChar(i + 2, save[i]);
  }
  lcd.setCursor(0, 0);
  StartScript(); //After the previous setup, it calls the starting script
}
void loop() {
  lcd.cursor();
  keyPadProcessing();
}
void StartScript() {
  callMemory(0, 0, 0);
  for (int d = 0; d < 7; d++) {
    for (int t = 0; t < 4; t++) {
      callMemory(addresses[d][t], d, t);
    }
  }
  if (strcmp(Name, "") == 0) { //If there is no name saved, then noname will be set to true and the device will ask for an input
    NoName = 1;
    lcd.print("Hello! You are?");
    lcd.setCursor(0, 1);
  }
  else { //If their is a name saved, then the device will welcome you using the saved name and shift to show the whole message
    String WelcomeText = "Welcome back " + String(Name);
    lcd.print(WelcomeText);
    delay(500);

    for (int i = 0; i < WelcomeText.length() - 6; i++) {
      lcd.scrollDisplayLeft();
      delay(250);
    }
    for (int i = 0; i < WelcomeText.length() - 6; i++) {
      lcd.scrollDisplayRight();
      delay(250);
    }

    delay(2000);
    startMenu(); //After either case, the main menu will be called
  }
}
void saveAnimation(int len) {
  for (int i = 2; i < 6; i++) { //Whenever this function is called, the list of characters is played through for the save animation
    lcd.setCursor(len, 0);
    lcd.write(i);
    delay(400);
  }
  lcd.setCursor(len, 0);
  lcd.print(" ");
  lcd.setCursor(strlen(tasks[dayIndex][catIndex]), 1);
}
void(* resetFunc) (void) = 0;
void keyPadProcessing() {
  //---------------------------------------------------
  char customKey = customKeypad.getKey(); //Character for the keypad press
  if (customKey) { //Whenever their is a keypress detected, it will add it to the combos list, increase the index by 1 and set the timer to 0
    charlist[index] = customKey;
    index += 1;
    t = 0;
  }
  //---------------------------------------------------
  if (NoName == 1 && strcmp(charlist, "*") == 0) { //When * is pressed and there is no name, the device displays the message and saves the name
    lcd.clear();
    lcd.print("Name saved!");
    delay(3000);
    writeMemory(0, Name);
    resetFunc();
  }
  if (strcmp(charlist, "##") == 0 && t > 100) { //When ## is the combo, the device backspaces and either takes one off of the name or the task at hand
    if (chosenDay != "") {
      tasks[dayIndex][catIndex][strlen(tasks[dayIndex][catIndex]) - 1] = '\0';
      taskindex -= 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(Days[dayIndex]);
      lcd.setCursor(0, 1);
      lcd.print(tasks[dayIndex][catIndex]);
      memset(charlist, 0, sizeof charlist);
    }
    if (NoName == 1) {
      Name[strlen(Name) - 1] = '\0';
      nameindex -= 1;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hello! You are?");
      lcd.setCursor(0, 1);
      lcd.print(Name);
      memset(charlist, 0, sizeof charlist);
    }
  }
  if (strcmp(charlist, "###") == 0 && t > 100) { //When ### is the combo, it erases the entire variable at hand
    if (chosenDay != "") {
      memset(tasks[dayIndex][catIndex], 0, sizeof tasks[dayIndex][catIndex]);
      taskindex = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(Days[dayIndex]);
      lcd.setCursor(0, 1);
      memset(charlist, 0, sizeof charlist);
    }
    if (NoName == 1) {
      memset(Name, 0, sizeof Name);
      nameindex = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hello! You are?");
      lcd.setCursor(0, 1);
      memset(charlist, 0, sizeof charlist);
    }
  }
  if (strcmp(charlist, "####") == 0 && t > 100) {
    resetMemory();
    resetFunc();
  }
  //---------------------------------------------------
  if (chooseDay == 1 && customKey && chosenDay == "") { //When the main menu is displayed, the user can use numbers 1-7 to choose what day to edit
    for (int r = 0; r < 7; r++) {
        if (customKey == dayKeys[r]) {
          lcd.clear();
          chosenDay = Days[customKey - '0' - 1];
          category = 'A';
          displayTask();
          memset(charlist, 0, sizeof charlist);
          chooseDay = 0;
        }
    }
  }
  if (chosenDay != "") { //When a day is chosen, the user can use the keypad characters
    if (customKey) {
      for (int t = 0; t < 4; t++) {
        if (customKey == categories[t]) {
          category = categories[t];
          displayTask();
          taskindex = strlen(tasks[dayIndex][catIndex]);
          memset(charlist, 0, sizeof charlist);
        }
      }
    }
    if (strcmp(charlist, "*") == 0 && t > 100) { //When * is pressed, it plays the save animation and saves the current task
      saveAnimation(chosenDay.length());
      writeMemory(addresses[dayIndex][catIndex], tasks[dayIndex][catIndex]);
      memset(charlist, 0, sizeof charlist);
    }
    if (strcmp(charlist, "#") == 0  && t > 100) { //When # is pressed, it returns to the main menu
      startMenu();
      chosenDay = "";
    }
  }
  //---------------------------------------------------
  if (t > 100) { //If the combos input is not read, it is cleared after 100 milliseconds
    memset(charlist, 0, sizeof charlist);
    index = 0;
  }
  t += 1; //Adds one to the timer every 10 milliseconds
  delay(10);
  //---------------------------------------------------
  if (!customKey && t > 100 && !chooseDay) { //When the timer has reached 100 millisecond and their is no keypressed, the device reads the combo, prints the corresponding letter, and adds it to the variable at hand
    for (int i = 0; i < 27; i++) {
      if (strcmp(charlist, combs[i].c_str()) == 0 && strlen(tasks[dayIndex][catIndex]) < 16) {
        lcd.print(letters[i]);
        memset(charlist, 0, sizeof charlist);
        if (NoName == 1) {
          Name[nameindex] = letters[i];
          nameindex += 1;
        }
        if (chosenDay != "") {
          tasks[dayIndex][catIndex][taskindex] = letters[i];
          taskindex += 1;
        }
      }
    }
  }
  //---------------------------------------------------
}
void startMenu() {
  lcd.setCursor(0, 0); //Asks user to choose a day and shifts message left to right
  lcd.clear();
  String menuText = "Choose a day to Read or Write";
  lcd.print(menuText);
  delay(500);


  for (int i = 0; i < menuText.length() - 11; i++) {
    lcd.scrollDisplayLeft();
    delay(250);
  }
  for (int i = 0; i < menuText.length() - 11; i++) {
    lcd.scrollDisplayRight();
    delay(250);
  }

  lcd.setCursor(0, 1);
  chooseDay = 1;
}
void callMemory(int startaddr, int d, int t) { //d and t are only for the array of tasks, this script reads the EEPROM from the start address until it reaches 128, assigning the letters the variable
  for (int i = startaddr;; i++) {
    int value = EEPROM.read(i);
    if (value != 128) {
      if (value != 0) {
        if (startaddr == 0) {
          Name[i - startaddr] = letters[value - 1];
        }
        if (startaddr != 0) {
          tasks[d][t][i - startaddr] = letters[value - 1];
        }
      }
    }
    else {
      break;
    }
  }
}
void writeMemory(int startaddr, char saveobj[]) { //Takes the saveobj parameter and starts at the EEPROM start address, interating a writing process that saves the index of each letter in the object
  int len = strlen(saveobj);
  for (int i = startaddr; i < len + startaddr; i++) {

    for (int l = 0; l < 27; l++) {
      if (saveobj[i - startaddr] == letters[l]) {
        EEPROM.write(i, l + 1);
      }
    }
  }
  EEPROM.write(startaddr + len, 128);
}
void resetMemory() {
  EEPROM.write(0, 128);
  for (int d = 0; d < 7; d++) {
    for (int t = 0; t < 4; t++) {
      EEPROM.write(addresses[d][t], 128);
    }
  }
}
void displayTask() {
  if (chosenDay != "") {
    for (int t = 0; t < 4; t++) {
      for (int d = 0; d < 7; d++) {
        if (category == categories[t] && chosenDay == Days[d]) {
          dayIndex = d;
          catIndex = t;
          lcd.clear();
          lcd.print(chosenDay);
          lcd.setCursor(0, 1);
          lcd.print(tasks[d][t]);
        }
      }
    }
  }
}

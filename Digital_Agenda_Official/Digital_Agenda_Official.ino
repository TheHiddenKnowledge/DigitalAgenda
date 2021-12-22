//Initializing modules
#include <Keypad.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>
// Index for character combination list
int index = 0;
// Index for the users name
int nameindex = 0;
// Index for the task being edited
int taskindex = 0;
// Rows and columns
const byte ROWS = 4;
const byte COLS = 4;
// List for combination of characters
char charlist[5];
// List for characters of users name
char Name[12];
// The chosen day for the set of tasks
String chosenDay;
// The chosen task for a given day
char category;
// Index for the day of a task
int dayIndex;
// Index for the category of a task
int catIndex;
// Boolean for whether a name has been entered
int NoName = 0;
// Boolean for whether a day needs to be chosen
int chooseDay = 0;
// Time constant for read/write character combination
int t = 0;
// Special characters for save animation
byte save[4][8] = {
    {0b00100, 0b00101, 0b00110, 0b00100, 0b00000, 0b00000, 0b00000, 0b00000},
    {0b00100, 0b00101, 0b00110, 0b00111, 0b00110, 0b00001, 0b00000, 0b00000},
    {0b00100, 0b00101, 0b00110, 0b00111, 0b01110, 0b10101, 0b00100, 0b00100},
    {0b00100, 0b10101, 0b01110, 0b11111, 0b01110, 0b10101, 0b00100, 0b00100}};
// Array for the characters of the matrix
char keys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
// Keys for accessing each day
char dayKeys[7] = {'1', '2', '3', '4', '5', '6', '7'};
// Array for the tasks
char tasks[7][4][17] = {
    {"", "", "", ""},
    {"", "", "", ""},
    {"", "", "", ""},
    {"", "", "", ""},
    {"", "", "", ""},
    {"", "", "", ""},
    {"", "", "", ""}};
// Array for the addresses assigned to each task
int addresses[7][4] = {
    {12, 29, 46, 63},
    {80, 97, 114, 131},
    {148, 165, 182, 199},
    {216, 233, 250, 267},
    {284, 301, 318, 335},
    {352, 369, 386, 403},
    {420, 437, 454, 471}};
// Array for all the possible combinations
String combs[36] = {"1", "11", "111", "2", "22", "222", "3", "33", "333", "4", "44", "444", "5", "55", "555",
                    "6", "66", "666", "7", "77", "777", "8", "88", "888", "9", "99", "999", "1111", "2222", "3333", "4444", "5555",
                    "6666", "7777", "8888", "9999"};
// Array for the characters corresponding to the keypad combinations
char letters[36] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', ' ', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
// Array for days of the week
String Days[7] = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
// Array for the possible task categories
char categories[4] = {'A', 'B', 'C', 'D'};
// Pins for the rows and columns
byte rowPins[ROWS] = {A3, A2, A1, A0};
byte colPins[COLS] = {2, 3, 4, 6};
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
// Pins for the LCD screen
const int rs = 7, en = 8, d4 = 9, d5 = 10, d6 = 11, d7 = 12;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); //Define the lcd

void setup()
{
  Serial.begin(9600);
  lcd.begin(16, 2);
  // Creates the characters for save animation
  for (int i = 0; i < 4; i++)
  {
    lcd.createChar(i + 2, save[i]);
  }
  lcd.setCursor(0, 0);
  // After the previous setup, it calls the starting script
  StartScript();
}
void loop()
{
  lcd.cursor();
  keyPadProcessing();
}
// Function that initializes the digital agenda's data saved on the EEPROM
void StartScript()
{
  // Gets the user's name from the EEPROM
  callMemory(0, 0, 0);
  // Gets the tasks data from the EEPROM
  for (int d = 0; d < 7; d++)
  {
    for (int t = 0; t < 4; t++)
    {
      callMemory(addresses[d][t], d, t);
    }
  }
  // If there is no name saved, then noname will be set to true and the device will ask for an input
  if (strcmp(Name, "") == 0)
  {
    NoName = 1;
    lcd.print("Hello! You are?");
    lcd.setCursor(0, 1);
  }
  // If their is a name saved, then the device will welcome you using the saved name and shift to show the whole message
  else
  {
    String WelcomeText = "Welcome back " + String(Name);
    lcd.print(WelcomeText);
    delay(500);

    for (int i = 0; i < WelcomeText.length() - 6; i++)
    {
      lcd.scrollDisplayLeft();
      delay(250);
    }
    for (int i = 0; i < WelcomeText.length() - 6; i++)
    {
      lcd.scrollDisplayRight();
      delay(250);
    }

    delay(2000);
    // After either case, the main menu will be called
    startMenu();
  }
}
// Function that plays the save animation
void saveAnimation(int len)
{
  for (int i = 2; i < 6; i++)
  {
    lcd.setCursor(len, 0);
    lcd.write(i);
    delay(400);
  }
  lcd.setCursor(len, 0);
  lcd.print(" ");
  lcd.setCursor(strlen(tasks[dayIndex][catIndex]), 1);
}
// Keyword function for resetting the hardware
void (*resetFunc)(void) = 0;
// Function for processing keypad entries
void keyPadProcessing()
{
  // Character passed from the keypad press
  char customKey = customKeypad.getKey();
  // Whenever their is a keypress detected, it will add it to the combos list, increase the index by 1 and set the timer to 0
  if (customKey)
  {
    charlist[index] = customKey;
    index += 1;
    t = 0;
  }
  // When * is pressed and there is no name, the device displays the message and saves the name
  if (NoName == 1 && strcmp(charlist, "*") == 0)
  {
    lcd.clear();
    lcd.print("Name saved!");
    delay(3000);
    writeMemory(0, Name);
    // After the name is saved the device is reset programmatically
    resetFunc();
  }
  // When ## is the combo, the device backspaces and either takes one off of the name or the task at hand
  if (strcmp(charlist, "##") == 0 && t > 100)
  {
    if (chosenDay != "")
    {
      // Since strings are ended by the character \0 this will effectively delete the last character
      tasks[dayIndex][catIndex][strlen(tasks[dayIndex][catIndex]) - 1] = '\0';
      taskindex -= 1;
      // Print the edited task to the LCD screen
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(Days[dayIndex]);
      lcd.setCursor(0, 1);
      lcd.print(tasks[dayIndex][catIndex]);
      memset(charlist, 0, sizeof charlist);
    }
    if (NoName == 1)
    {
      Name[strlen(Name) - 1] = '\0';
      nameindex -= 1;
      // Print the edited name to the LCD screen
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hello! You are?");
      lcd.setCursor(0, 1);
      lcd.print(Name);
      memset(charlist, 0, sizeof charlist);
    }
  }
  // When ### is the combo, it erases the entire task/name at hand
  if (strcmp(charlist, "###") == 0 && t > 100)
  {
    if (chosenDay != "")
    {
      // Sets the memory for the task's string to become all 0's
      memset(tasks[dayIndex][catIndex], 0, sizeof tasks[dayIndex][catIndex]);
      taskindex = 0;
      // Prints just the day to the LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(Days[dayIndex]);
      lcd.setCursor(0, 1);
      memset(charlist, 0, sizeof charlist);
    }
    if (NoName == 1)
    {
      // Sets the memory for the name string to become all 0's
      memset(Name, 0, sizeof Name);
      nameindex = 0;
      // Prints the just the new name prompt to the LCD
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Hello! You are?");
      lcd.setCursor(0, 1);
      memset(charlist, 0, sizeof charlist);
    }
  }
  // When #### is entered the entire devices EEPROM storage is wiped
  if (strcmp(charlist, "####") == 0 && t > 100)
  {
    resetMemory();
    // After the EEPROM is wiped the device is reset to load in the empty data
    resetFunc();
  }
  // When the main menu is displayed, the user can use numbers 1-7 to choose what day to edit
  if (chooseDay == 1 && customKey && chosenDay == "")
  {
    for (int r = 0; r < 7; r++)
    {
      if (customKey == dayKeys[r])
      {
        // If a day is chosen the day is displayed with the default task being 'A'
        lcd.clear();
        // The index is using the fact that the char for a digit differs from the actual byte value
        chosenDay = Days[customKey - '0' - 1];
        category = 'A';
        displayTask();
        memset(charlist, 0, sizeof charlist);
        chooseDay = 0;
      }
    }
  }
  // When a day is chosen, the user can use the keypad characters to edit/select/save the task
  if (chosenDay != "")
  {
    if (customKey)
    {
      for (int t = 0; t < 4; t++)
      {
        if (customKey == categories[t])
        {
          // Displays the selected task for the chosen day (task is synonymous with category)
          category = categories[t];
          displayTask();
          taskindex = strlen(tasks[dayIndex][catIndex]);
          memset(charlist, 0, sizeof charlist);
        }
      }
    }
    // When * is pressed, it plays the save animation and saves the current task
    if (strcmp(charlist, "*") == 0 && t > 100)
    {
      saveAnimation(chosenDay.length());
      writeMemory(addresses[dayIndex][catIndex], tasks[dayIndex][catIndex]);
      memset(charlist, 0, sizeof charlist);
    }
    // When # is pressed, it returns to the main menu
    if (strcmp(charlist, "#") == 0 && t > 100)
    {
      startMenu();
      chosenDay = "";
    }
  }
  // If the combos input is not read, it is cleared after 1 second
  if (t > 100)
  {
    memset(charlist, 0, sizeof charlist);
    index = 0;
  }
  // Adds one to the timer every 10 milliseconds
  t += 1;
  delay(10);
  // When the timer has reached 100 millisecond and their is no keypressed, the device reads the combo and gets the corresponding character
  if (!customKey && t > 100 && !chooseDay)
  {
    for (int i = 0; i < 27; i++)
    {
      // Will only add a char if the string length is below the maximum of 16
      if (strcmp(charlist, combs[i].c_str()) == 0 && strlen(tasks[dayIndex][catIndex]) < 16)
      {
        lcd.print(letters[i]);
        memset(charlist, 0, sizeof charlist);
        if (NoName == 1)
        {
          Name[nameindex] = letters[i];
          nameindex += 1;
        }
        if (chosenDay != "")
        {
          tasks[dayIndex][catIndex][taskindex] = letters[i];
          taskindex += 1;
        }
      }
    }
  }
}
// Function that handles the start menu for the agenda
void startMenu()
{
  // Asks user to choose a day and shifts message left to right
  lcd.setCursor(0, 0);
  lcd.clear();
  String menuText = "Choose a day to Read or Write";
  lcd.print(menuText);
  delay(500);

  for (int i = 0; i < menuText.length() - 11; i++)
  {
    lcd.scrollDisplayLeft();
    delay(250);
  }
  for (int i = 0; i < menuText.length() - 11; i++)
  {
    lcd.scrollDisplayRight();
    delay(250);
  }

  lcd.setCursor(0, 1);
  chooseDay = 1;
}
// Function that reads the EEPROM from the start address until it reaches 128, d and t are only for the array of tasks,
void callMemory(int startaddr, int d, int t)
{
  // Reading the EEPROM byte by byte
  for (int i = startaddr;; i++)
  {
    int value = EEPROM.read(i);
    if (value != 128)
    {
      if (value != 0)
      {
        // The user name is stored at address 0
        if (startaddr == 0)
        {
          Name[i - startaddr] = letters[value - 1];
        }
        if (startaddr != 0)
        {
          tasks[d][t][i - startaddr] = letters[value - 1];
        }
      }
    }
    else
    {
      break;
    }
  }
}
// Function that takes the saveobj parameter and starts at the EEPROM start address, iterating a writing process that saves the index of each letter in the object
void writeMemory(int startaddr, char saveobj[])
{
  int len = strlen(saveobj);
  for (int i = startaddr; i < len + startaddr; i++)
  {
    // This will store the index of the letter that matches the current char
    for (int l = 0; l < 27; l++)
    {
      if (saveobj[i - startaddr] == letters[l])
      {
        EEPROM.write(i, l + 1);
      }
    }
  }
  // 128 is the ending char for the string in the EEPROM
  EEPROM.write(startaddr + len, 128);
}
// Function that effectively clears all data in the EEPROM by assigning 128 to every address
void resetMemory()
{
  EEPROM.write(0, 128);
  for (int d = 0; d < 7; d++)
  {
    for (int t = 0; t < 4; t++)
    {
      EEPROM.write(addresses[d][t], 128);
    }
  }
}
// Function that displays the chosen day and task based on the user edited indexes
void displayTask()
{
  if (chosenDay != "")
  {
    for (int t = 0; t < 4; t++)
    {
      for (int d = 0; d < 7; d++)
      {
        if (category == categories[t] && chosenDay == Days[d])
        {
          // Prints the day and the task data to the LCD screen
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

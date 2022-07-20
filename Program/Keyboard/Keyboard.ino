#include "Wire.h";
// Rows
#define row 5
// Columns
#define col 11
// Pins assigned to each row
int ROWS[row] = {A0, A1, A2, A3, A6};
// Pins assigned to each column
int COLS[col] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
// Unique identifiers for each button on the keyboard
byte first[row][col] = {
    {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 0xF0}, // 0xF0 is backspace
    {'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', 0xF1}, // 0xF1 is enter
    {'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\''},
    {'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0xF2},          // 0xF2 is shift
    {0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA} // Home, space, back, left, up, down, right, 0xFA is empty
};
byte second[row][col] = {
    {'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', 0xF0}, // 0xF0 is backspace
    {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0xF1}, // 0xF1 is enter
    {'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"'},
    {'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0xF2},          // 0xF2 is shift
    {0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA} // Home, space, back, left, up, down, right, 0xFA are empty
};
// Determines if shift has been pressed
bool shifted;
void setup()
{
  // Setting the column pins to outputs
  for (int i = 0; i < row; i++)
  {
    pinMode[ROWS[i], INPUT_PULLUP];
  }
  for (int i = 0; i < col; i++)
  {
    pinMode[COLS[i], OUTPUT];
  }
  // Beginning I2C communication
  Wire.begin(9);
  Wire.onRequest(getKey);
}
void loop()
{
}
void getKey()
{
  // Variable used to check if shift has been pressed
  int checksum;
  // Looping through the rows and columns of the button array
  for (int i = 0; i < col; i++)
  {
    // Setting each column pin to high
    digitalWrite(COLS[i], HIGH);
    for (int j = 0; j < row; j++)
    {
      // If the row pin is read as high, the keyboard will
      // send the respective identifier
      if (analogRead(ROWS[j]) > 500)
      {
        if (first[j][i] == 0xF2)
        {
          checksum += 1;
        }
        if (!shifted)
        {
          Wire.write(first[j][i]);
        }
        else
        {
          Wire.write(second[j][i]);
        }
      }
      else
      {
        Wire.write(0xFA); // Empty char
      }
    }
    // Resetting the column pin
    digitalWrite(COLS[i], LOW);
  }
  if (checksum > 0)
  {
    shifted = true;
  }
  else
  {
    shifted = false;
  }
}

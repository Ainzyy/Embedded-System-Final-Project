#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <BluetoothSerial.h>

BluetoothSerial bt;

LiquidCrystal_I2C lcd(0x26, 20, 4);

const byte ROWS = 4;
const byte COLS = 4;
char hexaKeys[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte rowPins[ROWS] = {13, 12, 14, 27};
byte colPins[COLS] = {26, 25, 33, 32};
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

static const int solenoidRelay = 16;
static const String UID = "B3 97 37 96";
String userPassword = "1234";
const String adminPassword = "A#3*";
String newPassword = "";
String inputPassword = "";
bool adminMode = false;
bool changePasswordMode = false;
bool fromBluetooth = false;

void handleBluetooth();
void handleKeypad();
void askForPassword();
void askForAdminKey();
void displayInputPassword();
void checkPassword();
void checkAdminKey();
void askForNewPassword();
void displayNewPassword();
void checkNewPassword();
void unlockSolenoid();
void lockSolenoid();
bool isNumber(String);

void setup()
{
  Serial.begin(115200);
  bt.begin("ESP32 Lock");
  lcd.begin(20, 4);
  lcd.init();
  lcd.backlight();

  pinMode(solenoidRelay, OUTPUT);

  askForPassword();
}

void loop()
{
  handleKeypad();
  handleBluetooth();
}

void handleBluetooth()
{
  if (bt.available())
  {
    String btInput = bt.readStringUntil('\n');

    btInput.trim();

    if (changePasswordMode)
    {
      newPassword = btInput;
      fromBluetooth = true;
      checkNewPassword();
    }
    else if (adminMode)
    {
      inputPassword = btInput;
      checkAdminKey();
    }
    else
    {
      if (btInput == "*")
      {
        adminMode = true;
        inputPassword = "";
        askForAdminKey();
      }
      else
      {
        inputPassword = btInput;
        checkPassword();
      }
      inputPassword = "";
    }
  }
}

void handleKeypad()
{
  char key = customKeypad.getKey();

  if (key)
  {
    if (changePasswordMode)
    {
      if (isdigit(key))
      {
        newPassword += key;

        askForNewPassword();
        displayNewPassword();
        checkNewPassword();
      }
    }
    else if (adminMode)
    {
      inputPassword += key;

      askForAdminKey();
      displayInputPassword();
      checkAdminKey();
    }
    else
    {
      if (key == '*')
      {
        adminMode = true;
        inputPassword = "";

        askForAdminKey();
      }
      else if (isdigit(key))
      {
        inputPassword += key;

        askForPassword();
        displayInputPassword();
        checkPassword();
      }
    }
  }
}

void askForPassword()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Password:");
}

void checkPassword()
{
  if (inputPassword.length() == 4)
  {
    if (inputPassword == userPassword)
    {
      unlockSolenoid();
    }
    else
    {
      lockSolenoid();
    }
    inputPassword = "";
    askForPassword();
  }
}

void displayInputPassword()
{
  lcd.setCursor(0, 1);
  for (int i = 0; i < inputPassword.length(); i++)
  {
    lcd.print('*');
  }
  Serial.println(inputPassword);
}

void askForAdminKey()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter Admin Password:");
  Serial.println("Enter Admin Password:");
}

void checkAdminKey()
{
  if (inputPassword.length() == 4)
  {
    if (inputPassword == adminPassword)
    {
      adminMode = false;
      changePasswordMode = true;
      inputPassword = "";
      askForNewPassword();
    }
    else
    {
      adminMode = false;
      inputPassword = "";
      lcd.clear();
      lcd.print("Incorrect Admin Key!");
      delay(2000);
      askForPassword();
    }
  }
}

void askForNewPassword()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter New Password:");
}

void displayNewPassword()
{
  lcd.setCursor(0, 1);
  for (int i = 0; i < newPassword.length(); i++)
  {
    lcd.print('*');
  }
  Serial.println(newPassword);
}

void checkNewPassword()
{
  if ((newPassword.length() == 4 && !fromBluetooth ) || (fromBluetooth && newPassword.length() == 4 && isNumber(newPassword)))
  {
    userPassword = newPassword;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PASSWORD CHANGED");
    Serial.println("PASSWORD CHANGED");
    newPassword = "";
    changePasswordMode = false;
    fromBluetooth = false;
    delay(2000);
    askForPassword();
  }
  else if (fromBluetooth && newPassword.length() != 4 && !isNumber(newPassword))
  {
    changePasswordMode = false;
    newPassword = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("INVALID PASSWORD");
    lcd.setCursor(0, 3);
    lcd.print("Must be a number");
    Serial.println("INVALID PASSWORD");
    delay(2000);
    askForPassword();
  }
}

void unlockSolenoid()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UNLOCKED");
  Serial.println("UNLOCKED");

  digitalWrite(solenoidRelay, HIGH);
  delay(5000);
  digitalWrite(solenoidRelay, LOW);
}

void lockSolenoid()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INCORRECT PASSWORD");
  Serial.println("INCORRECT PASSWORD");
  delay(2000);
}

bool isNumber(String str)
{
  if (str.length() != 4)
  {
    return false;
  }

  for (int i = 0; i < 4; i++)
  {
    if (!isDigit(str[i]))
    {
      return false;
    }
  }

  return true;
}
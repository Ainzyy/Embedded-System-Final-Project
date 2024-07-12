#include <Arduino.h>

#define BLYNK_TEMPLATE_ID "TMPL6_Xb0_ZmE"
#define BLYNK_TEMPLATE_NAME "Home Automation and Dual Axis Solar Tracker"
#define BLYNK_AUTH_TOKEN "ZQDkgFCflnGNMd8QcLhNAyrbuHBWq_4B"

#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "Hello";
char password[] = "Nani!!!112358";

BlynkTimer timer; 

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C LCD(0x27, 20, 4);

#include <DHT.h>
DHT dht(25, DHT11);

#include <ESP32Servo.h>
Servo X_servo; // Horizontal servo
Servo Y_Servo; // Vertical servo

#include <BluetoothSerial.h>
BluetoothSerial BT;

// Servo Pins
#define SERVO_X_PIN 18
#define SERVO_Y_PIN 19

// Servo Min and Max
#define SERVO_X_MIN 0
#define SERVO_X_MAX 180
#define SERVO_Y_MIN 15
#define SERVO_Y_MAX 120

// Struct for LDR pins
struct LDR
{
  static const int TOP_RIGHT = 34;
  static const int TOP_LEFT = 39;
  static const int BOTTOM_RIGHT = 35;
  static const int BOTTOM_LEFT = 36;
  static const int pins[4];

  void init()
  {
    int length = sizeof(pins) / sizeof(pins[0]);
    for (int i = 0; i < length; i++)
    {
      pinMode(pins[i], OUTPUT);
    }
  }
};
const int LDR::pins[4] = {LDR::TOP_RIGHT, LDR::TOP_LEFT, LDR::BOTTOM_RIGHT, LDR::BOTTOM_LEFT};
LDR ldr;

// Struct to store servo positions
struct degrees
{
  int x;
  int y;
};
degrees servoDegrees;

// Struct for LED pins
struct Home
{
  static const int indoor = 32;
  static const int outdoor = 33;
  static const int pins[2];

  void init()
  {
    int length = sizeof(pins) / sizeof(pins[0]);
    for (int i = 0; i < length; ++i)
    {
      pinMode(pins[i], OUTPUT);
    }
  }
};
const int Home::pins[2] = {Home::indoor, Home::outdoor};
Home home;

struct Sensor
{
  float temperature;
  float humidity;
};
Sensor sensor;

void dualAxis();
bool Dual_Axis = false;
void centerText(String, int);
String getCompassDirection(int, int);

// Indoor Lights
BLYNK_WRITE(V2){
  bool indoorLightState = param.asInt();
  digitalWrite(home.indoor, indoorLightState);
  BT.println(indoorLightState ? "[+] Inside Lights are ON." : "[-] Inside Lights are OFF.");
}

// Outdoor Lights
BLYNK_WRITE(V3){
  bool outdoorLightState = param.asInt();
  digitalWrite(home.outdoor, outdoorLightState);
  BT.println(outdoorLightState ? "[+] Outdoor Lights are ON." : "[-] Outdoor Lights are OFF.");
}

// Dual Axis
BLYNK_WRITE(V4){
  Dual_Axis = param.asInt();
  BT.println(Dual_Axis ? "[+] Dual Axis Activated." : "[-] Dual Axis Deactivated.");
}

void sendSensorReadings() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);
}

void setup()
{
  Serial.begin(115200);
  LCD.begin(20, 4);
  LCD.init();
  LCD.backlight();
  BT.begin("Home Automation");
  dht.begin();
  Blynk.begin(auth, ssid, password);
  timer.setInterval(1000L, sendSensorReadings);

  Blynk.virtualWrite(V2, digitalRead(home.indoor));
  Blynk.virtualWrite(V3, digitalRead(home.outdoor));
  Blynk.virtualWrite(V4, Dual_Axis);

  // Initialize LDR pins
  ldr.init();

  // Initialize LED pins
  home.init();

  // Attach servo pins
  X_servo.attach(SERVO_X_PIN);
  Y_Servo.attach(SERVO_Y_PIN);

  // Initialize servos to middle positions
  servoDegrees.x = 90;
  servoDegrees.y = 90;
  X_servo.write(servoDegrees.x);
  Y_Servo.write(servoDegrees.y);
  delay(1000);
}

ulong DualAxisPreviousMillis = 0, LCDpreviousMillis = 0;

void loop()
{
  Blynk.run();
  timer.run();
  sensor.temperature = dht.readTemperature();
  sensor.humidity = dht.readHumidity();

  if (millis() - LCDpreviousMillis >= 2500)
  {
    LCDpreviousMillis = millis();

    LCD.clear();
    centerText("==== DHT SENSOR ====", 0);
    String sensorResult;
    sensorResult += String(sensor.temperature);
    sensorResult += char(223);
    sensorResult += "C    ";
    sensorResult += String(sensor.humidity, 2);
    sensorResult += "%";
    centerText(sensorResult, 1);

    centerText("== SOLAR POSITION ==", 2);
    String axisResult;
    axisResult += "X:";
    axisResult += String(servoDegrees.x);
    axisResult += char(223);
    axisResult += " Y:";
    axisResult += String(servoDegrees.y);
    axisResult += char(223);
    axisResult += "    ";
    axisResult += getCompassDirection(servoDegrees.x, servoDegrees.y);
    centerText(axisResult, 3);

    Serial.printf("%d  %d  %d  %d\n", analogRead(ldr.TOP_RIGHT), analogRead(ldr.TOP_LEFT), analogRead(ldr.BOTTOM_RIGHT), analogRead(ldr.BOTTOM_LEFT));
  }

  if (Dual_Axis && (millis() - DualAxisPreviousMillis >= 25))
  {
    DualAxisPreviousMillis = millis();
    dualAxis();
  }

  if (BT.available())
  {
    String input = BT.readStringUntil('\n');
    input.trim();

    if (input.equalsIgnoreCase("inside"))
    {
      digitalWrite(home.indoor, !digitalRead(home.indoor));
      (digitalRead(home.indoor) == HIGH) ? BT.println("[+] Inside Lights are ON.") : BT.println("[-] Inside Lights are OFF.");
      Blynk.virtualWrite(V2, digitalRead(home.indoor));
    }
    else if (input.equalsIgnoreCase("outside"))
    {
      digitalWrite(home.outdoor, !digitalRead(home.outdoor));
      (digitalRead(home.outdoor) == HIGH) ? BT.println("[+] Outside Lights are ON.") : BT.println("[-] Outside Lights are OFF.");
      Blynk.virtualWrite(V3, digitalRead(home.outdoor));
    }
    else if (input.equalsIgnoreCase("dual axis"))
    {
      Dual_Axis = !Dual_Axis;

      if (Dual_Axis)
      {
        BT.println("[+] Dual Axis Activated.");
      }
      else
      {
        BT.println("[-] Dual Axis Deactivated.");
      }
      Blynk.virtualWrite(V4, Dual_Axis);
    }
    else if (input.equalsIgnoreCase("dht Sensor"))
    {
      BT.println("\n===== DHT SENSOR =====");
      BT.printf("Temperature: %0.2f\nHumidity: %0.2f%\n",
                sensor.temperature, sensor.humidity);
      BT.println("======================\n");
    }
    else
    {
      BT.println("[ ! ] Invalid Command!");
    }
  }
}

void dualAxis()
{
  // Read LDR values
  int topLeft = analogRead(ldr.TOP_LEFT);
  int topRight = analogRead(ldr.TOP_RIGHT);
  int bottomLeft = analogRead(ldr.BOTTOM_LEFT);
  int bottomRight = analogRead(ldr.BOTTOM_RIGHT);

  // Calculate averages
  int avgTop = (topLeft + topRight) / 2;
  int avgBottom = (bottomLeft + bottomRight) / 2;
  int avgLeft = (topLeft + bottomLeft) / 2;
  int avgRight = (topRight + bottomRight) / 2;

  // Determine the direction to move the servos
  if (avgTop > avgBottom + 50)
  {
    servoDegrees.y = constrain(servoDegrees.y + 1, SERVO_Y_MIN, SERVO_Y_MAX);
  }
  else if (avgBottom > avgTop + 50)
  {
    servoDegrees.y = constrain(servoDegrees.y - 1, SERVO_Y_MIN, SERVO_Y_MAX);
  }

  if (avgLeft > avgRight + 50)
  {
    servoDegrees.x = constrain(servoDegrees.x - 1, SERVO_X_MIN, SERVO_X_MAX);
  }
  else if (avgRight > avgLeft + 50)
  {
    servoDegrees.x = constrain(servoDegrees.x + 1, SERVO_X_MIN, SERVO_X_MAX);
  }

  // Write new positions to servos
  X_servo.write(servoDegrees.x);
  Y_Servo.write(servoDegrees.y);
}

void centerText(String text, int row)
{
  int textLength = text.length();
  int spaces = (20 - textLength) / 2;

  if (spaces >= 0)
  {
    LCD.setCursor(spaces, row);
    LCD.print(text);
  }
  else
  {
    LCD.setCursor(0, row);
    LCD.print(text.substring(0, 20));
  }
}

String getCompassDirection(int x, int y) {
  if (x < 36) {
    if (y < 90) {
      return "W";
    } else if (y > 85 && y < 95) {
      return "MID";
    } else {
      return "E";
    }
  } else if (x >= 36 && x <= 71) {
    if (y < 90) {
      return "SW";
    } else if (y > 85 && y < 95) {
      return "MID";
    } else {
      return "NE";
    }
  } else if (x >= 72 && x <= 107) {
    if (y < 90) {
      return "S";
    } else if (y > 85 && y < 95) {
      return "MID";
    } else {
      return "N";
    }
  } else if (x >= 108 && x <= 143) {
    if (y < 90) {
      return "SE";
    } else if (y > 85 && y < 95) {
      return "MID";
    } else {
      return "NW";
    }
  } else {
    if (y < 90) {
      return "E";
    } else if (y > 85 && y < 95) {
      return "MID";
    } else {
      return "W";
    }
  }
}
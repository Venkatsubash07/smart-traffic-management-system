define BLYNK_TEMPLATE_ID "TMPL328Vzeno9"
define BLYNK_TEMPLATE_NAME "smart traffic management system"
define BLYNK_AUTH_TOKEN "Add your authentication token here"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// WiFi
char ssid[] = "smart_traffic";
char pass[] = "ECS@1234";

// LCD
LiquidCrystal_I2C lcd(0x27,16,2);

// LED
#define NUM_LEDS 4
#define LED_PIN 4
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Sensors
int irEast=34;
int irNorth=35;
int irWest=32;
int irSouth=33;

// Emergency buttons
int btnEast=25;
int btnNorth=26;
int btnWest=27;
int btnSouth=14;

// Buzzer
int buzzer = 15;

// Previous states
bool prevEast=HIGH, prevNorth=HIGH, prevWest=HIGH, prevSouth=HIGH;

// Counts
int eastCount=0;
int northCount=0;
int westCount=0;
int southCount=0;

// Traffic
int baseTime = 10;
int currentDir = 0;
int timerValue = 10;

int emergencyDir = -1;

unsigned long prevMillis = 0;

BlynkTimer timer;

// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(115200);

  pinMode(irEast,INPUT);
  pinMode(irNorth,INPUT);
  pinMode(irWest,INPUT);
  pinMode(irSouth,INPUT);

  pinMode(btnEast,INPUT_PULLUP);
  pinMode(btnNorth,INPUT_PULLUP);
  pinMode(btnWest,INPUT_PULLUP);
  pinMode(btnSouth,INPUT_PULLUP);

  pinMode(buzzer,OUTPUT);

  pixels.begin();
  pixels.show();

  lcd.init();
  lcd.backlight();

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, sendData);
}

// ---------------- LOOP ----------------
void loop()
{
  Blynk.run();
  timer.run();

  readSensors();
  checkEmergency();
  updateTraffic();
}

// ---------------- SENSOR ----------------
void readSensors()
{
  int currEast = digitalRead(irEast);
  int currNorth = digitalRead(irNorth);
  int currWest = digitalRead(irWest);
  int currSouth = digitalRead(irSouth);

  if((currEast == LOW && prevEast == HIGH)) eastCount++;
  if((currNorth == LOW && prevNorth == HIGH)) northCount++;
  if((currWest == LOW && prevWest == HIGH)) westCount++;
  if((currSouth == LOW && prevSouth == HIGH)) southCount++;

  prevEast = currEast;
  prevNorth = currNorth;
  prevWest = currWest;
  prevSouth = currSouth;
}

// ---------------- EMERGENCY (FIXED - NO DELAY) ----------------
void checkEmergency()
{
  static bool buzzerTriggered = false;

  int pressedDir = -1;

  if(digitalRead(btnEast)==LOW)  pressedDir = 0;
  if(digitalRead(btnNorth)==LOW) pressedDir = 1;
  if(digitalRead(btnWest)==LOW)  pressedDir = 2;
  if(digitalRead(btnSouth)==LOW) pressedDir = 3;

  if(pressedDir != -1)
  {
    emergencyDir = pressedDir;

    // 🔊 single trigger buzzer
    if(!buzzerTriggered)
    {
      digitalWrite(buzzer, HIGH);
      buzzerTriggered = true;
    }

    currentDir = emergencyDir;
    timerValue = baseTime;
  }
  else
  {
    digitalWrite(buzzer, LOW);
    buzzerTriggered = false;
  }
}

// ---------------- TRAFFIC ----------------
void updateTraffic()
{
  unsigned long currentMillis = millis();

  if(currentMillis - prevMillis >= 1000)
  {
    prevMillis = currentMillis;

    timerValue--;

    if(currentDir == 0 && eastCount > 0) eastCount--;
    if(currentDir == 1 && northCount > 0) northCount--;
    if(currentDir == 2 && westCount > 0) westCount--;
    if(currentDir == 3 && southCount > 0) southCount--;

    if(timerValue <= 0)
    {
      if(emergencyDir != -1)
      {
        emergencyDir = -1;
      }

      currentDir = (currentDir + 1) % 4;
      timerValue = baseTime;
    }
  }

  updateLEDs();
  displayLCD();
}

// ---------------- LED ----------------
void updateLEDs()
{
  for(int i=0;i<4;i++)
  {
    if(i == currentDir)
      pixels.setPixelColor(i, pixels.Color(0,150,0));
    else
      pixels.setPixelColor(i, pixels.Color(150,0,0));
  }
  pixels.show();
}

// ---------------- DISPLAY ----------------
void displayLCD()
{
  lcd.clear();

  lcd.setCursor(0,0);

  if(currentDir == 0) lcd.print("EAST ");
  if(currentDir == 1) lcd.print("NORTH ");
  if(currentDir == 2) lcd.print("WEST ");
  if(currentDir == 3) lcd.print("SOUTH ");

  lcd.print(timerValue);

  lcd.setCursor(0,1);
  lcd.print("E:");
  lcd.print(eastCount);
  lcd.print(" N:");
  lcd.print(northCount);
  lcd.print(" W:");
  lcd.print(westCount);
  lcd.print(" S:");
  lcd.print(southCount);
}

// ---------------- BLYNK ----------------
void sendData()
{
  Blynk.virtualWrite(V0, eastCount);
  Blynk.virtualWrite(V1, northCount);
  Blynk.virtualWrite(V2, westCount);
  Blynk.virtualWrite(V3, southCount);
}

#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Servo.h>
#include <Wire.h>
#include "rgb_lcd.h"

#include "arduino_secrets.h"


#include <SPI.h>
#include <BlynkSimpleWifi.h>

// WiFi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Telegram bot token
const char* botToken = TELEGRAM_TOKEN;

WiFiSSLClient client;
UniversalTelegramBot bot(botToken, client);

// store last Telegram chat
String chat_id = "";

// LCD
rgb_lcd lcd;

// Button
const int buttonPin = 4;
int buttonState = HIGH;
bool lastButtonState = HIGH;

// Colors
const int activeR = 0;
const int activeG = 255;
const int activeB = 0;

const int idleR = 255;
const int idleG = 0;
const int idleB = 0;

// Servo
Servo myservo;

bool isFeeding = false;
unsigned long feedStartTime = 0;
const unsigned long feedDuration = 500; 

int status = WL_IDLE_STATUS;

unsigned long lastTimeBotRan = 0;
const unsigned long botRequestDelay = 1000;

bool isIdleColor = false;
unsigned long lastLCDUpdate = 0;
unsigned long lastFeedTime = 0;

// -------- FEED FUNCTION --------
void feedPet() {

  if (isFeeding) return; 

  lcd.setCursor(0,0);
  lcd.print("Feeding...      ");

  myservo.write(180);

  isFeeding = true;
  feedStartTime = millis();

  lastFeedTime = millis();
  isIdleColor = false;

  lcd.setRGB(activeR, activeG, activeB);

  lcd.setCursor(0,1);
  lcd.print("Please wait     ");
}

// -------- HANDLE SERVO RETURN --------
void updateFeeding() {

  if (isFeeding && millis() - feedStartTime >= feedDuration) {

    myservo.write(0);
    isFeeding = false;

    lcd.setCursor(0,0);
    lcd.print("Food given!     ");
  }
}

// -------- TELEGRAM --------
void handleNewMessages(int numNewMessages) {

  for (int i = 0; i < numNewMessages; i++) {

    chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    Serial.print("Message: ");
    Serial.println(text);

    lcd.setCursor(0,0);
    lcd.print("Msg received    ");
    lcd.setCursor(0,1);
    lcd.print(text.substring(0,16));

    if (text == "/feed") {
      bot.sendMessage(chat_id, "Food dispensed!", "");
      feedPet();
    }

    if (text == "/start") {

      String welcome = "Servo Control Bot\n";
      welcome += "/feed - feed pet";

      bot.sendMessage(chat_id, welcome, "");

      lcd.clear();
      lcd.print("Bot Ready!");
    }
  }
}

BLYNK_WRITE(V0) {   // Button on app (Virtual Pin V0)

  int value = param.asInt();

  if (value == 1) {
    feedPet();

    if(chat_id != "") {
      bot.sendMessage(chat_id, "Food dispensed from Blynk!", "");
    }
  }
}

// -------- SETUP --------
void setup() {

  pinMode(buttonPin, INPUT_PULLUP);

  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.setRGB(activeR, activeG, activeB);
  lcd.print("Starting...");

  myservo.attach(8);

  // WiFi check
  if (WiFi.status() == WL_NO_MODULE) {

    Serial.println("WiFi module failed!");

    lcd.clear();
    lcd.print("WiFi Failed!");

    while (true);
  }

  lcd.clear();
  lcd.print("Connecting WiFi");

  while (status != WL_CONNECTED) {

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);

    delay(3000); 
  }

  Serial.println("WiFi connected!");

  lcd.clear();
  lcd.print("WiFi connected");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());


  Blynk.config(BLYNK_AUTH_TOKEN);
  Blynk.connect();

  lastFeedTime = millis();
}

// -------- LOOP --------
void loop() {
  Blynk.run();
  updateFeeding();

  // -------- BUTTON --------
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW && lastButtonState == HIGH) {

    if(chat_id != "") {
      bot.sendMessage(chat_id, "Food dispensed by button!", "");
    }

    feedPet();
  }

  lastButtonState = buttonState;

  // -------- TIMER --------
  unsigned long timeSinceFeed = (millis() - lastFeedTime) / 1000;

  if (millis() - lastLCDUpdate > 1000) {

    lcd.setCursor(0,0);
    lcd.print("Last feed:      ");

    lcd.setCursor(0,1);
    lcd.print(timeSinceFeed);
    lcd.print("s ago     ");

    lastLCDUpdate = millis();
  }

  // -------- COLOR --------
  if (timeSinceFeed >= 10 && !isIdleColor) {

    lcd.setRGB(idleR, idleG, idleB);
    isIdleColor = true;
  }

  if (timeSinceFeed < 10 && isIdleColor) {

    lcd.setRGB(activeR, activeG, activeB);
    isIdleColor = false;
  }

  // -------- TELEGRAM --------
  if (millis() - lastTimeBotRan > botRequestDelay) {

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {

      Serial.println("New Telegram message received!");

      handleNewMessages(numNewMessages);

      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }

    lastTimeBotRan = millis();
  }
}
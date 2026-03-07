#include <WiFiS3.h>
#include <WiFiSSLClient.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Servo.h>

#include "arduino_secrets.h"

// WiFi credentials
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Telegram bot token
const char* botToken = TELEGRAM_TOKEN;

WiFiSSLClient client;
UniversalTelegramBot bot(botToken, client);

// Servo
Servo myservo;

int status = WL_IDLE_STATUS;

unsigned long lastTimeBotRan = 0;
const unsigned long botRequestDelay = 1000;

void handleNewMessages(int numNewMessages);

void setup() {

  Serial.begin(9600);
  myservo.attach(8);

  while (!Serial);

  // Check WiFi module
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module failed!");
    while (true);
  }

  // Connect to WiFi
  while (status != WL_CONNECTED) {

    Serial.print("Connecting to WiFi: ");
    Serial.println(ssid);

    status = WiFi.begin(ssid, pass);

    delay(5000);
  }

  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

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

void handleNewMessages(int numNewMessages) {

  for (int i = 0; i < numNewMessages; i++) {

    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;

    Serial.print("Message: ");
    Serial.println(text);

    if (text == "/feed") {

      bot.sendMessage(chat_id, "The food is spilled !", "");

      myservo.write(180);
      delay(500);
      myservo.write(0);
    }

    if (text == "/start") {

      String welcome = "Servo Control Bot\n";
      welcome += "/feed - rotate servo 180°";

      bot.sendMessage(chat_id, welcome, "");
    }
  }
}
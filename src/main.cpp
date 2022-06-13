#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
// #include <MQUnifiedsensor.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "CTBot.h"
CTBot myBot;

const char *ssid = "Kochengimoet";      // silakan disesuaikan sendiri
const char *password = "gratisanterus"; // silakan disesuaikan sendiri
String token = "5426893950:AAG5DZKvBkA--7kb6G4_2fRv9sPxjEzKdkM";
const int id = 1163749622;

const char *mqtt_server = "broker.hivemq.com";

#define Board "Arduino"
#define Voltage_Resolution 5
#define sensorMQ2 A0          // Analog input 0 of your arduino
#define type "MQ-2"           // MQ2
#define ADC_Bit_Resolution 10 // For arduino UNO/MEGA/NANO
#define RatioMQ4CleanAir 9.83 // RS / R0 = 9.83 ppm

#define RED_LED D5   // led warna merah
#define GREEN_LED D6 // led warna hijau
#define BLUE_LED D7  // led warnah biru
#define buzzer D3

// MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, sensorMQ2, type);
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient espClient;
PubSubClient client(espClient);

long now = millis();
long lastMeasure = 0;
String macAddress = "";

int gas;

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  macAddress = WiFi.macAddress();
  Serial.print(WiFi.macAddress());
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(macAddress.c_str()))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.println("Starting TelegramBot...");
  myBot.wifiConnect(ssid, password);
  myBot.setTelegramToken(token);
  if (myBot.testConnection())
  {
    Serial.println("Koneksi Bagus");
  }
  else
  {
    Serial.println("Koneksi Jelek");
  }
  Wire.begin();
  Serial.begin(115200);
  Serial.println("Mqtt Node-RED");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  pinMode(RED_LED, OUTPUT); // atur pin-pin digital sebagai output
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(buzzer, OUTPUT);
  Serial.println("Contoh Penggunaan Sensor MQ2");
  // lcd.begin(16,2);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.clear();
  lcd.home();
  delay(3000);
  {
    Wire.begin();
    Serial.begin(115200);
    Serial.println("\nI2C Scanner");
  }
}

void scrollText(int row, String message, int delayTime, int lcdColumns)
{
  for (int i = 0; i < lcdColumns; i++)
  {
    message = " " + message;
  }
  message = message + " ";
  for (int pos = 0; pos < message.length(); pos++)
  {
    lcd.setCursor(0, row);
    lcd.print(message.substring(pos, pos + lcdColumns));
    delay(delayTime);
  }
}

// void printkurangdarinol(int data)
// {
//     if (data >= 0 && data < 10)
//     {
//         lcd.write('0');
//     }
//     lcd.print(data);
// }

void loop()
{
  gas = analogRead(sensorMQ2);
  Serial.print("Nilai Sensor : ");
  Serial.println(gas);
  delay(1000);

  if (gas <= 190)
  {
    Serial.println("Gas dalam kondisi aman!");
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, HIGH);
    digitalWrite(BLUE_LED, LOW);
    noTone(buzzer);
    myBot.sendMessage(id, "Gas dalam kondisi aman!");
    delay(1000);
  }

  if (190 < gas && gas <= 385)
  {
    Serial.println("Terdeteksi adanya kebocoran gas!");
    digitalWrite(RED_LED, LOW);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, HIGH);
    tone(buzzer, 370);
    myBot.sendMessage(id, "Terdeteksi adanya kebocoran gas!");
    delay(1000);
  }

  if (gas > 385)
  {
    Serial.println("Kebocoran gas darurat!");
    digitalWrite(RED_LED, HIGH);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(BLUE_LED, LOW);
    tone(buzzer, 500);
    myBot.sendMessage(id, "Kebocoran gas darurat!");
    delay(1000);
  }

  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect(macAddress.c_str());
  }
  now = millis();
  if (now - lastMeasure > 5000)
  {
    lastMeasure = now;

    // Reading LDR Censors
    gas = analogRead(sensorMQ2);
    static char damp[7];

    // Convert
    dtostrf(gas, 4, 2, damp);

    Serial.println(damp);

    // Publish hasil output dari tiap sensor berdasarkan topic
    client.publish("1941720043/room/damp", damp);
  }

  lcd.setCursor(0, 1);
  scrollText(1, "Hasil deteksi kebocoran gas", 250, 16);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(gas);
  lcd.print(" ");
  lcd.print("ppm");
}
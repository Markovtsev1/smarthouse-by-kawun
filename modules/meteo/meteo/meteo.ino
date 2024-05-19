#include <NTPClient.h>                  // NTP клиент 
#include <ESP8266WiFi.h>                // Предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <UniversalTelegramBot.h>       // Библиотека для работы с телеграмом
#include <DHT.h>                        // Библиотека для работы с датчиком температуры
#include <DNSServer.h>                  // Локальный DNS сервер для перенаправления всех запросов на страницу конфигурации
#include <ESP8266WebServer.h>           // Локальный веб сервер для страници конфигурации WiFi
#include <WiFiManager.h>                // Библиотека для удобного подключения к WiFi
#include <ESP8266HTTPClient.h>          // HTTP клиент
#include <ArduinoJson.h>                // Библиотека для работы с JSON
#include <ESP8266Firebase.h>            // Библиотека для работы с Firebase
#include "config.h"

// Параметры сети WiFi
WiFiClientSecure secured_client;

Firebase firebase(DB_URL);
unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;


X509List cert(TELEGRAM_CERTIFICATE_ROOT);

// Задержка для ответов бота
const unsigned long BOT_MTBS = 3000;

UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;


// Пин для датчика температуры
#define DHTPIN 14 
DHT dht(DHTPIN, DHT11);


const String lat = "53.90";             // Географические координаты
const String lon = "27.567";            // Минск



//////////////////NTPClient///////////////

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org");

///////////////Переменные/////////////////

WiFiManager wifiManager;
HTTPClient http;
DynamicJsonDocument doc(1500); 

byte hh, mm, ss;                        // Часы минуты секунды
String roomName = "Комната 1";
                                        // Переменные для хранения точек отсчета
unsigned long timing, rndTiming, LostWiFiMillis, lastUpdWeather, lastUpdData; 

String timeStr;                         // Строка с временем с нулями через точку


bool LostWiFi = false;                  //Флаг потери WiFi

int internetTemp, internetMinTemp, internetMaxTemp, weatherID;
byte humidity, clouds;                  //Влажность и облака в процентах
String location, weather, description;  //Местоположение, погода, подробное описание погоды 
float wind;                             //Ветер в м/с
long timeOffset;                        //Оффсет времени
byte httpErrCount = 0;                  //Счетчик ошибок получения погоды 


void setup() {
  Serial.begin(9600);
  dht.begin();
  configTime(0,0,"pool.ntp.org");
  secured_client.setTrustAnchors(&cert);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);            // Подключаемся к WiFi сети

  while (WiFi.status() != WL_CONNECTED)            // Пока соеденение не установлено, выполняем задержку
    delay(300);
  bot.sendMessage(CHAT_ID, "Подключено успешно!", "");
  int code = weatherUpdate();                      //Обновление погоды
  if (code!=200){
    bot.sendMessage(CHAT_ID, "Ошибка соединения с openweathermap.org!");
  }
  timeClient.begin();                              //Инициализация NTP клиента
  timeClient.setTimeOffset(timeOffset);            //Установка оффсета времени
}

void loop() {

  timeClient.update();   
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();             //Обновление времени

  
  if (millis() - lastUpdWeather > 120000){         //Обновление погоды раз в 2 минуты
      weatherUpdate();
      lastUpdWeather = millis();
  }

  int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
  while (numNewMessages)
  {
    handleNewMessages(numNewMessages);
    numNewMessages = bot.getUpdates(bot.last_message_received+1);
  }
}

void handleNewMessages(int numNewMessages)
{
  for (int i=0;i<numNewMessages;i++)
  {
    if (bot.messages[i].chat_id == CHAT_ID)
    {
      String text = bot.messages[i].text;
      if (text == "/room_state")
      {
        sendRoomState();
      }

      if (text == "/outside_state")
      {
        sendOutsideState();
      }

      if (text.startsWith("/changename "))
      {
        sendChangeName(text);
      }
    }
  }
}


int weatherUpdate() {                  
  if (WiFi.status() == WL_CONNECTED) { 
    String httpStr = String("http://api.openweathermap.org/data/2.5/weather") + String("?lat=") + String(lat) + String("&lon=") + String(lon) + String("&appid=") + String(appid) + String("&units=metric&lang=en");
    WiFiClient client;           // Создание WiFi клиента
    HTTPClient http;
    http.begin(client, httpStr); // Использование WiFi клиента

    int httpCode = http.GET();         
    String json = http.getString();    
    http.end();

    if(httpCode != 200) {              
      httpErrCount++;                  
      return httpCode;                 
    }
                                       
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {                       
      return httpCode;                 
    }
    
    internetTemp = doc["main"]["temp"];        
    internetMinTemp = doc["main"]["temp_min"];
    internetMaxTemp = doc["main"]["temp_max"];
    wind = doc["wind"]["speed"];
    description = doc["weather"][0]["description"].as<String>();
    weather = doc["weather"][0]["main"].as<String>();
    humidity = doc["main"]["humidity"];
    clouds = doc["clouds"]["all"];
    location = doc["name"].as<String>();
    timeOffset = doc["timezone"];
    weatherID = doc["weather"][0]["id"];
    
    httpErrCount = 0;                  
    
    return httpCode;                   
  } else         // Возвращаем какое-то значение, если WiFi не подключен
      return -1; // Например, -1 для обозначения отсутствия подключения
  
}

String currentTime() {
  return (hh < 10 ? "0" + String(hh) : String(hh)) + ":" + 
         (mm < 10 ? "0" + String(mm) : String(mm)) + ":" + 
         (ss < 10 ? "0" + String(ss) : String(ss));
}

void sendRoomState()
{
  int temp = int(dht.readTemperature());
  int humidity = int(dht.readHumidity());
  String roomReport = String("Отчет по комнате ") + roomName + " " + currentTime() + " " + currentData() + "\n" +
                      "Температура в комнате: " + String(temp) + "°C\n" +
                      "Влажность: " + String(humidity) + "%";

  bot.sendMessage(CHAT_ID, roomReport, "");
}


String currentData() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return "09.02.2006";
  }
  char buffer[11];  // Для формата DD-MM-YYYY
  strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
  return String(buffer);
}

void sendOutsideState()
{
  String weatherReport = "Отчет по улице " + currentTime() + " " + currentData() + "\n";
  weatherReport += "Температура: " + String(internetTemp) + "°C\n";
  weatherReport += "Минимальная температура: " + String(internetMinTemp) + "°C\n";
  weatherReport += "Максимальная температура: " + String(internetMaxTemp) + "°C\n";
  weatherReport += "Ветер: " + String(wind) + " м/с\n";
  weatherReport += "Описание: " + description + "\n";
  weatherReport += "Тип погоды: " + weather + "\n";
  weatherReport += "Влажность: " + String(humidity) + "%\n";
  weatherReport += "Облачность: " + String(clouds) + "%\n";
  weatherReport += "Местоположение: " + location + "\n";

  bot.sendMessage(CHAT_ID, weatherReport, "");
}

void sendChangeName(String text)
{
  if (text.length()>12)
  {
    String roomName = text.substring(12);
    String str = "Имя комнаты изменено на " + roomName;
    bot.sendMessage(CHAT_ID, str, "");
  }
  else bot.sendMessage(CHAT_ID, "Имя комнаты не может быть пустым!", "");
}
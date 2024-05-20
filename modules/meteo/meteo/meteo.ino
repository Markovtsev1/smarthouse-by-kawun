#include <NTPClient.h>                  // NTP клиент 
#include <ESP8266WiFi.h>                // Предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <UniversalTelegramBot.h>       // Библиотека для работы с телеграмом
#include <DHT.h>                        // Библиотека для работы с датчиком температуры
#include <DNSServer.h>                  // Локальный DNS сервер для перенаправления всех запросов на страницу конфигурации
#include <ESP8266WebServer.h>           // Локальный веб сервер для страници конфигурации WiFi
#include <WiFiManager.h>                // Библиотека для удобного подключения к WiFi
#include <ESP8266HTTPClient.h>          // HTTP клиент
#include <ArduinoJson.h>                // Библиотека для работы с JSON
#include "config.h"
#include <Firebase_ESP_Client.h>


// Параметры сети WiFi
WiFiClientSecure secured_client;

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

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;




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
  
  config.api_key = API_KEY;

  /* Assign the user sign in credentials */
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
  Firebase.reconnectNetwork(true);

  // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
  // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
  fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

  // Limit the size of response payload to be collected in FirebaseData
  fbdo.setResponseSize(2048);

  Firebase.begin(&config, &auth);

  Firebase.setDoubleDigits(5);

  config.timeout.serverResponse = 10 * 1000;

  getParamsFromDB();
  sendReport();
}

void loop() {

  timeClient.update();   
  hh = timeClient.getHours();
  mm = timeClient.getMinutes();
  ss = timeClient.getSeconds();             //Обновление времени

  
  if (millis() - lastUpdWeather > 120000){         //Обновление погоды раз в 2 минуты
      weatherUpdate();
      lastUpdWeather = millis();
      sendReport();
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

      if (text == "/sendreport")
      {
        sendReport();
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
    roomName = text.substring(12);
    String str = "Имя комнаты изменено на " + roomName;
    String path = ROOM_PARAMS_PATH;
    if (Firebase.RTDB.setString(&fbdo, path.c_str(), roomName.c_str())) {
    // Отправляем сообщение в Telegram о успешном изменении названия комнаты
    String message = "Название комнаты успешно изменено на " + roomName;
    bot.sendMessage(CHAT_ID, message, "");
  } else {
    // Отправляем сообщение в Telegram о неудачной попытке изменения названия комнаты
            bot.sendMessage(CHAT_ID, "Не удалось изменить название комнаты в базе данных", "");
  }
  }
  else bot.sendMessage(CHAT_ID, "Имя комнаты не может быть пустым!", "");
}

void getParamsFromDB() {
  // Получение пути к имени комнаты
  String roomPath = ROOM_PARAMS_PATH;

  // Получение названия комнаты из базы данных Firebase
  if (Firebase.RTDB.getString(&fbdo, roomPath.c_str())) {
    // Если получение успешно, сохраняем название комнаты в переменную roomName
    roomName = fbdo.stringData();
    bot.sendMessage(CHAT_ID, "Название комнаты успешно получено", "");
  } else {
    bot.sendMessage(CHAT_ID, "Не удалось получить название комнаты", "");
    roomName = ""; // Если не удалось получить название комнаты, возвращаем пустую строку
  }
}

String currentDataForJSON() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "09-02-2006";
  }
  char buffer[11];  // Для формата DD-MM-YYYY
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
  return String(buffer);
}

String currentTimeForJSON() {
  return (hh < 10 ? "0" + String(hh) : String(hh)) + ":" +
         (mm < 10 ? "0" + String(mm) : String(mm))  + ":" +
         (ss < 10 ? "0" + String(ss) : String(ss));
}
void sendReport()
{
  // Считывание данных с датчика
  float humidity = dht.readHumidity();
  float temp = dht.readTemperature();

  // Создание JSON файла
  String jsonData = "{\"temperature\":\"" + String(temp) + "°C\", \"humidity\":\"" + String(humidity) + "%\"}";
  FirebaseJson json;
  json.setJsonData(jsonData);

  String path = REPORTS_PATH;                             
  path += "/"   + currentDataForJSON() + " " + currentTimeForJSON();            

  if (!Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json))  // Передаем указатель на json
  {
     bot.sendMessage(CHAT_ID, fbdo.errorReason().c_str(), "");
  }
}

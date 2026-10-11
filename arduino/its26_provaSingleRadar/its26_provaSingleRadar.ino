// wifi section
#include <Math.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <stdio.h>
#include <Wire.h>
//#include "Grove_Temperature_And_Humidity_Sensor.h"
#include <UltrasonicSensorLibrary.h>

const char *ssid = "retehp";
const char *password = "a1b2c3d4";
const char *serverAddress = "http://192.168.137.1:8081";
const int ipkey = 50;
const char deviceApiKey[] = "z02Zs1A_d_vasNTOEFwLPtSqPP8fnvxMpvSwil_Yc00";

JsonDocument iot;

//#define DHTTYPE DHT20
//DHT dht(DHTTYPE);
float temperature=25;
float humidity=0;
int alarm2 = 0;
float val2 = 0;
float min2 = 20;
float max2 = 50;

const int ledPin = 1;
int sec = 0;
const int ultraSig = 12;
Ultrasonic sensor1(ultraSig);
int alarm1 = 0;
float val1 = 0;
float min1 = 20;
float max1 = 50;
// variabili per controllo di fermezza radar
int distBase;
int distMax = 10;
int steadyAlarm = 0;
int maxDistReached = 0;

bool connessioneWIFI();
void getIot();
void setRange();
void showStatus();
void blinkLed(int flag);
void checkSensor1();
void checkSensor2();

// gestione due task millis()

unsigned long previousMillisLED = 0;
const long intervalLED = 100;
bool blinkingflag = true;
// Variabili per il tuo nuovo codice (ogni 1000 ms)
unsigned long previousMillisTask = 0;
const long intervalTask = 1000;


void setup() {
  Serial.begin(115200);
  WiFi.onEvent([](arduino_event_id_t event, arduino_event_info_t info) {
    if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
      Serial.println("Evento Wi-Fi: associato all'access point.");
    } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
      Serial.println("Evento Wi-Fi: IP configurato.");
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
      Serial.print("Evento Wi-Fi: disconnesso, motivo=");
      Serial.println(static_cast<int>(info.wifi_sta_disconnected.reason));
    }
  });

  delay(5000);
  Serial.println("Cronometro!! in secondi");
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ledPin, OUTPUT);
  Wire.begin();
  //dht.begin();

  if (connessioneWIFI()) {
    getIot();
  } else {
    Serial.println("GET non eseguita: Wi-Fi non connesso.");
  }
}

void loop() {
  unsigned long currentMillis = millis();  // Leggiamo il tempo una sola volta per ciclo

  if (currentMillis - previousMillisTask >= intervalTask) {
    previousMillisTask = currentMillis;  // Aggiorna il timer del task
    loopMain();
  }
  if (currentMillis - previousMillisLED >= intervalLED) {
    previousMillisLED = currentMillis;  // Aggiorna il timer del LED
    if (steadyAlarm ) blinking();
  }
}

void loopMain() {
  //ogni secondo gira loopmain
  sec++;
  checkSensor1();
  //checkSensor2();
  showStatus();
  int dist = abs(distBase - (int)val1);

  if (dist > maxDistReached)
    maxDistReached = dist;
  Serial.print("---val1=");
  Serial.print(val1);
  Serial.print("---distbase=");
  Serial.print(distBase);
  Serial.print("---dist=");
  Serial.print(dist);

  if (!steadyAlarm)
    if (dist > distMax) {
      steadyAlarm = 1;
    }
  Serial.print("---distMax=");
  Serial.print(distMax);
  Serial.print("---maxDistReached=");
  Serial.println(maxDistReached);

  if (sec % 10 == 0) {
    // ogni 10 sec mi salvo la distanza attuale da tenere per 10 sec
    distBase = (int)val1;
    postData();
    maxDistReached = 0;
    steadyAlarm = 0;
  }
}

bool connessioneWIFI() {
  Serial.print("Connessione a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  IPAddress localIP(192, 168, 137, ipkey);
  IPAddress gateway(192, 168, 137, 1);
  IPAddress subnet(255, 255, 255, 0);
  IPAddress dns(192, 168, 137, 1);
  if (!WiFi.config(localIP, gateway, subnet, dns)) {
    Serial.println("Impossibile configurare l'IP statico.");
  }

  WiFi.begin(ssid, password);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 12000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nConnessione Wi-Fi fallita.");
    Serial.print("Stato Wi-Fi: ");
    Serial.println(static_cast<int>(WiFi.status()));
    Serial.print("IP ottenuto: ");
    Serial.println(WiFi.localIP());
    return false;
  }

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println("\nWiFi connesso!");
  return true;
}

void setMaxDist() {
  const char *jsonRangeStr = iot["jsonrange"];
  if (jsonRangeStr == nullptr) {
    Serial.println("jsonrange assente nel record IoT.");
    return;
  }

  JsonDocument rangeDoc;
  DeserializationError error = deserializeJson(rangeDoc, jsonRangeStr);
  if (error) {
    Serial.print("Errore nel parsing di jsonrange: ");
    Serial.println(error.c_str());
    return;
  }

  if (rangeDoc.size() > 0)  // check su primo sensore
  {
    if (!rangeDoc[0]["distmax"].isNull()) {
      distMax = rangeDoc[0]["distmax"].as<int>();
      Serial.print("distMax aggiornato dal JSON: ");
      Serial.println(distMax);
    }
  }
}

void setRange() {
  const char *jsonRangeStr = iot["jsonrange"];
  if (jsonRangeStr == nullptr) {
    Serial.println("jsonrange assente nel record IoT.");
    return;
  }

  JsonDocument rangeDoc;
  DeserializationError error = deserializeJson(rangeDoc, jsonRangeStr);
  if (error) {
    Serial.print("Errore nel parsing di jsonrange: ");
    Serial.println(error.c_str());
    return;
  }

  if (rangeDoc.size() > 0) {
    min1 = rangeDoc[0]["min"] | min1;
    max1 = rangeDoc[0]["max"] | max1;
  }
  if (rangeDoc.size() > 1) {
    min2 = rangeDoc[1]["min"] | min2;
    max2 = rangeDoc[1]["max"] | max2;
  }
}

void showStatus() {
  Serial.print(sec);
  Serial.println(" secondi run");
}

void blinkLed(int flag) {
  if (flag == 1) {
    digitalWrite(LED_BUILTIN, sec % 2 == 0 ? HIGH : LOW);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
}
void blinking() {
  if (blinkingflag) {
    digitalWrite(LED_BUILTIN,  HIGH );
    
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  blinkingflag=!blinkingflag;
}
void checkSensor1() {
  val1 = sensor1.readCM();
  Serial.print(val1);
  Serial.println(" cm");
  //alarm1 = (val1 < min1 || val1 > max1) ? 1 : 0;
}

void checkSensor2() {
  //temperature = dht.readTemperature();
  //val2 = temperature;
  //humidity = dht.readHumidity();
  //char buffer[50];
  //sprintf(buffer, "Temp: %0.1f C | Umi: %0.1f%%", temperature, humidity);
  //Serial.println(buffer);
  //alarm2 = (temperature < min2 || temperature > max2) ? 1 : 0;
}

void getIot() {
  HTTPClient http;
  String macaddress = WiFi.macAddress();
  String requestUrl = String(serverAddress) + "/api/iot?macaddress=" + macaddress;

  Serial.print("Richiesta IoT per MAC ");
  Serial.println(macaddress);
  Serial.print("GET ");
  Serial.println(requestUrl);

  http.begin(requestUrl);
  http.addHeader("X-Device-Key", deviceApiKey);
  int httpResponseCode = http.GET();

  if (httpResponseCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DeserializationError error = deserializeJson(iot, payload);
    if (error) {
      Serial.print("Errore parsing JSON: ");
      Serial.println(error.c_str());
      iot.clear();
    } else {
      Serial.println("JSON IoT ricevuto:");
      serializeJsonPretty(iot, Serial);
      Serial.println();

      setRange();
      setMaxDist();
    }
  } else if (httpResponseCode > 0) {
    Serial.print("Errore HTTP del server: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
  } else {
    Serial.print("Errore nella richiesta HTTP: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

void postData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("POST non eseguita: Wi-Fi non connesso.");
    return;
  }

  if (iot["id"].isNull()) {
    Serial.println("POST non eseguita: record IoT non caricato.");
    return;
  }

  String jsonData = creaJsonData();
  if (jsonData.length() > 120) {
    Serial.println("POST non eseguita: jsondata supera il limite di 120 caratteri.");
    return;
  }

  JsonDocument requestDocument;
  requestDocument["idIot"] = iot["id"].as<int>();
  requestDocument["jsondata"] = jsonData;

  String requestBody;
  serializeJson(requestDocument, requestBody);
  String requestUrl = String(serverAddress) + "/api/data";

  HTTPClient http;
  http.begin(requestUrl);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Device-Key", deviceApiKey);

  Serial.print("POST ");
  Serial.println(requestUrl);
  Serial.print("Body: ");
  Serial.println(requestBody);

  int httpResponseCode = http.POST(requestBody);
  if (httpResponseCode == HTTP_CODE_CREATED) {
    Serial.println("Dato IoT inserito.");
    Serial.println(http.getString());
  } else if (httpResponseCode > 0) {
    Serial.print("Errore HTTP del server: ");
    Serial.println(httpResponseCode);
    Serial.println(http.getString());
  } else {
    Serial.print("Errore nella richiesta POST: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}

// Funzione che accetta i valori e restituisce un char* contenente il JSON
char *creaJsonData() {

  // 1. Alloca un JsonDocument della dimensione adeguata.
  // Per un JSON così piccolo, 256 byte sono più che sufficienti e sicuri.
  JsonDocument doc;

  // jsondata viene inviato come stringa nel body POST.
  JsonArray jsondata = doc.to<JsonArray>();

  // Primo oggetto dell'array (val1 e alarm1)
  JsonObject obj1 = jsondata.add<JsonObject>();
  obj1["val"] = val1;
  obj1["alarm"] = alarm1;

  // Secondo oggetto dell'array (val2 e alarm2)
  JsonObject obj2 = jsondata.add<JsonObject>();
  obj2["val"] = val2;
  obj2["alarm"] = alarm2;
  if (steadyAlarm == 1) {
    JsonObject obj3 = jsondata.add<JsonObject>();
    obj3["type"] = "Tremolio";
    obj3["maxdist"] = maxDistReached;
  }
  // 3. Dichiariamo un array di char statico o globale per contenere la stringa risultante.
  // IMPORTANTE: static o globale significa che la memoria non viene distrutta
  // quando la funzione termina, permettendo al puntatore char* di rimanere valido!
  static char jsonBuffer[256];

  // 4. Serializza l'oggetto JSON dentro il nostro buffer di caratteri
  serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));

  // 5. Restituisce il puntatore al buffer
  return jsonBuffer;
}
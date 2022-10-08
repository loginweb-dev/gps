 #define TINY_GSM_MODEM_SIM7000

#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#ifndef __AVR_ATmega328P__
#define SerialAT Serial1
// or Software Serial on Uno, Nano
#else
#include <SoftwareSerial.h>
SoftwareSerial SerialAT(26, 27);  // RX, TX
#endif

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Your GPRS credentials, if any
const char apn[]      = "4g.entel";

// Server details
const char server[]   = "igps.live";

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4
#define LED_PIN             12


#include <TinyGsmClient.h>
#include <ArduinoJson.h>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm        modem(debugger);
#else
TinyGsm        modem(SerialAT);
#endif

TinyGsmClient  client(modem);

void enableGPS(void)
{
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,1 false ");
    }
    modem.enableGPS();
}

void disableGPS(void)
{
    modem.sendAT("+SGPIO=0,4,1,0");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,0 false ");
    }
    modem.disableGPS();
}

void modemPowerOn()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    //Datasheet Ton mintues = 1S
    digitalWrite(PWR_PIN, HIGH);
}

void modemPowerOff()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);    //Datasheet Ton mintues = 1.2S
    digitalWrite(PWR_PIN, HIGH);
}


void modemRestart()
{
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}

void setup() {
  // Set console baud rate
  SerialMon.begin(115200);
  delay(500);

  SerialAT.begin(9600, SERIAL_8N1, 26, 27);
  modemPowerOn();
  enableGPS();

  Serial.println("========INIT========");
  if (!modem.init()) {
      modemRestart();
      delay(2000);
      Serial.println("Failed to restart modem, attempting to continue without restarting");
      return;
  }
  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem Info: ");
  SerialMon.println(modemInfo);

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork()) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }

  // GPRS connection parameters are usually set after network registration
  SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
  if (!modem.gprsConnect(apn)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
  if (modem.isGprsConnected()) { SerialMon.println("GPRS connected"); }

  set_setting(modem.getIMEI());
  
}
float milat,  milon, mispeed, mialt, miaccuracy;
int   vsat2, usat2;
int mitiempo = 3000; //seg
int geocerca = 3;  //kmph
#define TIEMPO 3000
#define GEOCERCA 3
void loop() {
  while (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
  }
  while (SerialMon.available()) {
      SerialAT.write(SerialMon.read());
  }
    Serial.println("Empezar a posicionar. Asegúrese de ubicar al aire libre.");
    Serial.println("La luz indicadora azul parpadea para indicar el posicionamiento."); 
    enableGPS();
    while (1) {
        if (modem.getGPS(&milat, &milon, &mispeed, &mialt, &vsat2, &usat2, &miaccuracy)) {
            Serial.println("The location has been locked, the latitude and longitude are:");
            Serial.print("latitude:"); Serial.println(milat, 8);
            Serial.print("longitude:"); Serial.println(milon, 8);
            Serial.print("Velicidad:"); Serial.println(mispeed, 8);
            Serial.print("Altura:"); Serial.println(mialt, 8);
            Serial.print("Visible Satellites: "); Serial.println(vsat2+" Used Satellites: "+usat2);
            Serial.print("Senal:"); Serial.println(miaccuracy, 8);
            String gps_raw = modem.getGPSraw();
            Serial.println(gps_raw);
            if(mispeed > geocerca){
              enviar_punto(modem.getIMEI(), milat, milon, mispeed, mialt);
            }
            break;
        }
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
    }
    delay(mitiempo); // 1 seg
}

void set_setting(String imei){
//  StaticJsonDocument<200> doc;
  SerialMon.print("Connecting to ");
  SerialMon.println(server);
  if (!client.connect(server, 80)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
  String miresource = "/api/devices/get?imei="+imei;
  Serial.println(miresource);
  SerialMon.println("Performing HTTP POST request...");
//  client.println(F("POST /api/devices/get?imei=869951034480511 HTTP/1.0"));
//  client.println(F("Host: igps.live"));
//  client.println(F("Connection: close"));
  
//  client.print(String("GET ") + miresource + " HTTP/1.1\r\n");
//  client.print(String("Host: ") + server + "\r\n");
//  client.print("Content-Type application/json\r\n");
//  client.print("Connection: close\r\n\r\n");
//  client.println();
//  Serial.println(client.read());
//  uint32_t timeout = millis();
//  while (client.connected() && millis() - timeout < 10000L) {
//    while (client.available()) {
//      char c = client.read();
//      SerialMon.print(c);
//      timeout = millis();
//    }
//  }
//  SerialMon.println();


  client.println(F("GET /example.json HTTP/1.0"));
  client.println(F("Host: arduinojson.org"));
  client.println(F("Connection: close"));

    if (client.println() == 0) {
      Serial.println(F("Failed to send request"));
//      client.stop();
//      return;
    }
  
// // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status + 9, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
//    client.stop();
//    return;
  }
  Serial.println(strcmp(status + 9, "HTTP/1.1 200 OK"));
//
//
//    // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
//    client.stop();
//    return;
  }
//
  const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
  DynamicJsonDocument doc(capacity);
  Serial.println(capacity);
  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
//    client.stop();
//    return;
  }
  Serial.println(F("Response:"));
  Serial.println(doc["sensor"].as<const char*>());
  Serial.println(doc["time"].as<long>());
  Serial.println(doc["data"][0].as<float>(), 6);
  Serial.println(doc["data"][1].as<float>(), 6);

//  Serial.println(doc);
    
  client.stop();
  SerialMon.println(F("Server disconnected"));
    
}
void enviar_punto(String imei, float latitud, float longitud, float velocidad, float altura){
  SerialMon.print("Connecting to ");
  SerialMon.println(server);
  if (!client.connect(server, 80)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
  String miresource = "/api/waypoints/save?imei="+imei+"&lat="+String(latitud, 8)+"&long="+String(longitud, 8)+"&speed="+velocidad+"&height="+altura;
  Serial.println(miresource);
  SerialMon.println("Performing HTTP POST request...");
  client.print(String("POST ") + miresource + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type application/json\r\n");
  client.print("Connection: close\r\n\r\n");
  client.println();
  uint32_t timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    while (client.available()) {
      char c = client.read();
      SerialMon.print(c);
      timeout = millis();
    }
  }
  SerialMon.println();
  client.stop();
  SerialMon.println(F("Server disconnected"));
}

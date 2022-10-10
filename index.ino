
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

// Your WiFi connection credentials, if applicable
const char wifiSSID[] = "admin";
const char wifiPass[] = "password";

#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

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
#include <WiFi.h>
#include <HTTPClient.h>

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

    Serial.println("/**********************************************************/");
    Serial.println("Preparando para recivir sms");
    Serial.println("/**********************************************************/");
    String response;
    modem.sendAT("ATE0");
    modem.waitResponse(1000L, response);
    Serial.println(response);
    modem.sendAT("+CMGF=1");
    modem.waitResponse(1000L, response);
    Serial.println(response);
    modem.sendAT("+CNMI=1,2,0,0");
    modem.waitResponse(1000L, response);
    Serial.println(response);
    delay(5000);
    
 
  #if TINY_GSM_USE_GPRS
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
     SerialMon.print("Waiting for network...");
      if (!modem.waitForNetwork()) {
        SerialMon.println(" fail");
        delay(10000);
        return;
      }
    set_setting_gprs(modem.getIMEI());
  #endif

  #if TINY_GSM_USE_WIFI
    // Wifi connection parameters must be set before waiting for the network
     Serial.printf("Connecting to %s ", wifiSSID);
     WiFi.begin(wifiSSID, wifiPass);
     while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
     }
     Serial.println(" CONNECTED");
     set_setting_wifi(modem.getIMEI());
  #endif  
  
}

//Variables globales
float milat,  milon, mispeed, mialt, miaccuracy;
int vsat2, usat2;
long mitiempo = 3000; //seg
int geocerca = 3;  //kmph
int velocidad_max = 40;
String num_cliente = "72823861";
String num_gps = "67353115";
String name_cliente = "Ing. Percy Alvarez";
String res = "";
void loop() {
  while (SerialAT.available()) {
    SerialMon.write(SerialAT.read());
     String mymessage = SerialAT.readString();
     String phoneint = mymessage.substring(8, 16);
     res = "";
     mymessage.toUpperCase();
             if(mymessage.indexOf("MAPA")>=0){              
              Serial.println("Empezar a posicionar. Asegúrese de ubicar al aire libre.");
              Serial.println("La luz indicadora azul parpadea para indicar el posicionamiento."); 
              while (1) {
                  if (modem.getGPS(&milat, &milon)) {
                    String mapa = "https://maps.google.com/maps?q=loc:"+String(milat, 8)+","+String(milon, 8);
                    res = modem.sendSMS("+591"+phoneint, mapa);
                     Serial.println("/**********************************************************/");
                    if(res == "1"){
                      Serial.println("Mensaje enviado a "+phoneint);
                    }else{
                         Serial.println("Mensaje NO enviado ERROR a :"+ phoneint);
                    }
                     Serial.println("/**********************************************************/");
                    Serial.println("La ubicación ha sido bloqueada, la latitud y la longitud son:");
                    Serial.print("latitude:"); Serial.println(milat, 8);
                    Serial.print("longitude:"); Serial.println(milon, 8);
                    break;
                  }
                  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                  delay(500);
              }
              digitalWrite(LED_PIN, LOW);
            }else if(mymessage.indexOf("INFO")>=0){
              String imei = modem.getIMEI();
              String ccid = modem.getSimCCID();
              String cop = modem.getOperator();
              res = modem.sendSMS("+591"+phoneint, "Imei: "+imei+"\nOPERADOR: "+cop+"\nPropietario: "+name_cliente+"\nNumero Prop: "+num_cliente+"\nNumero GPS: "+num_gps+"\nVel. Max.: "+velocidad_max);
               Serial.println("/**********************************************************/");
              if(res == "1"){
                Serial.println("Mensaje enviado a "+phoneint);
              }else{
                  Serial.println("Mensaje NO enviado ERROR a :"+ phoneint);
              }
               Serial.println("/**********************************************************/");
            }else if(mymessage.indexOf("GPS")>=0){
              while (1) {
                if (modem.getGPS(&milat, &milon)) {                
                  String gps_raw = modem.getGPSraw();
                  Serial.println(gps_raw);
                  if (gps_raw != "") {
                    res = modem.sendSMS("+591"+phoneint, "DATOS DE GPS: \n"+gps_raw);
                        Serial.println("/**********************************************************/");
                    if(res == "1"){
                      Serial.println("Mensaje enviado a "+phoneint);
                    }else{
                        Serial.println("Mensaje NO enviado ERROR a :"+ phoneint);
                    }
                        Serial.println("/**********************************************************/");
                  }
                  break;
                }
                  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
                  delay(500);
              }
              digitalWrite(LED_PIN, LOW);
            }else if(mymessage.indexOf("LED")>=0){
              digitalWrite(LED_PIN, !digitalRead(LED_PIN));
              Serial.println(digitalRead(LED_PIN));
            }else{
              Serial.println("No envio ninguna palabra clave, intenta con las siguientes palabras:\n1.- Info\n2.- Mapa\n3.- Gps\n4.- Led");
              res = modem.sendSMS("+591"+phoneint, "No envio ninguna palabra clave, intenta con las siguientes palabras:\n1.- Info\n2.- Mapa\n3.- Gps\n4.- Switch, \n5.- Mas");
               Serial.println("/**********************************************************/");
              if(res == "1"){
                Serial.println("Mensaje enviado a "+phoneint);
              }else{
                   Serial.println("Mensaje NO enviado ERROR a :"+ phoneint);
              }
               Serial.println("/**********************************************************/");
               break;
            }
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
                #if TINY_GSM_USE_WIFI
                  
                #endif                  
                #if TINY_GSM_USE_GPRS
                  enviar_punto(modem.getIMEI(), milat, milon, mispeed, mialt);
                #endif  
            }

              if(mispeed > velocidad_max ){
               res = modem.sendSMS("+591"+num_cliente, "Mensaje de alerte, supero la velocidad maxima establecia cual es: "+String(mispeed));
                if(res == "1"){
                  Serial.println("Mensaje enviado a "+num_cliente);
                }else{
                    Serial.println("Mensaje NO enviado ERROR");
                }
              }
            break;
        }
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
    }
    delay(mitiempo); // 3 seg
}

void set_setting_wifi(String imei){
    HTTPClient http;
    http.begin("http://igps.live/api/devices/get?imei="+imei);
    delay(10000);
    int httpResponceCode = http.GET();
    if (httpResponceCode > 0) {
      String response = http.getString();
      Serial.println(httpResponceCode);
      Serial.println(response);
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      mitiempo = doc["delay"];
      geocerca = doc["geocerca"];
      velocidad_max = doc["velocidad_max"];
      Serial.println(mitiempo + geocerca + velocidad_max);
      
    } else {
      Serial.print("err:");
      Serial.println(httpResponceCode);
    }
    http.end(); 
}

void set_setting_gprs(String imei){
  SerialMon.print("Connecting to ");
  SerialMon.println(server);
  if (!client.connect(server, 80)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" success");
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

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// Your GPRS credentials, if any
const char apn[]  = "4g.entel";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <WiFi.h>
#include <TinyGsmClient.h>
//#include <HTTPClient.h>
#include <ArduinoHttpClient.h>   
#include <ArduinoJson.h>
//#include <ArduinoJson.hpp>

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif


#define uS_TO_S_FACTOR      1000000ULL  // Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP       60          // Time ESP32 will go to sleep (in seconds)

#define UART_BAUD           9600
#define PIN_DTR             25
#define PIN_TX              27
#define PIN_RX              26
#define PWR_PIN             4
#define LED_PIN             12

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

String res;
float lat,  lon;
int count = 0;
TinyGsmClient  client(modem);
void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);

    delay(100);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    modemPowerOn();
    enableGPS();
    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);
    
    Serial.println("/**********************************************************/");
    Serial.println("Para inicializar la prueba de red, asegúrese de que su LET ");
    Serial.println("la antena se ha conectado a la interfaz SIM en la placa.");
    Serial.println("/**********************************************************/\n");

     SerialMon.print("Connecting to ");
      SerialMon.print(apn);
      if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        SerialMon.println(F(" [fail]"));
        SerialMon.println(F("************************"));
        SerialMon.println(F(" Is GPRS enabled by network provider?"));
        SerialMon.println(F(" Try checking your card balance."));
        SerialMon.println(F("************************"));
        delay(10000);
        Serial.println(".");
        return;
      }
      SerialMon.println(F(" [OK]"));
  
    delay(5000);
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


    Serial.println("========INIT========");
    if (!modem.init()) {
        modemRestart();
        delay(2000);
        Serial.println("Failed to restart modem, attempting to continue without restarting");
        return;
    }

    Serial.println("Empezar a posicionar. Asegúrese de ubicar al aire libre.");
    Serial.println("La luz indicadora azul parpadea para indicar el posicionamiento."); 
    enableGPS();
    while (1) {
        if (modem.getGPS(&lat, &lon)) {
            Serial.println("The location has been locked, the latitude and longitude are:");
            Serial.print("latitude:"); Serial.println(lat);
            Serial.print("longitude:"); Serial.println(lon);
            break;
        }
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        delay(500);
    }
    digitalWrite(LED_PIN, LOW);
    
    Serial.println("/**********************************************************/");
    Serial.println("After the network test is complete, please enter the  ");
    Serial.println("AT command in the serial terminal.");
    Serial.println("/**********************************************************/");
}

void loop()
{
        while (SerialAT.available()) {
            SerialMon.write(SerialAT.read());
            String mymessage = SerialAT.readString();
            res = "";
            delay(500);
            String phoneint = mymessage.substring(8, 16);
            mymessage.toUpperCase();
            Serial.println(mymessage);
            if(mymessage.indexOf("MAPA")>=0){              
              Serial.println("Empezar a posicionar. Asegúrese de ubicar al aire libre.");
              Serial.println("La luz indicadora azul parpadea para indicar el posicionamiento."); 
              while (1) {
                  if (modem.getGPS(&lat, &lon)) {
                    String mapa = "https://maps.google.com/maps?q=loc:"+String(lat)+","+String(lon);
                    res = modem.sendSMS("+591"+phoneint, mapa);
                    if(res == "1"){
                      Serial.println("Mensaje enviado a "+phoneint);
                    }else{
                        Serial.println("Mensaje NO enviado ERROR");
                    }
                    Serial.println("La ubicación ha sido bloqueada, la latitud y la longitud son:");
                    Serial.print("latitude:"); Serial.println(lat);
                    Serial.print("longitude:"); Serial.println(lon);
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
              res = modem.sendSMS("+591"+phoneint, "Datos de Dispocitivo \nImei: "+imei+"\nCCID: "+ccid+"\nOPERADOR: "+cop+"\nPropietario: Ing. Percy Alvarez \nNumero: +591 67353115");
              if(res == "1"){
                Serial.println("Mensaje enviado a "+phoneint);
              }else{
                  Serial.println("Mensaje NO enviado ERROR");
              }
            }else if(mymessage.indexOf("GPS")>=0){
              while (1) {
                if (modem.getGPS(&lat, &lon)) {                
                  String gps_raw = modem.getGPSraw();
                  Serial.println(gps_raw);
                  if (gps_raw != "") {
                    res = modem.sendSMS("+591"+phoneint, "DATOS DE GPS: \n"+gps_raw);
                    if(res == "1"){
                      Serial.println("Mensaje enviado a "+phoneint);
                    }else{
                        Serial.println("Mensaje NO enviado ERROR");
                    }
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
              res = modem.sendSMS("+591"+phoneint, "No envio ninguna palabra clave, intenta con las siguientes palabras:\n1.- Info\n2.- Mapa\n3.- Gps\n4.- Led");
              if(res == "1"){
                Serial.println("Mensaje enviado a "+phoneint);
              }else{
                  Serial.println("Mensaje NO enviado ERROR");
              }
            }
        }
        while (SerialMon.available()) {
            SerialAT.write(SerialMon.read());
        }

  //enivar points al servidor
   if (modem.getGPS(&lat, &lon)) {
      Serial.println("tu latitud y la longitud:");
      Serial.print("latitude: "); Serial.println(lat);
      Serial.print("longitude: "); Serial.println(lon);
      Serial.print("count: "); Serial.println(count);
//      httPost("65465465847", lat, lon);
    }
    delay(9000);
    count +=1;
}


void httPost(String imei, float latitud, float longitud) {
  DynamicJsonDocument doc(512);
  doc["imei"] = imei;
  doc["latitud"] = latitud;
  doc["longitud"] = longitud;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);

  HttpClient WeatherAPI = HttpClient(client, "igps.loginweb.dev/api/waypoints/store", 80);
//  WeatherAPI.get("/api/waypoints/store?imei=" + imei + "&latitud=" + latitud + "&longitud=" + longitud);
//  int len = measureJson(jsonBuffer);
//    WeatherAPI.addHeader("Content-Type", "application/json");
//  WeatherAPI.connectionKeepAlive(); 
    WeatherAPI.post("/", "application/json", jsonBuffer);
  // read the status code and body of the response
  int statusCode = WeatherAPI.responseStatusCode();
  String response = WeatherAPI.responseBody();
  Serial.println("-------------------------------------");
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
   Serial.println("-------------------------------------");
  
//  HTTPClient httPost = HttpClient();
//  httPost.begin("https://igps.loginweb.dev/api/waypoints/store");
//  httPost.addHeader("Content-Type", "application/json");
//  int httpResponceCode = httPost.POST(jsonBuffer);
//  if (httpResponceCode > 0) {
//    String response = httPost.getString();
//    Serial.println(httpResponceCode);
//    Serial.println(response);
//  } else {
//    Serial.print("err:");
//    Serial.println(httpResponceCode);
//  }
//  httPost.end();
}
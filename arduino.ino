// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial

// Set serial for AT commands (to the module)
// Use Hardware Serial on Mega, Leonardo, Micro
#define SerialAT Serial1

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define SerialAT Serial1

// See all AT commands, if wanted
// #define DUMP_AT_COMMANDS

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "4g.entel";     //SET TO YOUR APN
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <SPI.h>
#include <SD.h>
#include <Ticker.h>

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

#define SD_MISO             2
#define SD_MOSI             15
#define SD_SCLK             14
#define SD_CS               13
#define LED_PIN             12


//#define SMS_TARGET  "+59172823861"

void enableGPS(void)
{
    // Set SIM7000G GPIO4 LOW ,turn on GPS power
    // CMD:AT+SGPIO=0,4,1,1
    // Only in version 20200415 is there a function to control GPS power
    modem.sendAT("+SGPIO=0,4,1,1");
    if (modem.waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,1 false ");
    }
    modem.enableGPS();
}

void disableGPS(void)
{
    // Set SIM7000G GPIO4 LOW ,turn off GPS power
    // CMD:AT+SGPIO=0,4,1,0
    // Only in version 20200415 is there a function to control GPS power
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

void setup()
{
    // Set console baud rate
    SerialMon.begin(115200);

    delay(10);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    modemPowerOn();
    enableGPS();
    Serial.println("========SDCard Detect.======");
    SPI.begin(SD_SCLK, SD_MISO, SD_MOSI);
    if (!SD.begin(SD_CS)) {
        Serial.println("SDCard MOUNT FAIL");
    } else {
        uint32_t cardSize = SD.cardSize() / (1024 * 1024);
        String str = "SDCard Size: " + String(cardSize) + "MB";
        Serial.println(str);
    }
    Serial.println("===========================");

    SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);


//    Serial.println("/**********************************************************/");
//    Serial.println("To initialize the network test, please make sure your LET ");
//    Serial.println("antenna has been connected to the SIM interface on the board.");
//    Serial.println("/**********************************************************/\n\n");

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
}

String msg = "";
String smsStatus = "";
String senderNumber = "";
String receivedDate = "";
void loop()
{
    String res;

    Serial.println("========INIT========");

    if (!modem.init()) {
        modemRestart();
        delay(2000);
        Serial.println("Failed to restart modem, attempting to continue without restarting");
        return;
    }

    Serial.println("========SIMCOMATI======");
    modem.sendAT("+SIMCOMATI");
    modem.waitResponse(1000L, res);
    res.replace(GSM_NL "OK" GSM_NL, "");
    Serial.println(res);
    res = "";
    Serial.println("=======================");

    Serial.println("=====Preferred mode selection=====");
    modem.sendAT("+CNMP?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);
    }
    res = "";
    Serial.println("=======================");


    Serial.println("=====Preferred selection between CAT-M and NB-IoT=====");
    modem.sendAT("+CMNB?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);
    }
    res = "";
    Serial.println("=======================");


    String name = modem.getModemName();
    Serial.println("Modem Name: " + name);

    String modemInfo = modem.getModemInfo();
    Serial.println("Modem Info: " + modemInfo);

    // Unlock your SIM card with a PIN if needed
    if ( GSM_PIN && modem.getSimStatus() != 3 ) {
        modem.simUnlock(GSM_PIN);
    }


    for (int i = 0; i <= 4; i++) {
        uint8_t network[] = {
            2,  /*Automatic*/
            13, /*GSM only*/
            38, /*LTE only*/
            51  /*GSM and LTE only*/
        };
        Serial.printf("Try %d method\n", network[i]);
        modem.setNetworkMode(network[i]);
        delay(3000);
        bool isConnected = false;
        int tryCount = 60;
        while (tryCount--) {
            int16_t signal =  modem.getSignalQuality();
            Serial.print("Signal: ");
            Serial.print(signal);
            Serial.print(" ");
            Serial.print("isNetworkConnected: ");
            isConnected = modem.isNetworkConnected();
            Serial.println( isConnected ? "CONNECT" : "NO CONNECT");
            if (isConnected) {
                break;
            }
            delay(1000);
            digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        }
        if (isConnected) {
            break;
        }
    }
    digitalWrite(LED_PIN, HIGH);

    Serial.println();
    Serial.println("Device is connected .");
    Serial.println();

    Serial.println("=====Inquiring UE system information=====");
    modem.sendAT("+CPSI?");
    if (modem.waitResponse(1000L, res) == 1) {
        res.replace(GSM_NL "OK" GSM_NL, "");
        Serial.println(res);
    }


    Serial.println("Start positioning . Make sure to locate outdoors.");
    Serial.println("The blue indicator light flashes to indicate positioning."); 
    enableGPS();
    float lat,  lon;
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
    disableGPS();

    Serial.println("/**********************************************************/");
    Serial.println("After the network test is complete, please enter the  ");
    Serial.println("AT command in the serial terminal.");
    Serial.println("/**********************************************************/");

    while (1) {
        while (SerialAT.available()) {
            SerialMon.write(SerialAT.read());
            String mymessage = SerialAT.readString();
            res = "";
            delay(500);
            String phoneint = mymessage.substring(8, 16);
            if(mymessage.indexOf("MAPA")>=0){              
              Serial.println("Empezar a posicionar. Asegúrese de ubicar al aire libre.");
              Serial.println("La luz indicadora azul parpadea para indicar el posicionamiento."); 
              enableGPS();
              while (1) {
                  if (modem.getGPS(&lat, &lon)) {
                    String mapa = "https://maps.google.com/maps?q=loc:"+String(lat)+","+String(lon);
                    res = modem.sendSMS("+591"+phoneint, mapa);
                    if(res == "1"){
                      Serial.println("Mensaje enviado "+phoneint);
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
              disableGPS();
            }else if(mymessage.indexOf("INFO")>=0){
              String imei = modem.getIMEI();
              String ccid = modem.getSimCCID();
              String cop = modem.getOperator();
              res = modem.sendSMS("+591"+phoneint, "Datos de GPS \nImei: "+imei+"\nCCID: "+ccid+"\nOPERADOR: "+cop+"\nPropietario: Ing. Percy Alvarez \nNumero: +591 67353115");
              if(res == "1"){
                Serial.println("Mensaje enviado: "+phoneint);
              }else{
                  Serial.println("Mensaje NO enviado ERROR");
              }
            }else{
               Serial.println("No envio ninguna palabra clave");
            }
        }
        while (SerialMon.available()) {
            SerialAT.write(SerialMon.read());
        }
    }
}

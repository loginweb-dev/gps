#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
//#define HOST "http://4154d15d.ngrok.io"

const char* ssid       = "admin";
const char* password   = "password";

//StaticJsonDocument<200> doc;
//StaticJsonDocument<200> postJson;
void setup()
{

  Serial.begin(115200);
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
//  httpGet();
  httPost();
}

void loop(){
}

void httpGet() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin("https://igps.loginweb.dev/api/waypoints");
    int httpResponceCode = http.GET();
    if (httpResponceCode > 0) {
      String response = http.getString();
      Serial.println(httpResponceCode);
      Serial.println(response);
    } else {
      Serial.print("err:");
      Serial.println(httpResponceCode);
    }
    http.end();
  } else {
    Serial.println("wifi err");
  }
}

void httPost() {
  HTTPClient httPost;
  httPost.begin("https://igps.loginweb.dev/api/waypoints/store");
  httPost.addHeader("Content-Type", "application/json");
  int httpResponceCode = httPost.POST("{\n\"imei\":\"684684684\", \n\"latitud\":\"-14.54545\", \n\"longitud\":\"-64.54684\"}");
  if (httpResponceCode > 0) {
    String response = httPost.getString();
    Serial.println(httpResponceCode);
    Serial.println(response);
  } else {
    Serial.print("err:");
    Serial.println(httpResponceCode);
  }
  httPost.end();

}
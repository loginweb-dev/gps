#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid       = "admin";
const char* password   = "password";

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
  httPost("asdfasdfas", -14.65654684, -64.654454);
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

void httPost(String imei, float latitud, float longitud) {
  DynamicJsonDocument doc(512);
  doc["imei"] = imei;
  doc["latitud"] = latitud;
  doc["longitud"] = longitud;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);
  Serial.println(jsonBuffer);
  HTTPClient httPost;
  httPost.begin("https://igps.loginweb.dev/api/waypoints/store");
  httPost.addHeader("Content-Type", "application/json");
  int httpResponceCode = httPost.POST(jsonBuffer);
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
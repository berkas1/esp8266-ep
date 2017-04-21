#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClientSecure.h>
#include "DHT.h"



#define SENSORTYPE DHT22
#define PRINT_DEBUG_MESSAGES true

// ThingSpeak configuration
#define ENABLE_THINGSPEAK false
#if ENABLE_THINGSPEAK
  const char* _thingspeak_id = "";
  const char* _thingspeak_apikey = "";
#endif

// Enter your WIFI network details
const char* ssid = "";
const char* password = "";

// interval for sending data to cloud service. Default = 20 seconds
const long sendInterval = 20000;


//////////////////////////////
// DO NOT EDIT BELOW
//////////////////////////////

typedef struct {
  float temperature_c;
  float temperature_f;
  float humidity;
} data;
data tempData;

// Web Server on port 80
ESP8266WebServer server(80);

// DHT Sensor
const int DHTPin = D2;

// Initialize DHT sensor.
DHT dht(DHTPin, SENSORTYPE);
unsigned long previousMillis = 0; 


void requestTemperature() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  tempData.humidity = h;
  tempData.temperature_c = t;
}

#if ENABLE_THINGSPEAK

void sendDataThingspeak() {
  const char* host="api.thingspeak.com";

  WiFiClientSecure client;
  String data = String("field1=") + tempData.temperature_c + 
      "&field2=" + tempData.humidity + "&api_key=" + String(_thingspeak_apikey);
  Serial.println(data);

  if (!client.connect(host, 443)) {
    Serial.println("Error: connection failed.");
    return;
  }
  Serial.println("sfdf");

  String http = String("POST ") + "/update HTTP/1.1\r\n" +
      "Host: " + host + "\r\n" +
      "Connection: close\r\n" +
      "Content-Type: application/x-www-form-urlencoded\r\n" +
      "Content-Length: " + data.length() + "\r\n\r\n" + data +
      "\r\n"; 
  client.print(http);


  int timeout_at = millis() + 2000;
  while (!client.available() && timeout_at - millis() < 0) {
    client.stop();
    return;
  }
  
  String line;
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }
  line = client.readStringUntil('\n');

  #if PRINT_DEBUG_MESSAGES
    Serial.println("Message to ThingSpeak sent.");
  #endif

}
#endif



void setup() {
  Serial.begin(115200);
  delay(100);

  dht.begin();

  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");


  // define webserver
  server.on("/plaint", [](){ 
      requestTemperature();
      delay(10);
      server.send(200, "text/plain", String(tempData.temperature_c));
      #ifdef PRINT_DEBUG_MESSAGES
        Serial.println(String("webserver request /plaint, value:") + tempData.temperature_c);
      #endif
  });

  server.on("/plainh", [](){ 
      requestTemperature();
      delay(10);
      server.send(200, "text/plain", String(tempData.humidity));
      #ifdef PRINT_DEBUG_MESSAGES
        Serial.println(String("webserver request /plainh, value:") + tempData.humidity);
      #endif
  });

  server.on("/json", [](){ 
      requestTemperature();
      delay(10);
      String json = String("{\"temp_c\": ") + String(tempData.temperature_c) +
        + ", \"humidity\": " + String(tempData.humidity) + String("}");
      server.send(200, "text/plain", json);
      #ifdef PRINT_DEBUG_MESSAGES
        Serial.println(String("webserver request /json"));
      #endif
  });

  server.on("/xml", [](){ 
    
      requestTemperature();
      delay(10);
      String json = String("{ \"temp_c\": ") + String(tempData.temperature_c) +
        + ", \"humidity\": " + String(tempData.humidity) + String("}");

      String xml = String("<data>") + "<temp_c>" + String(tempData.temperature_c) +
        "</temp_c>" + "<humidity>" + String(tempData.humidity) + "</humidity>" +
        "</data>";
      server.send(200, "text/plain", xml);
      #ifdef PRINT_DEBUG_MESSAGES
        Serial.println(String("webserver request /xml"));
      #endif
  });

  server.on("/html", [](){ 
      requestTemperature();
      delay(10);
      String data = String("<html><body><h2>Temperature: ") + tempData.temperature_c
          + " &#8451;<p>Humidity: " + tempData.humidity  + "</h2></body></html>";
      server.send(200, "text/html", data);
      #ifdef PRINT_DEBUG_MESSAGES
        Serial.println(String("webserver request /html"));
      #endif
  });

  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP:");
  delay(3000);

  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
}


void loop() {
  server.handleClient();
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= sendInterval) {
    previousMillis = currentMillis;
    requestTemperature();
    #if ENABLE_THINGSPEAK
      sendDataThingspeak();
    #endif
  }
}
#include "main.h"
#include <SparkFunCCS811.h>
#include <DHT.h>
#include <WiFi.h>
#include <CircularBuffer.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

#define ARR_SIZE 1000
#define DEVICE_MODE 1 // 0 - values from sensors, 1 - random values generation

DHT dht(DHT_PIN, DHT_TYPE);
CCS811 ccs(CCS811_ADDR);

CircularBuffer<float,ARR_SIZE> temp_arr;   // uses 988 bytes
CircularBuffer<float,ARR_SIZE> humidity_arr;   // uses 988 bytes
CircularBuffer<uint16_t,ARR_SIZE> co2_arr;   // uses 988 bytes
CircularBuffer<int,ARR_SIZE> time_arr;   // uses 988 bytes


IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);

AsyncWebServer server(80);

// Create an Event Source on /events
AsyncEventSource events("/events");

unsigned long delayStart = 0;

static uint32_t arr_cnt;

String read_sensors()
{
  bool result;

  #if DEVICE_MODE == 1
  result = temp_arr.push(random(20, 30));
  result = humidity_arr.push(random(100, 200));
  result = co2_arr.push(random(100, 5000));

  #else

  float temp = dht.readTemperature();
  if (isnan(temp)) {    
    Serial.println("Failed to read from BME280 sensor!");
  }
  else {
    result = temp_arr.push(temp);
    Serial.println(result);
    Serial.println("TEMPERATURE: ");
    Serial.println(temp);
  }

    float hum = dht.readHumidity();
  if (isnan(hum)) {
    Serial.println("Failed to read from BME280 sensor!");
  }
  else {
    result = humidity_arr.push(hum);
    Serial.println(result);
    Serial.println("HUMIDITY: ");
    Serial.println(hum);
  }

 if (ccs.dataAvailable())
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
    ccs.readAlgorithmResults();
    float co2 = ccs.getCO2();
    result = co2_arr.push(co2);
    Serial.println("CO2: ");
    Serial.print(co2);
    //Returns calculated CO2 reading
    Serial.println(result);
    Serial.println("tVOC");
    //Returns calculated TVOC reading
    Serial.print(ccs.getTVOC());
  }
  #endif
  return "OK";


}

// Get ALL Sensor Readings and return JSON object
String JSON_AllSensorReadings(){
  DynamicJsonDocument doc(30000);

  JsonArray temperature = doc.createNestedArray("temperature");
  JsonArray co2 = doc.createNestedArray("co2");
  JsonArray humidity = doc.createNestedArray("humidity");

  read_sensors();
  read_sensors();
  read_sensors();
  read_sensors();

  for(int i = 0; i < temp_arr.size(); i++)
  {
    temperature.add(temp_arr[i]);
  }


  for(int i = 0; i < humidity_arr.size(); i++)
  {
    humidity.add(humidity_arr[i]);
  }


  for(int i = 0; i < co2_arr.size(); i++)
  {
    co2.add(co2_arr[i]);
  }


  // serializeJsonPretty(doc, Serial);

  String jsonString; 
  serializeJson(doc, jsonString);     
  serializeJsonPretty(doc, Serial);     

  return jsonString;
}

// Get Last Sensor Readings and return JSON object
String JSON_SensorReadings(){
  DynamicJsonDocument doc(500);

  JsonArray temperature = doc.createNestedArray("temperature");
  JsonArray co2 = doc.createNestedArray("co2");
  JsonArray humidity = doc.createNestedArray("humidity");

  temperature.add(temp_arr.last());
  humidity.add(humidity_arr.last());
  co2.add(temp_arr.last());

  String jsonString;
  serializeJson(doc, jsonString);     

  return jsonString;
}


void setup() {
  /*Init serial*/
  Serial.begin(115200);

  /*Init gpio*/
  pinMode(BUTTON_PIN, INPUT);

  /*Init DHT22*/
  dht.begin();

  /*Init CCS*/
  Wire.begin();
  if(!ccs.begin())
  {
    Serial.println("Failed to start sensor!");
    while(1);
  }
  
  /*Init SPIFFS*/
  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  /*Init Access Point*/
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(ssid, password);
  JSON_AllSensorReadings();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html");
  });
  // Sending highcharts library
  server.on("/highcharts.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/highcharts.js");
  });
  // Sending highcharts library
  server.on("/highcharts.js.map", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/highcharts.js.map");
  });
    // Request for all sensor readings
  server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = JSON_AllSensorReadings();
    request->send(200, "application/json", json);
    json = String();
  });

  server.serveStatic("/", SPIFFS, "/");

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("hello!", NULL, millis(), 10000);
  });
  delay(100);
  server.addHandler(&events);

  server.begin();
  delayStart = millis();

}
void loop() {
  /*Compute data from the sensors*/
  if((millis() - delayStart) > 5000)
  {
      // Serial.println("HELLO 4");

    read_sensors();
    events.send("ping",NULL,millis());
    events.send(JSON_SensorReadings().c_str(), "new_readings", millis());

    delayStart = millis();
    // delay(1000);
  }
}





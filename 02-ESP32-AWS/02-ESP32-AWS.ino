#include "secrets.h"
#include <WiFiClientSecure.h>
#include "WiFi.h"
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <TaskScheduler.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);

#define LED_PIN 5
#define LED_PIN1 19
#define DHTPIN 4
#define DHTTYPE DHT11


DHT dht(DHTPIN, DHTTYPE);

const int PushButton = 15;

void connectAWS()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.println("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // Create a message handler
  client.onMessage(messageHandler);

  Serial.print("Connecting to AWS IOT");

  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }

  // Subscribe to a topic
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);

  Serial.println("AWS IoT Connected!");
}

// We declare the function that we are going to use
void read_dht();
void read_button();

// We create the Scheduler that will be in charge of managing the tasks
Scheduler runner;

// We create the task indicating that it runs every 500 milliseconds, forever, and call the led_blink function
Task readDHT(2000, TASK_FOREVER, &read_dht);
Task readbutton(2000, TASK_FOREVER, &read_button);

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["time"] = millis();
  doc["sensor_a0"] = analogRead(0);
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void messageHandler(String &topic, String &payload) {
  
    Serial.println("-------new message from broker-----");
    Serial.print("channel:");
    Serial.println(topic);
    Serial.print("data:");  
    Serial.println(payload);
    Serial.println();


    //contol LED
    if(payload == "1"){      
      digitalWrite(LED_PIN,1);
      Serial.print("Turning on LED RED");
    }
    else if(payload == "0"){ 
      digitalWrite(LED_PIN,0);
      Serial.print("Turning off LED RED");
      
    }else if(payload == "3"){
      digitalWrite(LED_PIN1,1);
      Serial.print("Turning on LED GREEN");
      
    }else if(payload == "2"){
      digitalWrite(LED_PIN1,0);
      Serial.print("Turning off LED GREEN");
    }
    
    Serial.println();
}

void setup() {
  Serial.begin(115200);
  connectAWS();

   Wire.begin();

   lightMeter.begin();

  pinMode (LED_PIN, OUTPUT);
  pinMode (LED_PIN1, OUTPUT);

  dht.begin();

  // We add the task to the task scheduler
  runner.addTask(readDHT);
  runner.addTask(readbutton);
  
  // We activate the task
  readDHT.enable();
  readbutton.enable();
}

void loop() {
  client.loop();
  runner.execute();
  delay(10);


}

void read_button(){

  int Push_button_state = digitalRead(PushButton);

  StaticJsonDocument<200> doc;
  doc["device_id"] = THINGNAME;
  
  if ( Push_button_state == HIGH ){ 
  
  doc["button"] = "1";
  
  
  }
  else {

  doc["button"] = "0";
  
  }

  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC_BUTTON, jsonBuffer);
}

void read_dht(){
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  float lux = lightMeter.readLightLevel();
  

   StaticJsonDocument<200> doc;
   doc["device_id"] = THINGNAME;

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
   
    doc["temperature"] = "0";
    doc["humidity"] = "0";
   
  }
  else{
   
    doc["temperature"] = String(t);
    doc["humidity"] = String(h);
    doc["light"] = String(lux);
    
  }
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

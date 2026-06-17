/*
    This sketch shows the Ethernet event usage

*/

// Important to be defined BEFORE including ETH.h for ETH.begin() to work.
// Example RMII LAN8720 (Olimex, etc.)
#include <Arduino.h>
#ifndef ETH_PHY_MDC
#define ETH_PHY_TYPE ETH_PHY_LAN8720
#if CONFIG_IDF_TARGET_ESP32
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER -1
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#elif CONFIG_IDF_TARGET_ESP32P4
#define ETH_PHY_ADDR  0
#define ETH_PHY_MDC   31
#define ETH_PHY_MDIO  52
#define ETH_PHY_POWER 51
#define ETH_CLK_MODE  EMAC_CLK_EXT_IN
#endif
#endif

#include <ETH.h>
#include <ArduinoMqttClient.h>



NetworkClient ethClient;
static bool eth_connected = false;

MqttClient mqttClient(ethClient);
static bool mqttBroker_connected = false;

const char broker[] = "192.168.0.39";
int        port     = 1883;
const char infoTopic[]  = "thermalControl/WT32ETH01Sensors/nSensors";
String baseTopic  = "thermalControl/WT32ETH01Sensors/";
String nSensorsTopic  = "nSensors";
// const char nSensorsTopic[]  = "BO_PufferOben";

const long connectInterval = 1000;
const long reportingInterval = 10000;


int count = 0;

#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: 
    Serial.println("ETH Connected"); 
    break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      Serial.println("attempting to connect to Broker");
      if (!mqttClient.connect(broker, port)) {
          Serial.print("MQTT connection failed! Error code = ");
          Serial.println(mqttClient.connectError()); 
          delay(connectInterval);
      }
      mqttBroker_connected = true;
  

      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      mqttBroker_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      mqttBroker_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      mqttBroker_connected = false;
      break;
    default: break;
  }
}


void setup() {
  Serial.begin(115200);
  Network.onEvent(onEvent);
  ETH.begin();
  mqttClient.connect(broker, port);
  sensors.begin();
}

void loop() {
  Serial.println("attepting to send message");
  if (mqttBroker_connected) {
    Serial.println("sending message...");
    // testClient("google.com", 80);
    
    int nDevices = sensors.getDeviceCount();
    
    mqttClient.poll();
    mqttClient.beginMessage(baseTopic+nSensorsTopic);
    mqttClient.print(nDevices);
    mqttClient.endMessage();
    sensors.requestTemperatures(); 
    for(int i=0;i<nDevices;i++){
      
      Serial.println("Sensor"+String(i));
      float temperatureC = sensors.getTempCByIndex(i);
      
      if(temperatureC > -30){
        mqttClient.beginMessage(baseTopic+"Sensor"+i);
        mqttClient.print(temperatureC);
        mqttClient.endMessage();
      }
      
    }
    

    Serial.println("message sent");
  }else{
    Serial.println("not connected to MQTT broker");
  }
  delay(reportingInterval);
}

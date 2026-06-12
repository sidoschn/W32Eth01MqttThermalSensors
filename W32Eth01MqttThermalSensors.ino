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

static bool eth_connected = false;

NetworkClient ethClient;

MqttClient mqttClient(ethClient);

const char broker[] = "192.168.0.39";
int        port     = 1883;
const char topic[]  = "thermalControl/WT32ETH01Sensors";

const long interval = 1000;
unsigned long previousMillis = 0;

int count = 0;


// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Connected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      Serial.println("attempting to connect to Broker");
      if (!mqttClient.connect(broker, port)) {
          Serial.print("MQTT connection failed! Error code = ");
          Serial.println(mqttClient.connectError()); 
          delay(1000);

      while (1);
  }

      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default: break;
  }
}

// void testClient(const char *host, uint16_t port) {
//   Serial.print("\nconnecting to ");
//   Serial.println(host);

//   NetworkClient client;
//   if (!client.connect(host, port)) {
//     Serial.println("connection failed");
//     return;
//   }
//   client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
//   while (client.connected() && !client.available());
//   while (client.available()) {
//     Serial.write(client.read());
//   }

//   Serial.println("closing connection\n");
//   client.stop();
// }

void setup() {
  Serial.begin(115200);
  Network.onEvent(onEvent);
  ETH.begin();
  mqttClient.connect(broker, port);
}

void loop() {
  Serial.println("attepting to send message");
  if (eth_connected) {
    Serial.println("sending message...");
    // testClient("google.com", 80);
    mqttClient.poll();
    mqttClient.beginMessage(topic);
    mqttClient.print("hello from ESP32");
    mqttClient.endMessage();
    Serial.println("message sent");
  }
  delay(5000);
}

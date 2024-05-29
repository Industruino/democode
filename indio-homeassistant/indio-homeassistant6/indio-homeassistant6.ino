/*
  Industruino INDIO HomeAssistant (HASS) generic sketch
  using communication module:
  wifi or ethernet (or gsm - not implemented yet)

  connects to HASS over MQTT https://www.home-assistant.io/integrations/mqtt/
  discovered by HASS using MQTT Discovery https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  identified by its 4-byte indio_mac
  has an availability topic for online/offline status

  4 digital inputs (with pulse counters):   DIG CH1-4
  4 digital outputs:                        DIG CH5-8
  4 analog inputs (0-10V):                  AIN CH1-4
  2 analog outputs (0-10V):                 AOUT CH1-2

  SETUP
  initialises I/O channels
  connects to network via Eth or Wifi
  connects to home assistant's MQTT server
  subscribes to command topics of digital and analog outputs, and digital input pulse counters
  publishes initial states, values, counters

  LOOP
  handles MQTT callbacks with set commands for digital and analog outputs
  reads digital channels (1-8) and publishes state if changed
  reads analog channels (1-4) and publishes value if changed (at certain interval)

  CONFIGURATION in HOME ASSISTANT by MQTT DISCOVERY (retained):
  during normal operation, press UP button, then DOWN button, to publish the configuration
    digital input X: "binary_sensor" (channels 1-4) for binary state
      name:                 indio_mac_dX
      unique_id:            indio_mac_dX
      availability_topic:   homeassistant/indio_mac/availability
      state_topic:          homeassistant/binary_sensor/indio_mac_dX/state
    digital input X: "number" (channels 1-4) for pulse counting
      name:                 indio_mac_counter_dX
      unique_id:            indio_mac_counter_dX
      availability_topic:   homeassistant/indio_mac/availability
      command_topic:        homeassistant/number/indio_mac_counter_dX/set
//      retain:               true  // do not retain, otherwise always receiving latest set value on startup, should use FRAM stored counter
      max:                  10000000
      mode:                 box
      state_topic:          homeassistant/number/indio_mac_counter_dX/value
    digital output X: "switch" (channels 5-8)
      name:                 indio_mac_dX
      unique_id:            indio_mac_dX
      availability_topic:   homeassistant/indio_mac/availability
      command_topic:        homeassistant/switch/indio_mac_dX/set
      retain:               true
      state_topic:          homeassistant/switch/indio_mac_dX/state
    analog input X: "sensor" (channels 1-4)
      name:                 indio_mac_aiX
      unique_id:            indio_mac_aiX
      availability_topic:   homeassistant/indio_mac/availability
      unit_of_measurement:  %
      state_topic:          homeassistant/sensor/indio_mac_aiX/value
    analog output X: "number" (channels 1-2)
      name:                 indio_mac_aoX
      unique_id:            indio_mac_aoX
      availability_topic:   homeassistant/indio_mac/availability
      command_topic:        homeassistant/number/indio_mac_aoX/set
      retain:               true
      unit_of_measurement:  %
      state_topic:          homeassistant/number/indio_mac_aoX/value
  

  Tom Tobback, Jan 2023

  *******************************************************************************************************************************************************
    MIT LICENSE
    Copyright (c) 2019-2023 Cassiopeia Ltd
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
    IN THE SOFTWARE.
 *******************************************************************************************************************************************************
*/

////// USER CHANGE AS NEEDED ////////////////////////////////////////////////////////////////////////////////////////////
#define COMM_MODULE 1                              // 0: wifi, 1: eth, 2: gsm
#define USE_SSL 0                                  // 0 no SSL, 1 for SSL -- default 0 for local Home Assistant installation
const char mqtt_server[] = "homeassistant.local";  // default Home Assistant broker
const char mqtt_user[] = "indio_mqtt";             // any user in Home Assistant
const char mqtt_pwd[] = "indio_pwd";               // user pwd in Home Assistant
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// other constants
const int ANALOG_READ_INTERVAL_SEC = 1;        // frequency of analog read/publish if changed
const int PULSE_COUNTER_PUB_INTERVAL_SEC = 5;  // frequency of pulse counters publish and save to FRAM
const int MQTT_RECONNECT_INTERVAL_SEC = 10;    // interval between MQTT reconnect attempts

// state variables
bool dig_ch_prev_state[9] = { 0 };              // to trigger a publish
float ana_in_ch_prev_value[5] = { 0 };          // to trigger a publish
float ana_out_ch_current_value[3] = { 0 };      // to display
unsigned long dig_in_pulse_counter[5] = { 0 };  // pulse counters
unsigned long analog_read_ts;
unsigned long pulse_counter_pub_ts;
unsigned long last_mqtt_attempt_ts;
unsigned long last_display_ts;

// helper tabs
#include "indio-general.h"
#include "indio-wifi.h"
#include "indio-eth.h"
#include "indio-gsm.h"

// MQTT
// client is defined in eth, gsm, wifi tabs
#include <PubSubClient.h>
#if COMM_MODULE == 0
PubSubClient mqtt_client(wifi_client);
#elif COMM_MODULE == 1
PubSubClient mqtt_client(eth_client);
#endif
// use standard MQTT ports
#if USE_SSL == 1
const int mqtt_port = 8883;
#else
const int mqtt_port = 1883;
#endif

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void setup() {

  // start LCD
  pinMode(LCD_BACKLIGHT, OUTPUT);
  digitalWrite(LCD_BACKLIGHT, HIGH);  // backlight on Industruino LCD
  lcd.begin();
  displayIntro();

  SerialUSB.begin(115200);
  delay(2000);  // after upload the serial is not immediately available, pause a second
  SerialUSB.println();
  SerialUSB.println("sketch details:");
  SerialUSB.print(FILENAME);
  SerialUSB.println(" uploaded on " __DATE__ " " __TIME__);
  SerialUSB.println(__FILE__);
  SerialUSB.println("===============================");
  SerialUSB.println("Industruino Home Assistant test");
  SerialUSB.println("===============================");

  // start Industruino SD, FRAM, WDT, MAC, IO
  initSD_FRAM_WDT_MAC();  // start watchdog timer, get MAC from EEPROM, get counters from FRAM
  configIO();             // default config of I/O channels

  // join the network
  if (COMM_MODULE == 0) initWifi();
  if (COMM_MODULE == 1) initEthernet();
  myWDT.clear();

  // MQTT settings and initial connect
  mqtt_client.setServer(mqtt_server, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  //mqtt_client.setKeepAlive(60);  // default 15
  //mqtt_client.setSocketTimeout(60);  // default 15
  mqtt_client.setBufferSize(1024);  // default 256, was 512
  //wifi_client.setCACert(mqtt_ssl_cert);  // set SSL certificate
  mqttConnect();

  // send configuration to Home Assistant -- only do this by button press when needed
  //  SerialUSB.println("[MQTT] SEND CONFIGURATION OF ENTITIES TO HOME ASSISTANT");
  //  publishConfig();

  // force publish of initial states (dig in/out), values (ana in) and counters (dig in)
  SerialUSB.println("[INDIO] READ CHANNELS TO SYNC ENTITY STATES");
  readDigitalChannels(true);   // do force_publish -- including output channels, they will be updated soon by retained mqtt message /set
  readAnalogChannels(true);    // do force_publish on startup
  publishPulseCounters(true);  // do force_publish on startup
  //writeAnalogChannels();          // not necessary to set the output value, wait for retained mqtt message

  SerialUSB.println();
  SerialUSB.println("===============================");
  SerialUSB.println("END OF SETUP");
  SerialUSB.println("===============================");
  SerialUSB.println();

  // LCD display fixed items
  displayMain();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void loop() {

  myWDT.clear();  // watchdog reset

  mqtt_client.loop();  // handle MQTT connection

  // if MQTT connection lost, try to connect at intervals
  if (!mqtt_client.connected() && millis() - last_mqtt_attempt_ts > MQTT_RECONNECT_INTERVAL_SEC * 1000) {
    mqttConnect();  // uses LCD display
    last_mqtt_attempt_ts = millis();
    displayMain();  // restore fixed items display
  }

  // read digital channels every loop
  readDigitalChannels(false);  // do not force_publish, publish if changed
  /*
    // read only if digital channels have changed (interrupt) -- does not work reliably, previous state can change without publish
    if (digital_channel_change_flag) {
      SerialUSB.println("[ISR] flag set");
      readDigitalChannels();
      digital_channel_change_flag = 0;
    }
  */

  // at interval read analog channels and publish
  if (millis() - analog_read_ts > ANALOG_READ_INTERVAL_SEC * 1000) {
    readAnalogChannels(false);  // do not force_publish, publish if changed
    analog_read_ts = millis();
  }

  // at interval publish pulse counters
  if (millis() - pulse_counter_pub_ts > PULSE_COUNTER_PUB_INTERVAL_SEC * 1000) {
    publishPulseCounters(false);  // do not force_publish, publish if changed
    pulse_counter_pub_ts = millis();
  }

  // at interval refresh the display and check the buttons
  if (millis() - last_display_ts > 500) {
    displayData();
    handleButtons();
    last_display_ts = millis();
  }
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

void mqttConnect() {

  SerialUSB.println("[MQTT] run mqttConnect()..");

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[MQTT] connect");

  SerialUSB.print("[MQTT] connecting to server ");
  SerialUSB.print(mqtt_server);
  SerialUSB.print(" on port ");
  SerialUSB.println(mqtt_port);
  lcd.setCursor(0, 2);
  lcd.print("server:");
  lcd.setCursor(0, 3);
  lcd.print(mqtt_server);
  lcd.setCursor(0, 4);
  lcd.print("port: ");
  lcd.print(mqtt_port);

  // use last will/testament to indicate offline
  String availability_topic = "homeassistant/" + indio_mac + "/availability";
  SerialUSB.print("[MQTT] connecting with last will/testament message to topic: ");
  SerialUSB.println(availability_topic);

  if (mqtt_client.connect(indio_mac.c_str(), mqtt_user, mqtt_pwd, availability_topic.c_str(), 1, true, "offline")) {
    SerialUSB.println("[MQTT] connected");
    lcd.setCursor(0, 5);
    lcd.print("connected");
    /*
      // subscribe to status topic: this has birth/last will but default not retained
      // it shows the connection between broker and Home Assistant
      // not very interesting, broker and HASS are on same server, unlike to disconnect
      String status_topic = "homeassistant/status";
      SerialUSB.print("[MQTT] subscribe to topic: "); SerialUSB.print(status_topic);
      if (mqtt_client.subscribe(status_topic.c_str())) {
      SerialUSB.println(" OK");
      } else {
      SerialUSB.println(" FAIL");
      }
    */
    // publish 'online' to availability topic
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(availability_topic);
    SerialUSB.print(" payload: online");
    if (mqtt_client.publish(availability_topic.c_str(), "online", 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    // subscribe to digital input counter topics (number commands)
    // DIGITAL CH1-4: pulse counter = "number"
    SerialUSB.println("[MQTT] SUBSCRIBE TO PULSE COUNTER (number) ENTITIES");
    for (int i = 1; i <= 4; i++) {
      String this_topic = "homeassistant/number/" + indio_mac + "_counter_d" + String(i);
      String command_topic = this_topic + "/set";
      SerialUSB.print("[MQTT] subscribe to topic: ");
      SerialUSB.print(command_topic);
      if (mqtt_client.subscribe(command_topic.c_str())) {
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
    }
    // subscribe to digital output topics (switch commands)
    // DIGITAL CH5-8: OUTPUT = "switch"
    SerialUSB.println("[MQTT] SUBSCRIBE TO DIGITAL OUTPUT (switch) ENTITIES");
    for (int i = 5; i <= 8; i++) {
      String this_topic = "homeassistant/switch/" + indio_mac + "_d" + String(i);
      String command_topic = this_topic + "/set";
      SerialUSB.print("[MQTT] subscribe to topic: ");
      SerialUSB.print(command_topic);
      if (mqtt_client.subscribe(command_topic.c_str())) {
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
    }
    // subscribe to analog output topics (number commands)
    // ANALOG CH1-2: OUTPUT = "number"
    SerialUSB.println("[MQTT] SUBSCRIBE TO ANALOG OUTPUT (number) ENTITIES");
    for (int i = 1; i <= 2; i++) {
      String this_topic = "homeassistant/number/" + indio_mac + "_ao" + String(i);
      String command_topic = this_topic + "/set";
      SerialUSB.print("[MQTT] subscribe to topic: ");
      SerialUSB.print(command_topic);
      if (mqtt_client.subscribe(command_topic.c_str())) {
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
    }

  } else {
    SerialUSB.print("[MQTT] failed, rc=");
    int mqtt_status = mqtt_client.state();
    SerialUSB.print(mqtt_status);
    lcd.setCursor(0, 5);
    lcd.print("failed");
    lcd.setCursor(0, 6);
    if (mqtt_status == -2) {
      SerialUSB.println(" connect fail");
      lcd.print("connect failed");
    }
    if (mqtt_status == -4) {
      SerialUSB.println(" connect timeout");
      lcd.print("connect timeout");
    }
  }

  delay(1000);  // for displaying
}

//////////////////////////////////////////////////////////////////////////////////

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // receive topic and payload
  String payload_str = "";
  SerialUSB.print("[MQTT] message arrived on topic: ");
  SerialUSB.print(topic);
  SerialUSB.print(" with payload: " + payload_str);
  for (int i = 0; i < length; i++) {
    SerialUSB.print((char)payload[i]);
    payload_str += (char)payload[i];
  }
  SerialUSB.println();

  // look for command topic match
  String topic_str(topic);
  String start_str;
  String end_str = "/set";

  // DIGITAL OUTPUT: check for switch command from topic
  // format topic: homeassistant/switch/indiomac_d5/set with payload: ON
  start_str = "homeassistant/switch/" + indio_mac + "_d";
  if (topic_str.startsWith(start_str) && topic_str.endsWith(end_str)) {
    int index1 = start_str.length();
    int index2 = topic_str.indexOf(end_str);
    int this_channel = topic_str.substring(index1, index2).toInt();
    if (this_channel >= 5 && this_channel <= 8) {
      if (payload_str == "ON") {
        Indio.digitalWrite(this_channel, HIGH);
        SerialUSB.print("> [INDIO] switch channel ");
        SerialUSB.print(this_channel);
        SerialUSB.println(" ON");
      } else if (payload_str == "OFF") {
        Indio.digitalWrite(this_channel, LOW);
        SerialUSB.print("> [INDIO] switch channel ");
        SerialUSB.print(this_channel);
        SerialUSB.println(" OFF");
      } else SerialUSB.println("[MQTT] payload invalid, ignore");
    }
    // do not acknowledge by publishing to the /state topic, will be done in loop
  }

  // DIGITAL INPUT pulse counter: check for number command from topic
  start_str = "homeassistant/number/" + indio_mac + "_counter_d";
  if (topic_str.startsWith(start_str) && topic_str.endsWith(end_str)) {
    int index1 = start_str.length();
    int index2 = topic_str.indexOf(end_str);
    int this_channel = topic_str.substring(index1, index2).toInt();
    if (this_channel >= 1 && this_channel <= 4) {
      int set_value = payload_str.toInt();
      SerialUSB.print("> [INDIO] set counter for digital input channel ");
      SerialUSB.print(this_channel);
      SerialUSB.print(" to ");
      SerialUSB.println(set_value);
      dig_in_pulse_counter[this_channel] = set_value;
      SerialUSB.print("> [FRAM] update stored counter value");
      writeFRAMulong(FRAM_COUNTER_ADDRESS_START + (this_channel - 1) * 4, set_value);
      // acknowledge with update of the value topic
      String this_topic = "homeassistant/number/" + indio_mac + "_counter_d" + String(this_channel);
      String state_topic = this_topic + "/value";
      String this_payload = String(set_value);
      SerialUSB.print("> [MQTT] publish on topic: ");
      SerialUSB.print(state_topic);
      SerialUSB.print(" payload: ");
      SerialUSB.print(this_payload);
      if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // not retain?
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
    }
  }

  // ANALOG OUTPUT: check for number command from topic
  start_str = "homeassistant/number/" + indio_mac + "_ao";
  if (topic_str.startsWith(start_str) && topic_str.endsWith(end_str)) {
    int index1 = start_str.length();
    int index2 = topic_str.indexOf(end_str);
    int this_channel = topic_str.substring(index1, index2).toInt();
    if (this_channel >= 1 && this_channel <= 2) {
      float set_value = payload_str.toFloat();
      if (set_value >= 0 && set_value <= 100) {
        Indio.analogWrite(this_channel, set_value, false);  // not retain value in eeprom
        SerialUSB.print("> [INDIO] set analog output channel ");
        SerialUSB.print(this_channel);
        SerialUSB.print(" to ");
        SerialUSB.print(set_value, 2);
        SerialUSB.println("%");
        ana_out_ch_current_value[this_channel] = set_value;  // remember the value for display
                                                             // acknowledge with update of the value topic
        String this_topic = "homeassistant/number/" + indio_mac + "_ao" + String(this_channel);
        String state_topic = this_topic + "/value";
        String this_payload = String(set_value, 2);
        SerialUSB.print("> [MQTT] publish on topic: ");
        SerialUSB.print(state_topic);
        SerialUSB.print(" payload: ");
        SerialUSB.print(this_payload);
        if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // not retain?
          SerialUSB.println(" [OK]");
        } else {
          SerialUSB.println(" [FAIL]");
        }
      } else SerialUSB.println("[MQTT] payload invalid, ignore");
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void publishConfig() {

  // use MQTT discovery feature https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery
  // TO DO we could use abbreviations to reduce the payload size https://www.home-assistant.io/integrations/mqtt/#discovery-payload

  lcd.setCursor(0, 7);
  lcd.print("sending config..");

  // construct the config payload shared by all channels/entities = device details and availability topic
  String general_config_payload = "\"device\":{\"identifiers\":[\"indio" + indio_mac + "\"],";  // can use abbreviations https://www.home-assistant.io/integrations/mqtt/#discovery-payload
  general_config_payload += "\"manufacturer\":\"Industruino\",";
  general_config_payload += "\"model\":\"IND.I/O D21G\",";
  general_config_payload += "\"sw_version\":\"" + String(FILENAME) + "\",";
  general_config_payload += "\"name\":\"Industruino " + indio_mac + "\"},";
  general_config_payload += "\"availability_topic\":\"homeassistant/" + indio_mac + "/availability\"";

  // DIGITAL CH1-4: inputs = "binary_sensor"
  for (int i = 1; i <= 4; i++) {
    String this_entity = indio_mac + "_d" + String(i);
    String this_topic = "homeassistant/binary_sensor/" + this_entity;
    String config_topic = this_topic + "/config";
    String state_topic = this_topic + "/state";
    // create config payload
    String this_payload = "{\"name\":\"" + this_entity + "\",";  // can use abbreviations https://www.home-assistant.io/integrations/mqtt/#discovery-payload
    this_payload += "\"unique_id\":\"" + this_entity + "\",";
    this_payload += "\"state_topic\":\"" + state_topic + "\",";
    this_payload += general_config_payload;
    this_payload += "}";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(config_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(config_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    delay(1000);  // allow HASS to process the new entity/device
  }
  lcd.print(".");

  // DIGITAL CH1-4: inputs pulse counter = "number"
  for (int i = 1; i <= 4; i++) {
    String this_entity = indio_mac + "_counter_d" + String(i);
    String this_topic = "homeassistant/number/" + this_entity;
    String config_topic = this_topic + "/config";
    String state_topic = this_topic + "/value";
    String command_topic = this_topic + "/set";  // was /set
    // create config payload
    String this_payload = "{\"name\":\"" + this_entity + "\",";  // can use abbreviations https://www.home-assistant.io/integrations/mqtt/#discovery-payload
    this_payload += "\"unique_id\":\"" + this_entity + "\",";
    this_payload += "\"state_topic\":\"" + state_topic + "\",";
    this_payload += "\"command_topic\":\"" + command_topic + "\",";
    //    this_payload += "\"retain\":\"true\",";  // to receive the latest set value on startup, but we want to use FRAM stored counters
    this_payload += "\"max\":\"10000000\",";  // default is 100.0
    this_payload += "\"mode\":\"box\",";      // default shows slider in dashboard
    this_payload += general_config_payload;
    this_payload += "}";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(config_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(config_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    delay(1000);  // allow HASS to process the new entity/device
  }
  lcd.print(".");

  // DIGITAL CH5-8: OUTPUT = "switch"
  for (int i = 5; i <= 8; i++) {
    String this_entity = indio_mac + "_d" + String(i);
    String this_topic = "homeassistant/switch/" + this_entity;
    String config_topic = this_topic + "/config";
    String state_topic = this_topic + "/state";
    String command_topic = this_topic + "/set";
    // create config payload
    String this_payload = "{\"name\":\"" + this_entity + "\",";
    this_payload += "\"unique_id\":\"" + this_entity + "\",";
    this_payload += "\"state_topic\":\"" + state_topic + "\",";
    this_payload += "\"command_topic\":\"" + command_topic + "\",";
    this_payload += "\"retain\":\"true\",";  // to receive the latest value on startup
    this_payload += general_config_payload;
    this_payload += "}";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(config_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(config_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    delay(1000);  // allow HASS to process the new entity/device
  }
  lcd.print(".");

  // ANALOG INPUT CH1-4: "sensor"
  for (int i = 1; i <= 4; i++) {
    String this_entity = indio_mac + "_ai" + String(i);
    String this_topic = "homeassistant/sensor/" + this_entity;
    String config_topic = this_topic + "/config";
    String state_topic = this_topic + "/value";
    // create config payload
    String this_payload = "{\"name\":\"" + this_entity + "\",";
    this_payload += "\"unique_id\":\"" + this_entity + "\",";
    this_payload += "\"state_topic\":\"" + state_topic + "\",";
    this_payload += "\"unit_of_measurement\":\"%\",";
    this_payload += general_config_payload;
    this_payload += "}";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(config_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(config_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    delay(1000);  // allow HASS to process the new entity/device
  }
  lcd.print(".");

  // ANALOG OUTPUT CH1-2: "number"
  for (int i = 1; i <= 2; i++) {
    String this_entity = indio_mac + "_ao" + String(i);
    String this_topic = "homeassistant/number/" + this_entity;
    String config_topic = this_topic + "/config";
    String state_topic = this_topic + "/value";
    String command_topic = this_topic + "/set";
    // create config payload
    String this_payload = "{\"name\":\"" + this_entity + "\",";
    this_payload += "\"unique_id\":\"" + this_entity + "\",";
    this_payload += "\"state_topic\":\"" + state_topic + "\",";
    this_payload += "\"command_topic\":\"" + command_topic + "\",";
    this_payload += "\"retain\":\"true\",";  // to receive the latest value on startup
    this_payload += "\"unit_of_measurement\":\"%\",";
    this_payload += general_config_payload;
    this_payload += "}";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(config_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(config_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    delay(1000);  // allow HASS to process the new entity/device
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void readDigitalChannels(bool force_publish) {

  // check each channel, including the output channels
  for (int i = 1; i <= 8; i++) {
    // switch output channels temporarily to input
    if (i >= 5) Indio.digitalMode(i, INPUT);  // ch5-8
    bool dig_ch_now_state = Indio.digitalRead(i);
    if (i >= 5) Indio.digitalMode(i, OUTPUT);  // ch5-8
    // check if we need to publish (channel changed, or force publish)
    if (dig_ch_prev_state[i] != dig_ch_now_state || force_publish) {
      if (!force_publish) {
        SerialUSB.print("[INDIO] changed state detected on digital channel ");
        SerialUSB.println(i);
        // for inputs ch1-4, look for rising edge, count pulse
        if (i <= 4 && dig_ch_now_state && !dig_ch_prev_state[i]) {
          dig_in_pulse_counter[i]++;
          SerialUSB.print("[INDIO] pulse counter digital channel ");
          SerialUSB.print(i);
          SerialUSB.print(": ");
          SerialUSB.println(dig_in_pulse_counter[i]);
        }
      }
      String this_topic;
      if (i < 5) this_topic = "homeassistant/binary_sensor/" + indio_mac;  // ch1-4
      else this_topic = "homeassistant/switch/" + indio_mac;               // ch5-8
      this_topic += "_d" + String(i);
      String state_topic = this_topic + "/state";
      String this_payload;
      if (dig_ch_now_state) this_payload = "ON";
      else this_payload = "OFF";
      SerialUSB.print("> [MQTT] publish on topic: ");
      SerialUSB.print(state_topic);
      SerialUSB.print(" payload: ");
      SerialUSB.print(this_payload);
      if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // not retain?
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
      dig_ch_prev_state[i] = dig_ch_now_state;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void readAnalogChannels(bool force_publish) {

  // read analog input channels
  for (int i = 1; i <= 4; i++) {
    float ana_ch_now_value = Indio.analogRead(i);  // 0-100%
    // check if we need to publish (channel changed, or force publish)
    if (ana_in_ch_prev_value[i] != ana_ch_now_value || force_publish) {
      if (!force_publish) {
        SerialUSB.print("[INDIO] changed value detected on analog channel ");
        SerialUSB.println(i);
      }
      String this_topic = "homeassistant/sensor/" + indio_mac + "_ai" + String(i);
      String state_topic = this_topic + "/value";
      String this_payload = String(ana_ch_now_value, 2);
      SerialUSB.print("> [MQTT] publish on topic: ");
      SerialUSB.print(state_topic);
      SerialUSB.print(" payload: ");
      SerialUSB.print(this_payload);
      if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // retain
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
      ana_in_ch_prev_value[i] = ana_ch_now_value;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void publishPulseCounters(bool force_publish) {

  // only publish the values if changed from last publish
  // publish pulse counters for digital inputs ch1-4
  for (int i = 1; i <= 4; i++) {

    // read FRAM to check if the counter has changed
    unsigned long saved_counter = readFRAMulong(FRAM_COUNTER_ADDRESS_START + (i - 1) * 4);

    // if changed or force_publish, publish
    if (saved_counter != dig_in_pulse_counter[i] || force_publish) {
      // if changed, update FRAM
      if (!force_publish) {
        SerialUSB.print("[FRAM] updating pulse counter ");
        SerialUSB.print(i);
        SerialUSB.print(": ");
        SerialUSB.println(dig_in_pulse_counter[i]);
        writeFRAMulong(FRAM_COUNTER_ADDRESS_START + (i - 1) * 4, dig_in_pulse_counter[i]);
      }
      String this_topic = "homeassistant/number/" + indio_mac + "_counter_d" + String(i);
      String state_topic = this_topic + "/value";
      String this_payload = String(dig_in_pulse_counter[i]);
      SerialUSB.print("> [MQTT] publish counter on topic: ");
      SerialUSB.print(state_topic);
      SerialUSB.print(" payload: ");
      SerialUSB.print(this_payload);
      if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // retain
        SerialUSB.println(" [OK]");
      } else {
        SerialUSB.println(" [FAIL]");
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////
/*
void writeAnalogChannels() {  // could also be done in configIO() but better here for symmetry
// not necessary, wait for retained MQTT message to set the channel

  // set analog output channels
  for (int i = 1; i <= 2; i++) {
    Indio.analogWrite(i, 0, false);  // set to zero on startup, not retain value in eeprom
    String this_topic = "homeassistant/number/" + indio_mac + "_ao" + String(i);
    String state_topic = this_topic + "/value";
    String this_payload = "0";
    SerialUSB.print("[MQTT] publish on topic: ");
    SerialUSB.print(state_topic);
    SerialUSB.print(" payload: ");
    SerialUSB.print(this_payload);
    if (mqtt_client.publish(state_topic.c_str(), this_payload.c_str(), 1)) {  // retain
      SerialUSB.println(" [OK]");
    } else {
      SerialUSB.println(" [FAIL]");
    }
    ana_out_ch_current_value[i] = 0;
  }
}
*/
///////////////////////////////////////////////////////////////////////////////////////

void displayIntro() {

  lcd.clear();
  lcd.print("[HOME ASSISTANT]");

  lcd.setCursor(0, 1);
  lcd.print("firmware:");
  // print FILENAME wrapped, needs a bit of code to wrap..
  int len = 20;
  lcd.setCursor(0, 2);
  int index = 0;
  while (FILENAME[index] != '\0') {  // FILENAME is char array but sizeof() does not work
    if (index % len == 0) lcd.setCursor(0, 2 + index / len);
    lcd.print(FILENAME[index]);
    index++;
    if (index > 50) break;
  }
  lcd.setCursor(0, 2 + index / len + 1);
  lcd.print(__DATE__);

  lcd.setCursor(0, 6);
  lcd.print(mqtt_server);
  lcd.setCursor(0, 7);
  lcd.print("port: ");
  lcd.print(mqtt_port);
}

///////////////////////////////////////////////////////////////////////////////////////

void displayMain() {

  lcd.clear();
  lcd.print("[HOME ASSISTANT]");
  lcd.setCursor(0, 1);
  lcd.print("device: ");
  lcd.print(indio_mac);
  // digital ch1-8
  lcd.setCursor(0, 2);
  lcd.print("di1-4:");  // ch1-4
  lcd.setCursor(0, 3);
  lcd.print("do5-8:");  // ch5-8
  // analog inputs ch1-4
  for (int i = 1; i <= 4; i++) {
    lcd.setCursor(0, 3 + i);
    lcd.print("ai");
    lcd.print(i);
    lcd.print(":");
  }
  // analog outputs ch1-2
  for (int i = 1; i <= 2; i++) {
    lcd.setCursor(64, 1 + i);
    lcd.print("ao");
    lcd.print(i);
    lcd.print(":");
  }
  // digital input pulse counters ch1-4
  for (int i = 1; i <= 4; i++) {
    lcd.setCursor(64, 3 + i);
    lcd.print("di");
    lcd.print(i);
    lcd.print("c:");
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void displayData() {

  // digital inputs ch1-4
  lcd.setCursor(36, 2);
  for (int i = 1; i <= 4; i++) lcd.print(dig_ch_prev_state[i]);
  // digital outputs ch5-8
  lcd.setCursor(36, 3);
  for (int i = 5; i <= 8; i++) lcd.print(dig_ch_prev_state[i]);
  // analog inputs ch1-4
  for (int i = 1; i <= 4; i++) {
    lcd.setCursor(30, 3 + i);
    lcd.print(ana_in_ch_prev_value[i], 0);
    lcd.print("%  ");
  }
  // analog outputs ch1-2
  for (int i = 1; i <= 2; i++) {
    lcd.setCursor(64 + 30, 1 + i);
    lcd.print(ana_out_ch_current_value[i], 0);
    lcd.print("%  ");
  }
  // digital input pulse counters ch1-4
  for (int i = 1; i <= 4; i++) {
    lcd.setCursor(64 + 30, 3 + i);
    lcd.print(dig_in_pulse_counter[i]);
    lcd.print("     ");
  }
}

///////////////////////////////////////////////////////////////////////////////////////

void handleButtons() {

  // press UP to get to config publish confirmation
  if (!digitalRead(UP_PIN)) {
    SerialUSB.println("[BUTTON] UP pressed");
    lcd.clear();
    lcd.print("[CONFIG PUBLISH]");
    lcd.setCursor(0, 2);
    lcd.print("press DOWN to publish");
    lcd.setCursor(0, 3);
    lcd.print("config to HASS..");
    while (!digitalRead(UP_PIN)) {   // hold UP button
      if (!digitalRead(DOWN_PIN)) {  // press DOWN button
        SerialUSB.println("[BUTTON] DOWN pressed");
        SerialUSB.println("[MQTT] SEND CONFIGURATION OF ENTITIES TO HOME ASSISTANT");
        publishConfig();
      }
    }
    SerialUSB.println("[BUTTON] UP released");
    displayMain();
  }

  // press ENTER to see intro screen
  if (!digitalRead(ENTER_PIN)) {
    SerialUSB.println("[BUTTON] ENTER pressed");
    displayIntro();
    while (!digitalRead(ENTER_PIN))
      ;
    SerialUSB.println("[BUTTON] ENTER released");
    displayMain();
  }
}
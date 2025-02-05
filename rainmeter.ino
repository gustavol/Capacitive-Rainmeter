#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <RunningMedian.h>
#include "config.h"


#define ANALOG_FULL_RANGE   4096
#define HEAT_HYSTERESIS     25


#define WEBPAGESIZE         1024
char webpage[WEBPAGESIZE];
#define MQTTBUFFSIZE        256
char mqttbuff[MQTTBUFFSIZE];

int rawresult;
int rainintensity;


WiFiClient client;
WebServer server(80);
HTTPUpdateServer httpUpdater;
PubSubClient mqttclient(client);

#define RUNNING_MEDIANS     60
RunningMedian measurements = RunningMedian(RUNNING_MEDIANS);   


// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
                                
  Serial.setTxTimeoutMs(0);   // comment out for basic ESP32 variants with no on chip USB
                              // prevents slow serial prints on some ESP32 variants if CDC on boot is enabled, usb is connected but no terminal running
  
  pinMode(GPIO_HEATER, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);

  pinMode(GPIO_1MOHM, INPUT);
  pinMode(GPIO_CAPACITOR, OUTPUT);
  digitalWrite(GPIO_CAPACITOR, LOW);    // discharge the capacitor preparing it for the first measurement

  Serial.print("Connecting to wifi ");
  Serial.print(String(ssid));
  WiFi.setHostname(hostname);
  WiFi.begin(ssid, wifipassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis()>60000) {              // still no connection after one minute, reboot and retry
      ESP.restart();
    }
  }
  Serial.print(" done!\nSignal strength: ");
  Serial.println(String(WiFi.RSSI()));
  Serial.print("Webserver started at http://");
  Serial.println(WiFi.localIP());

  server.on("/", handlewebpage);
  server.begin();
  httpUpdater.setup(&server, ota_path, ota_username, ota_password);
  mqttclient.setServer(mqttserver, 1883);
  mqttConnect(); 
}



// -------------------------------------------------------------------
void loop() {

  unsigned long now = millis();

  server.handleClient();
  mqttclient.loop();
  
  static unsigned long timerone;
  if (now - timerone >= 50) {       // every 50ms
    timerone = now;
    heatercontrol();
  }

  static unsigned long timertwo;
  if (now - timertwo >= 1000) {     // every 1s
    timertwo = now;
    measurerain();
  }
  
  static unsigned long timerthree;
  if (now - timerthree >= 60000) {  // every 60s
    timerthree = now;
    mqttsend(rainintensity); 
  }
}



// -------------------------------------------------------------------
void measurerain (void) {

  unsigned long timerstart;
  
  heater_off();  // prevent overheating if stuck in the while loop below

  // -- stop discharging the now fully discharged capacitor by making gpio high impedance --
  pinMode(GPIO_CAPACITOR, INPUT); 

  // -- start charging the capacitor and measure the time to charge it to 63% --  
  pinMode(GPIO_1MOHM, OUTPUT);
  digitalWrite(GPIO_1MOHM, HIGH);
  timerstart = micros();
  while (analogRead(GPIO_CAPACITOR) < 0.63*ANALOG_FULL_RANGE);
  rawresult = (int)(micros()-timerstart);

  // Repackage result
  measurements.add(constrain(rawresult-CAPACITANCE_OFFSET, -500, 8000));      // remove offset and sanitize and then smooth and remove further outliers with the median array
//  rainintensity = (int)(10*cbrt(max(measurements.getMedian(),(float)0.0)));   // make range feel more linear      
  rainintensity = (int)(3*sqrt(max(measurements.getMedian(),(float)0.0)));   // make range feel more linear      

  // -- start discharging capacitor preparing it for the next measurement --  
  pinMode(GPIO_CAPACITOR, OUTPUT);
  digitalWrite(GPIO_CAPACITOR, LOW);
}



// -------------------------------------------------------------------
bool mqttConnect(void) {
  
  if (mqttclient.connected()) {
    return true;
  }
  Serial.print("Connecting to mqtt server ");
  Serial.print(String(mqttserver));
  if (mqttclient.connect(hostname, mqtt_username, mqtt_password)) {
    digitalWrite(BLUE_LED, HIGH);
    Serial.println(" done!");
    return true;
  } 
  digitalWrite(BLUE_LED, LOW);
  Serial.print(" failed, rc=");
  Serial.println(mqttclient.state());
  return false;
}



// -------------------------------------------------------------------
void mqttsend (int rain) {

  if (!mqttclient.connected()) {
    mqttConnect();
  }
  snprintf( mqttbuff, MQTTBUFFSIZE, "{\"~\":\"homeassistant/sensor/rainmeter\",\"name\":\"%s\",\"unique_id\":\"%s\",\"device_class\":\"precipitation_intensity\",\"stat_t\":\"~/state\"}", haName, haUniqid); 
  mqttclient.publish("homeassistant/sensor/rainmeter/config", mqttbuff);
  Serial.println(mqttbuff);
  snprintf( mqttbuff, MQTTBUFFSIZE, "%d", rain );
  mqttclient.publish("homeassistant/sensor/rainmeter/state", mqttbuff);
  Serial.println(mqttbuff);
}



// -------------------------------------------------------------------
void heatercontrol(void) {    
  int temperature = ANALOG_FULL_RANGE-analogRead(GPIO_NTC);
  if (temperature < HEAT_LEVEL && rainintensity>0) {    // only heat up the sensor if it detects rain or snow
      heater_on();
  }
  if (temperature > HEAT_LEVEL+HEAT_HYSTERESIS) {
      heater_off();
  }
}

void heater_on(void) {    
  digitalWrite(GPIO_HEATER, HIGH);
  digitalWrite(YELLOW_LED, HIGH);
}

void heater_off(void) {    
  digitalWrite(GPIO_HEATER, LOW);
  digitalWrite(YELLOW_LED, LOW);
}




// -------------------------------------------------------------------
void handlewebpage(void){

  static int max_offset = 0;
  static int max_rain = 0;

  max_offset = max((int)measurements.getMedian()+CAPACITANCE_OFFSET, max_offset);

  snprintf( webpage, WEBPAGESIZE, " \
  <html><head><meta http-equiv=refresh content=3></head><body><pre>\n \
  Hostname:     %s \n \
  Wifi signal:  %d dB \n \
  \n \
  Mqtt server:  %s \n \
  Connected:    %d \n \
  HA name:      %s \n \
  HA uniq_id:   %s \n \
  \n \
  \n \
  When the board is freshly rebooted and the sensor is \n \
  dry take the value from below and add to config.h \n \
  \n \
  <span style=\"color:#0c0;font-weight:bold;\">&#35;define CAPACITANCE_OFFSET %d </span>\n \
  \n \
  \n \
  Currently entered in config.h: \n \
  \n \
  <span style=\"color:#00e;font-weight:bold;\">&#35;define CAPACITANCE_OFFSET %d </span>\n \
  \n \
  \n \
  If the value of CAPACITANCE_OFFSET in config.h is set \n \
  slightly too low the reported rainintensity value will \n \
  occasionally rise above zero even whith no rain \n \
  \n \
  Current rainintensity: %d \n \
  </pre></body></html> \
  ", hostname, WiFi.RSSI(), mqttserver, mqttclient.connected(), haName, haUniqid, max_offset, CAPACITANCE_OFFSET, rainintensity ); 
  server.send(200, "text/html", webpage );
}




// --------------------------

// Air quality main source code

#include "Screen.h"
#include <Wire.h>
#include <WiFi.h>
#include "Sensor.h"
#include "Battery.h"
#include "MqttClient.h"
#include "SdCard.h"
#include <esp32fota.h>
#include "SPIFFS.h"
#include "TimeKeeper.h"
// #include <esp_arduino_version.h> // works from V2

#define BUTTON_1                (39) // pin for button1 used for wake up

#define CO2_WIFI_ENABLED 1           // 1 to enable WiFi

#define FIRMWARE_VERSION 6           // firmware version. should match AirQualityBackend/static/fota/version.h
const char* firmwareName = "AirQ";   // firmware name. should match AirQualityBackend/static/fota/version.h

esp_sleep_source_t wakeupReason;
RTC_DATA_ATTR int bootCount = 0;     // keep tracks of boot from wake up
RTC_DATA_ATTR int wifiConnectionFailures = 0; // keep track of wifi connection failures to avoid spending battery when no wifi
int loopCount = 0;

const char* formatDateTime = "%d %b %H:%M:%S";
TimeKeeper timeKeeper; // class to keep track of time
Screen ePaper; // class to handle GUI
Sensor sensor; // class to handle sensor
Battery battery; // class to handle battery monitoring
SdCard sdCard(&timeKeeper); // class to handle storage of measures on sd card
esp32FOTA esp32FOTA(String(firmwareName), FIRMWARE_VERSION, false);

// TODO: avoid hard coded WiFi & mosquitto credentials
const char* server = "http://192.168.1.108:5000/";
const char* ssid="TP-Link_CD96";
const char *password ="24232876";
const char *mqttBroker = "192.168.1.108";
const char *mqttUser = "";
const char *mqttPassword = "";
const int mqttPort = 1883;
MqttClient *mqttClient = NULL;
WiFiClient wifiClient;
PubSubClient pubSubClient(wifiClient);

// scan wifi access point for test purpose. Not used.
void testWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();

    Serial.println("scan done");
    if (n == 0) {
        Serial.println("no networks found");
    } else {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }
    }
    Serial.println("");
}

// set the current time
int setTime(double timeStamp) // seconds since epoch including ms as decimals
{
  log_v("Got time %s", timeKeeper.toDate(timeStamp));
  timeKeeper.setTime(timeStamp);
  return 1;
}

// disbale Wifi when not needed to save battery
void disableWifi()
{
  // reduce power: https://www.mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1/
      /*
        Note: at 80Mhz, no wifi, no bluetooth we have:
        - 26 mA without reading
        - 74 mA while reading every 5s
        with an average at 40 mA, a battery of 2800 mAh should last around 70h ~ 3 days

        at 80Mhz, one shot reading: noisy (variations +/- 200 ppm)
        - 25~75 mA while reading
        - 0.49 mA in hibernation with sensirion powered on
        - 0.29 mA in hibernation with sensition powered off

        at 80MHz, low power reading
        - 26 mA in refresh (~5s)
        - 2 mA ~ 50 mA while reading (~1s)
        - 0.94 mA between read
      */
  WiFi.disconnect(true); // Disconnect from the network
  WiFi.mode(WIFI_OFF);   // turn off wifi
}

// start Wifi/MQTT connection
void wifiConnection()
{
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(100);
  WiFi.begin(ssid, password);
  int i = 0;
  wl_status_t status = WL_IDLE_STATUS;
  for (i=0; i < 30 && status != WL_CONNECTED; i++)
  {
    delay(500);
    status = WiFi.status();
    log_d ("status = %d", status);
  }

  if (status == WL_CONNECTED)
  {
    String message = "connected to " + WiFi.localIP().toString();
    log_d("%s", message.c_str());
    ePaper.printMessage(message);

    log_d("Check firmware OTA");
    esp32FOTA.checkURL = String(server) + String("check_fota");
    bool updatedNeeded = esp32FOTA.execHTTPcheck();
    if (updatedNeeded)
    {
      log_d("Firmware OTA started...");
      ePaper.printMessage("OTA firmware update started, please wait");
      esp32FOTA.execOTA();
      // does not go any further since execOTA reboots
    }
    else
    {
      log_d("no need to update firmware");
    }
  }
  else
  {
    String message = "WiFi error " + String(status);
    wifiConnectionFailures++;
    ePaper.printMessage(message);
    disableWifi();
    return;
  }

  log_d("Init mqtt");
// TODO: handle TLS
  mqttClient = new MqttClient();

  mqttClient->setTimeCB = &(setTime);
  mqttClient->init(&pubSubClient, sensor.getSerial());
  mqttClient->setBrokerCredentials(mqttBroker, mqttPort, mqttUser, mqttPassword);
  mqttClient->publishAnouncement();
  log_d("mqtt initialized");
}

// Check chip id for test purpose.
void checkChip()
{
  esp_chip_info_t chip_info;

  esp_chip_info(&chip_info);
  log_v("This is ESP32 chip with %d CPU cores, WiFi%s%s, ",
            chip_info.cores,
            (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
            (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  log_v("silicon revision %d, ", chip_info.revision);

  log_v("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  uint8_t mac[6];
  char macString[13];
  esp_efuse_mac_get_default(mac);
  for (int i=0; i<6; i++) sprintf(macString+(2*i), "%02X", mac[i]);
  macString[12] = 0;
  log_v ("MAC: %s\n", macString);
}

// Go to deep sleep for a time.
void sleep(int seconds)
{
    ePaper.powerDown();
    sdCard.close();

    if (seconds <= 0)
    {
      log_d("sleep until button pressed");
      sensor.powerDown();
    }
    else
    {
      esp_sleep_enable_timer_wakeup(seconds * 1000 * 1000);
      log_d("sleep for %d seconds or until button pressed", seconds);
    }

    esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ALL_LOW);
    esp_deep_sleep_start();
}

// the startup function called once by Arduino on boot
void setup()
{
    Serial.begin(9600);
    log_d("setup");
    
    wakeupReason = esp_sleep_get_wakeup_cause();
    bootCount++;
    log_d("wakeup reason = %d, boot count = %d", wakeupReason, bootCount);

    log_v("ESP IDF %s", esp_get_idf_version());
#ifdef ESP_ARDUINO_VERSION_MAJOR // works from ESP32 Arduino V2. Still use 1.0.6 for the time being
    log_v("ESP_ARDUINO V%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
#endif

    checkChip(); // for test purpose
    btStop();              // disconnect bluetooth
    setCpuFrequencyMhz(80); // good enough, save power and heat...

    log_d("%d samples left in sensor stack", sensor.stackSpace()); // this does not require sensor.init() that takes time
    bool needWifi = false;
#ifdef CO2_WIFI_ENABLED
    needWifi = (wakeupReason != ESP_SLEEP_WAKEUP_TIMER || 
                (sensor.stackSpace() < 3 && wifiConnectionFailures < 3));
#endif
    if (!needWifi)
    {
      disableWifi();  // disable wifi asap, prior time consuming initializations...
    }

    bool spiffOk = SPIFFS.begin();
    if (!spiffOk)
    {
      log_v("Formatting spiff...");
      if (SPIFFS.format())
      {
        spiffOk =  SPIFFS.begin();
      }
    }

    Wire.begin();
    ePaper.init();

    int sensorMode = (wakeupReason == ESP_SLEEP_WAKEUP_TIMER) ? SENSOR_LOW_POWER : SENSOR_NORMAL;
    log_d ("sensor mode = %d", sensorMode);

    if (! sensor.init(sensorMode))
    {
      log_e("%s", sensor.getErrorMessage().c_str());
    }

    ePaper.info1 = String(firmwareName) + " V" + String(FIRMWARE_VERSION) + " - SCD41";
    ePaper.info2 = String(timeKeeper.toDate(timeKeeper.now(), formatDateTime));
    battery.init();
    sdCard.init(String(sensor.getSerial()), wakeupReason != ESP_SLEEP_WAKEUP_TIMER);

    if (needWifi)
    {
      // mqttClient = new MqttClient();
      wifiConnection(); // this requires prior initializations of screen and sensor for serial number
    }
}

// the loop function called forever by Arduino framework after setup
void loop()
{
  battery.measure();
  ePaper.bat = battery.load;

  double now = timeKeeper.now();
  ePaper.info2 = String(timeKeeper.toDate(now, formatDateTime));
  if (sensor.measure(now))
  {
    log_i("%d ,\t%f ,\t%f", (int)sensor.last.co2, sensor.last.temperature, sensor.last.humidity);

    ePaper.co2 = sensor.last.co2;
    ePaper.temp = sensor.last.temperature;
    ePaper.h2o = sensor.last.humidity;
  }
  else
  {
    log_e("%s", sensor.getErrorMessage().c_str());
  }

  ePaper.printLevels();

  if (mqttClient != NULL) 
  {
    for (int i=0; i<30 && !mqttClient->loop(); i++)
    {
      log_d("wait for MQTT (%ds)", i);
      delay(1000);
    }

    if (mqttClient->loop())
    {
      mqttClient->publishMonitoring(battery.voltage, battery.load);
      int numMeasures = 0;
      struct Measure *measures = sensor.getStack(&numMeasures);
      log_d("%d measures to send", numMeasures);
      for (int i=0; i<numMeasures; i++)
      {
        mqttClient->publishMeasure(measures->timeStamp, measures->co2, measures->temperature, measures->humidity);
        mqttClient->loop();
        measures++;
      }
      delay(100); // give more time to mqtt, not sure if this is needed...
    }
  }

  sdCard.appendRecord(sensor.last.co2, sensor.last.temperature, sensor.last.humidity, battery.voltage);

  int bat0 = 0;
  while (battery.load == 0)
  {
    log_i("voltage = %fV, load = %d%%\n", battery.voltage, battery.load);

    if (bat0++ < 3)
    {
      delay(100);
    }
    else
    {
      sleep(-1);
    }

    battery.measure();
  }

  if (sensor.getMode() == SENSOR_LOW_POWER)
  {
    sleep(60); 
    // we do not got continue beyond this point since sleep with ends up in setup 1 min later
  }

  if (sensor.getMode() == SENSOR_NORMAL && loopCount > 10)
  {
    sensor.changeMode(SENSOR_LOW_POWER);
  }

  loopCount++;
  delay(5000); // wait 5 s since in normal mode, we get a measure every 5 s.
}

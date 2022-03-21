# AirQuality

The purpose of this project is indoor air quality monitoring with affordable IoT devices. It covers all aspects of the problem, from sensors to data monitoring, including 3D modeling, electronics, communication protocols, data base and web front end.

## IoT Device

The device displays CO2 level with a smiley depending on the CO2 level, so it can be used in standalone mode.

![image](https://user-images.githubusercontent.com/20434204/156890097-6f668abd-a406-48f5-88e0-f4490e710f5c.png)

The device is based on 2 circuits, connected via I2C in a [3D printed casing](https://github.com/ChristopheLaurent/AirQuality/tree/main/Model3D):
### [Sensirion SCD41](https://developer.sensirion.com/sensirion-products/scd4x-co2-sensors/)
The SEK-SCD41-Sensor Evaluation Kit can measure CO2 level, Humidity and Temperature. It is powered in 3.3V. The sampling rate is 5s in normal mode, 30s in low power mode.
### [LILYGO® TTGO T5 V2.3.1_2.13](http://www.lilygo.cn/prod_view.aspx?TypeId=50061&Id=1393&FId=t3:50061:3) 
This [ESP32 ECO V3](https://www.espressif.com/sites/default/files/documentation/ESP32_ECO_V3_User_Guide__EN.pdf) module integrates an e-Paper screen, SD card, USB, JSP plug for battery support, WiFi and Bluetooth. The JST plug can be connected to an optional 18650 lithium battery. The USB can be connected to a 5V/1A mobile phone power supply.

## Backend server

The devices can connect to a Flask server via WiFi / MQTT.

![image](https://user-images.githubusercontent.com/20434204/156892154-178326cb-2f74-429d-9aa1-47f284b16314.png)

The Flask server can run on a PC, together with a mosquitto broker and PostgreSQL data base

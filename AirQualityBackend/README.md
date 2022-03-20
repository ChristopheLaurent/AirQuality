# Backend

AirQuality backend is designed to run on a local WiFi network, not the cloud. It handles MQTT publications from sensors south bound, and Web request from browsers north bound. It stores all measures from sensors on a local PostGres data base.

![image](https://user-images.githubusercontent.com/20434204/156892154-178326cb-2f74-429d-9aa1-47f284b16314.png)

## Data model

The data base is designed to support the following features:
- Each sensor is identified by its serial number.
- Each sensor has a type (for example SCD41, but it could be something else)
- Each sensor type has a set of channels.
- Each channel is defined by a name (for example CO2) and a unit (for example ppm)
- Each channel has a set of measures, e.g. samples (time stamp and value)
- At a given period of time, a sensor is in a location 
- A location is identified by a building, floor, room number...

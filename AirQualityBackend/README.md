# Backend

AirQuality backend is designed to run on a local WiFi network, not the cloud. It handles MQTT publications from sensors south bound, and Web request from browsers north bound. It stores all measures from sensors on a local PostGres data base.

![image](https://user-images.githubusercontent.com/20434204/156892154-178326cb-2f74-429d-9aa1-47f284b16314.png)

## Windows Installation steps

### PostgreSQL
- [Install postgres DB](https://www.postgresql.org/download/windows/)
- You can use `admin` as password. (it is hardcoded in the source code for the time being)
- Do not launch stack builder at exit.
- Create database `AirQuality` (using pgAdmin for example)

### Mosquitto
- [Install Mosquitto broker](https://mosquitto.org/download/)
- If you choose to run mosquitto as a service, you can reboot your computer then check mosquitto.exe process is running.
- If you decide to run it manually, go to the mosquitto folder with a command prompt then type `mosquitto -v`

### Python
- [Install latest Python 3 version](https://www.python.org/downloads/)
- During installation process, ticks Add Python 3.xx to PATH

Run as admin windows power shell then type the following commands:
-	`cd AirQuality\AirQualityBackend`
-	`Set-ExecutionPolicy Unrestricted -Force`
-	`pip install virtualenv`
-	`virtualenv venv`
-	`venv\Scripts\activate`
-	`pip install Flask`
-	`pip install psycopg2`
-	`pip install flask-mqtt`
-	`python init_db.py`
-	`$env:FLASK_APP = "app"`
-	`flask run --host=0.0.0.0`
-	Open your favorite browser then go to [http://127.0.0.1:5000/raw](http://127.0.0.1:5000/raw). You should see an empty table:

![image](https://user-images.githubusercontent.com/20434204/159179162-9971dff5-f468-49b9-a144-a2a41ee92656.png)

-	In another power shell, execute `python twin.py` . This will run a digital twin that simulates a sensor to populate the data base. If you refresh your browser, you should then see some data :



## Data model

The data base is designed to support the following features:
- Each sensor is identified by its serial number.
- Each sensor has a type (for example SCD41, but it could be something else).
- Each sensor type has a set of channels.
- Each channel is defined by a name (for example CO2) and a unit (for example ppm).
- Each channel has a set of measures, e.g. samples (time stamp and value).
- At a given period of time, a sensor is in a location.
- A location is identified by a building, floor, room number...

![image](https://user-images.githubusercontent.com/20434204/159177875-cd676da6-6f5a-4132-9506-f623b03e84d7.png)

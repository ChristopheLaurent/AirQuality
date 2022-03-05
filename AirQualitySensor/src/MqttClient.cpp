#include "MqttClient.h"
#include "ArduinoJson.h"
#include <WiFi.h>

#define SERIAL_DEBUG 1

#ifdef SERIAL_DEBUG    
#ifdef Arduino_h
#define DPRINT(...)    log_d(__VA_ARGS__)  //assume that 1st arg is a format
#else
#define DPRINT(...)    log_d(__VA_ARGS__)     //assume that 1st arg is a format
#endif
#else
#define DPRINT(...)     //now defines a blank line
#endif

#define ID_LENGTH 12
#define MIN_TIME_BETWEEN_CONNECTION_ATTEMPTS 30000 // ms
const char* topicPrefix  = "AirQuality";

const char *topicString[] =
{
    // Master Out - Slave In topics (sequentially ordered)
    "SetTime",
    // Master In - Slave Out topics
    "Connect",
    "Time",
    "Measure",
    "Monitoring",
    "Error",
    NULL
};

MqttClient *instance;

void callback(char *topic, byte *payload, unsigned int length)
{
    DPRINT("new command: topic = [%s], payload length = %d, instance=%p, payload = %p [", topic, length, instance, payload);
    for (unsigned int i = 0; i < length && i < 8; i++)
    {
        DPRINT("%2X,", payload[i]);
    }
    DPRINT("]\n");

    char *topicEnd = topic + strlen(topicPrefix) + ID_LENGTH + 2;

    for (int i = FIRST_MOSI_TOPIC; i <= LAST_MOSI_TOPIC; i++)
    {
        if (strcmp(topicEnd, topicString[i]) == 0)
        {
            switch ((Topic)i)
            {
                case SetTime:
                {
                    StaticJsonDocument<16> doc;
                    // char json[256];
                    // memcpy(json, payload, (size_t)length);
                    // json[length] = '\0';
                    // log_w("json=%s", json);
                    deserializeJson(doc, payload, length);
                    double timeSet = doc["now"];
                    log_v("got time %lf", timeSet);
                    instance->setTimeCB(timeSet);
                    break;
                }
                default:
                {
                    char errorMsg[50];
                    sprintf(errorMsg, "%s should not be used in this context", topicString[i]);
                    instance->error(errorMsg);
                }
            }
        }
    }
}

MqttClient::MqttClient()
{
	_isInitialized = false;
    _timeOfLastConnectionAttempt = 0;
}

void MqttClient::init(PubSubClient *pubSubClient, char* sensorId)
{
	DPRINT("MqttClient init...");
	_pubSubClient = pubSubClient;
	instance = this;
	
	DPRINT("random seed...");
	randomSeed(micros());
	
	DPRINT("get id...");
	// byte mac[6];
	// WiFi.macAddress(mac);
	// sprintf(_id, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); // check if trailing \0 needed
    strcpy(_id, sensorId);
	DPRINT("MqttClient created: _id = %s\n", _id);
	
	if (MQTT_SOCKET_TIMEOUT > MQTT_KEEPALIVE)
	{
		_timeOut = MQTT_KEEPALIVE * 1000UL;
	}
	else
	{
		_timeOut = MQTT_SOCKET_TIMEOUT * 1000UL;
	}

	_isInitialized = true;
}


void MqttClient::setBrokerCredentials(const char *broker, unsigned int port, const char* mqttUser, const char* mqttPassword)
{
	DPRINT("set broker to [%s], instance=%p\t", broker, instance);
	_user = mqttUser;
	_password = mqttPassword;
	_pubSubClient->setServer(broker, port);
	_pubSubClient->setCallback(callback);
}

char* MqttClient::setTopic(Topic topic)
{
	return setTopic(topic, _id);
}

char* MqttClient::setTopic(Topic topic, char* source)
{
	sprintf (_lastTopic, "%s/%s/%s", topicPrefix, source, topicString[topic]);
	return _lastTopic;
}

void MqttClient::error(char *errorMsg)
{
    if (_pubSubClient->connected())
    {
        _pubSubClient->publish(setTopic(Error), errorMsg);
    }
}

bool MqttClient::publishMeasure(double timeStamp, int co2, float temp, int h2o)
{
    bool ret = false;
    if (_pubSubClient->connected())
    {
        DPRINT("Publish %s = %d %%\n", setTopic(Measure), co2);
        sprintf (_payload, "{\"TS\":%lf,\"CO2\":%d,\"Temp\":%.1f,\"H2O\":%d}", timeStamp, co2, temp, h2o);
        bool ret = _pubSubClient->publish(setTopic(Measure), _payload);
    }
	return ret;
}

bool MqttClient::publishMonitoring(float voltage, int load)
{
    bool ret = false;
    if (_pubSubClient->connected())
    {
        DPRINT("Publish %s = %f V\n", setTopic(Monitoring), voltage, load);
        sprintf (_payload, "{\"Voltage\":%f, \"Load\":%d}", voltage, load);
        bool ret = _pubSubClient->publish(setTopic(Monitoring), _payload);
    }
	return ret;
}

bool MqttClient::publishAnouncement()
{
    bool ret = false;
    if (_pubSubClient->connected())
    {
        int16_t rssi = (int16_t)(WiFi.RSSI());
        DPRINT("publish an announcement, RSSI = %d\n", rssi);
        sprintf (_payload, "{\"RSSI\":%d}", rssi);
        bool ret = _pubSubClient->publish(setTopic(Connect), _payload);
    }
	return ret;
}

bool MqttClient::connect()
{
    bool ret = true;
    // Loop until we're reconnected
    if (!_pubSubClient->connected())
    {
        ret = false;

        // dot not try to connect too frequently
        long now = (long)millis();
        if ((now - _timeOfLastConnectionAttempt) < MIN_TIME_BETWEEN_CONNECTION_ATTEMPTS)
        {
            DPRINT("Skip connection for %s...\n", _id);
            return ret;
        }

        // Attempt to connect
        DPRINT("Attempting MQTT connection for %s...\n", _id);
        _timeOfLastConnectionAttempt = now;

        wl_status_t wifiStatus = WiFi.status();
        if (wifiStatus == WL_CONNECTED)
        {
            String clientId = _id;
            clientId += String(random(0xffff), HEX);

            if (_pubSubClient->connect(clientId.c_str(), _user, _password))
            {
                DPRINT("connected, subscribing...");

                _pubSubClient->subscribe(setTopic(SetTime, "000000000000"));

                // Once connected, publish an announcement...
                ret = publishAnouncement();
            }
            DPRINT("connection failure: %d\n", _pubSubClient->state());
        }
        DPRINT("Wifi status = %d\n", wifiStatus);
    }
    return ret;
}

bool MqttClient::loop()
{
    if (connect())
	{
		return _pubSubClient->loop();
    }
    else
    {
        DPRINT("MqttClient::loop() -> not connected !\n");
    }

    return false;
}

#ifndef MqttClient_h
#define MqttClient_h

#include <Arduino.h>
#include <PubSubClient.h>

#define ID_LENGTH 13 // a MAC address = 6 x 02X

enum Topic
{
  // Master Out - Slave In topics (sequentially ordered)
  SetTime = 0, // not yet implemented

  // Master In - Slave Out topics
  Connect, // payload: {"RSSI":x}
  Time, // not yet implemented
  Measure, // payload: {"CO2":x, "Temp":y, "H2O":z} note: CO2 in ppm, Temp in degC, H2O in %
  Monitoring, // payload: {"Voltage":x, "Load":y} note voltage in V, load in %
  Error, // payload: string_t (hardware related error message, not yet implemented, etc...)
  NumTopic // unknown topic. must be the last item in the enum list
};

#define FIRST_MOSI_TOPIC SetTime
#define LAST_MOSI_TOPIC SetTime
#define FIRST_MISO_TOPIC Connect
#define LAST_MISO_TOPIC Error

#define MAX_TOPIC_LENGTH 64
#define MAX_PAYLOAD_LENGTH 128


class MqttClient
{
	public:
    MqttClient();
    void init(PubSubClient *pubSubClient, char* sensorId);
    void setBrokerCredentials(const char *mqttBroker, unsigned int port, const char *mqttUser, const char *mqttPassword);
    bool loop();
    bool publishMeasure(double timeStamp, int co2, float temp, int h2o);
    bool publishMonitoring(float voltage, int load);
	  bool publishAnouncement();
	  void error(char *errorMsg);
    int (*setTimeCB)(double timestamp);
  private:
    bool connect();
  	char *setTopic(Topic topic);
	  char *setTopic(Topic topic, char *source);
    const char *_user;
    const char *_password;
    PubSubClient *_pubSubClient;
    char _id[ID_LENGTH];
	  char _lastTopic[MAX_TOPIC_LENGTH];
    char _payload[MAX_PAYLOAD_LENGTH];
    PubSubClient _client;
    unsigned int _timeOut;
	  bool _isInitialized = false;
    long _timeOfLastConnectionAttempt;
};
    
#endif
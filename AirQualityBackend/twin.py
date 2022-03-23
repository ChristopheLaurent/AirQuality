# AirQuality digital twin

import paho.mqtt.client as mqtt
import json
import math
import uuid
from datetime import datetime, timezone
import time

sensor_type = 'SCD41' # hard coded for the time being
serial_number = 'TWIN' + hex(uuid.getnode())[2:10]

def on_connect(client, userdata, flags, rc):
    """
    keep track on connection
    """
    if rc == 0:
        print("Connected to broker")
        global Connected                #Use global variable
        Connected = True                #Signal connection
        client.subscribe('AirQuality/000000000000/SetTime') # subscribe to time
    else:
        print("Connection failed")

def on_message(client, userdata, message):
    """
    handle new subscribed message
    """
    payload=message.payload.decode()
    print("Got message " + message.topic + " with payload " + payload)
    topic_items = message.topic.split('/')
    if (topic_items[2] == 'SetTime'):
        data = json.loads(payload)
        for key, value in data.items(): 
            if (key == 'now'): 
                time_stamp = datetime.fromtimestamp(value)
                print ("Received reference time " + time_stamp.strftime("%H:%M:%S") + " current time is " + datetime.now(timezone.utc).strftime("%H:%M:%S"))
            else:
                print ("Unknown property " + key)
    else:
        print ("Unknown topic " + message.topic)

print ('start digital twin ' + serial_number)
mqttc =  mqtt.Client(serial_number)
mqttc.connect("192.168.1.108", 1883, 60)
mqttc.on_connect= on_connect                      #attach function to callback
mqttc.on_message= on_message                      #attach function to callback

mqttc.loop_start()
try:
    while True:
        timeStamp = datetime.now().timestamp() # number of seconds since epoch, with milliseconds
        co2 = timeStamp % 2000
        h2o = (timeStamp/10) % 100
        temp = 10 + (timeStamp/100) % 20
        payload = json.dumps({"TS": timeStamp, "CO2": co2, "Temp": temp, "H2O": h2o})
        topic = 'AirQuality/'+serial_number+'/Measure'
        print ('publish ' + topic + ' ' + payload)
        mqttc.publish(topic, payload)
        time.sleep(5)# sleep for 5 seconds before next call

except KeyboardInterrupt:
    print ("exiting")
    mqttc.disconnect()
    mqttc.loop_stop()

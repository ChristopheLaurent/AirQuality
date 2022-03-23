# AirQuality server

import psycopg2
import json
import math
import io
import os
from db_api import *
from flask import Flask, render_template, request, send_file, jsonify
from flask_mqtt import Mqtt
from datetime import datetime, timezone
import time

app = Flask(__name__)
# TODO Find a way to setup MQTT credentials properly.
app.config['MQTT_BROKER_URL'] = '127.0.0.1'  # use the local mosquitto broker
app.config['MQTT_BROKER_PORT'] = 1883  # default port for non-tls connection
app.config['MQTT_USERNAME'] = ''  # set the username here if you need authentication for the broker
app.config['MQTT_PASSWORD'] = ''  # set the password here if the broker demands authentication
app.config['MQTT_KEEPALIVE'] = 5  # set the time interval for sending a ping to the broker to 5 seconds
app.config['MQTT_TLS_ENABLED'] = False  # set TLS to disabled for testing purposes

sensor_type = 'SCD41' # hard coded for the time being

# handle mqtt
sensor_ids = []

try:
    mqtt = Mqtt(app)
    
    @mqtt.on_connect()
    def handle_connect(client, userdata, flags, rc):
        """
        connect flask server to mqtt broker.
        """
        print("CLIENT CONNECTED")
        mqtt.subscribe('AirQuality/#') # multi level wildcard => subscribe to everything from AirQuality sensors
        publishTime()

    @mqtt.on_disconnect()
    def handle_disconnect():
        """
        disconnect flask server to mqtt broker
        """
        print("CLIENT DISCONNECTED")

    @mqtt.on_message()
    def handle_mqtt_message(client, userdata, message):
        """
        handle new subscribed message
        """
        payload=message.payload.decode()
        time_stamp = datetime.now(timezone.utc)
        print(payload)
        topic_items = message.topic.split('/')
#TODO: Fix crash in case of wrong topic, this could be an attack
        conn = get_db_connection()
        sensor_id = getOrInsertSensor(conn, sensor_type, topic_items[1])
        if (topic_items[2] == 'Measure' or topic_items[2] == 'Monitoring'):
            measures = json.loads(payload)
            for key, value in measures.items(): # for Measure topic, key takes values 'TS' then 'CO2', 'Temp', 'H2O'
                if (key == 'TS'): # we assume that time stamp is the 1st item in payload
                    time_stamp = datetime.fromtimestamp(value) # we use original time stamp from the sensor that buffers samples before publish
                else:
                    row = getChannel(conn, sensor_type, key) # we get the channel id from DB init
                    if (not row is None): # only save known channels, discard unknown (quietly ?)
                        insertMeasure(conn, sensor_id, row[0], time_stamp, value) 
        if (topic_items[2] == 'Connect' and sensor_id in sensor_ids):
            publishTime()
        if not sensor_id in sensor_ids:
            publishTime()
            sensor_ids.append(sensor_id)
        conn.close()
except:
    print ('Unable to connect to mqtt broker ',app.config['MQTT_BROKER_URL'])

#handle firmware OTA
basedir = os.path.abspath(os.path.dirname(__file__))

@app.route('/check_fota', methods=['GET'])
def check_fota():
    """
    returns the current firmware version
    """
    with open(os.path.join(basedir, 'static/fota/version.json'), 'r') as f:
        firmware = json.loads(f.read())
    print(firmware)
    return jsonify(firmware)

@app.route('/fota', methods=['GET'])
def fota():
    """
    Serves the binary firmware.
    """
    with open(os.path.join(basedir, 'static/fota/firmware.bin'), 'rb') as bites:
#TODO: We may add a signature to the firmware binary to ensure it comes from the right source.
        return send_file(
                     io.BytesIO(bites.read()),
                     attachment_filename='firmware.bin',
                     mimetype='application/octet-stream'
               )
#TODO: Fix crash in case of missing file for some reason

#handle WEB access
@app.route('/raw')
def raw():
    """
    Displays last 100 measures in a table for all sensors.
    """
    conn = get_db_connection()
    measures = getLastMeasures(conn)
    conn.close()
    return render_template('raw.html', measures=measures)

@app.route('/raw/<serial_number>')
def raw_sensor(serial_number):
    """
    Displays last 100 measures in a table for a specific sensor.
    """
    conn = get_db_connection()
    measures = getLastSensorMeasures(conn, sensor_type, serial_number)
    conn.close()
    return render_template('raw_sensor.html', measures=measures, serial_number=serial_number)

@app.route('/graph/<serial_number>')
def graph_sensor(serial_number):
    """
    Displays last 1000 measures in a table for a specific sensor.
    """
    print ('graph_sensor (',serial_number,')')
    conn = get_db_connection()
    cur = conn.cursor()
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (not sensor_row is None):
        datasets = []
        rows = getChannels(conn, sensor_type)
        for channel_row in rows:
            datasets.append(formatGraph(getChannelGraphData(cur, sensor_row[0], channel_row[0]), channel_row[2], channel_row[3]))
    cur.close()
    conn.close()
    return render_template('graph_sensor.html', serial_number=serial_number, datasets=datasets)

@app.route('/raw/<serial_number>/<channel>')
def raw_channel(serial_number, channel):
    """
    Displays last 100 measures in a table for a specific sensor and a specific channel.
    """
    print ('raw_channel (',serial_number,',',channel,')')
    conn = get_db_connection()
    measures, unit = getLastChannelMeasures(conn, sensor_type, serial_number, channel)
    conn.close()
    return render_template('raw_channel.html', measures=measures, serial_number=serial_number, channel=channel, unit=unit)

@app.route('/graph/<serial_number>/<channel>')
def graph_channel(serial_number, channel):
    """
    Displays last 1000 measures in a graph for a specific sensor and a specific channel.
    """
    print ('graph_channel (',serial_number,',',channel,')')
    conn = get_db_connection()
    cur = conn.cursor()
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (not sensor_row is None):
        datasets = []
        channel_row = getChannel(conn, sensor_type, channel)
        if (not channel_row is None):
            datasets.append(formatGraph(getChannelGraphData(cur, sensor_row[0], channel_row[0]), channel_row[2], channel_row[3]))
    cur.close()
    conn.close()
    return render_template('graph_sensor.html', serial_number=serial_number, datasets=datasets)

def formatGraph(measures, name, unit):
    return dict (
        name = name,
        unit = unit,
        data = '[' + ",".join(['{x: '+f"'{str(m[0])}'"+',y:'+str(m[1])+'}' for m in reversed(measures)]) + ']')

def publishTime():
    with app.app_context():
        payload = json.dumps({"now": datetime.now().timestamp()}) # number of seconds since epoch, with milliseconds
        print ("publish SetTime ", payload)
        mqtt.publish('AirQuality/000000000000/SetTime', payload)


# postgres DB access routines for AirQuality project.
# Implementation note: This should probably be implemented in an object orient way rather than this procedural approach.

import psycopg2 

def get_db_connection():
    """
    returns a connection to postgres DB
    Note that all credentials are hardcoded. OK for proto, not production.
    
    :return: DB connection
    """
    conn = psycopg2.connect(
# TODO: replace hardcoded credentials by something else.
        host="localhost",
        database="AirQuality",
        user='postgres', #os.environ['PGLogin'],
        password='admin' #os.environ['PGPassword']
        )
    return conn

def getLastMeasures(conn):
    """
    get last 100 measures 
    
    :param conn: DB connection
    :return: array of array [sample time, sensor serial_number, sensor name name, recorded value, value's unit]
    """
    cur = conn.cursor()
    cur.execute('SELECT time, serial_number, name, value, unit FROM measures ' +
                'INNER JOIN channels ON measures.channel = channels.id ' +
                'INNER JOIN sensors ON measures.sensor = sensors.id ' +
                'ORDER BY time DESC LIMIT 100;')
    measures = cur.fetchall()
    cur.close()
    return measures

def getLastSensorMeasures(conn, sensor_type, serial_number):
    """
    get last 100 measures for a given sensor
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param serial_number: sensor serial number as a string
    :return: array of array [sample time, recorded value, value's unit]
    """
    cur = conn.cursor()
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (not sensor_row is None):
        cur.execute('SELECT time, name, value, unit FROM measures ' +
                    'INNER JOIN channels ON measures.channel = channels.id ' +
                    'WHERE measures.sensor = %s ORDER BY time DESC LIMIT 100;',
                    (sensor_row[0],))
        measures = cur.fetchall()
    cur.close()
    return measures

def getLastChannelMeasures(conn, sensor_type, serial_number, channel):
    """
    get last 100 measures for a specific channel of a given sensor
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param serial_number: sensor serial number as a string
    :param channel: sensor channel as a string
    :return: array of array [sample time, recorded value, value's unit]
    """
    cur = conn.cursor()
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (not sensor_row is None):
        channel_row = getChannel(conn, sensor_type, channel)
        if (not channel_row is None):
            unit = channel_row[3]
            cur.execute('SELECT time, value FROM measures WHERE sensor = %s AND channel = %s ORDER BY time DESC LIMIT 100;',
                        (sensor_row[0], channel_row[0]))
            measures = cur.fetchall()
    cur.close()
    return measures, unit

def getOrInsertSensor(conn, sensor_type, serial_number):
    """
    get sensor Id or insert a new sensor in DB
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param serial_number: sensor serial number as a string
    :return: sensor id
    """
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (sensor_row is None):
# TODO: if the sensor is unknown, it means it was not white listed by the commissioning precedure, if any.
# As there is no commissioning procedure for the time being, we decide to record its data but we flag the 
# sensor as rogue. Once the commissioning procedure is in place, we needhave to decide what to do (alert, record attack...) 
        sensor_id = insertSensor(conn, sensor_type, serial_number, "rogue")
    else:
        sensor_id = sensor_row[0]
    return sensor_id

def getSensor(conn, sensor_type, serial_number):
    """
    get sensor Id from its serial number
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param serial_number: sensor serial number as a string
    :return: sensor id
    """
    cur = conn.cursor()
    cur.execute('SELECT id FROM sensors WHERE serial_number = %s AND sensor_type = %s',
                (serial_number,sensor_type))
    row = cur.fetchone()
    cur.close()
    return row

def insertSensor(conn, sensor_type, serial_number, mcu_mac):
    """
    insert new sensor in DB
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param serial_number: sensor serial number as a string
    :param mcu_mac: unique identifier of the mcu associated to this sensor
    :return: sensor id
    """
    cur = conn.cursor()
    cur.execute('INSERT INTO sensors (serial_number, sensor_type, mcu_mac) VALUES (%s, %s, %s) RETURNING id',
                (serial_number,sensor_type, mcu_mac))
    id = cur.fetchone()[0]
    conn.commit()
    cur.close()
    return id

def getChannel(conn, sensor_type, name):
    """
    get sensor's channel meta data from channel name
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :param name: channel namer as a string
    :return: array of [channel id, sensor type, channel name, channel unit, channel description]
    """
    cur = conn.cursor()
    cur.execute('SELECT * FROM channels WHERE name = %s AND sensor_type = %s',
                (name,sensor_type))
    row = cur.fetchone()
    cur.close()
    return row

def getChannels(conn, sensor_type):
    """
    get sensor's channel's meta data from sensor type
    
    :param conn: DB connection
    :param sensor_type: sensor type as a string
    :return: array of array of [channel id, sensor type, channel name, channel unit, channel description]
    """
    cur = conn.cursor()
    cur.execute('SELECT * FROM channels WHERE sensor_type = %s', (sensor_type,))
    rows = cur.fetchall()
    cur.close()
    return rows

def getChannelGraphData(cur, sensor_id, channel_id):
    """
    get last 1000 measures for a specific channeel of a specific sensor
    
    :param conn: DB connection
    :param sensor_id: sensor id as int
    :param channel_id: channel id as int
    :return: array of array of [sample time stamp, sample value]
    """
    cur.execute('SELECT time, value FROM measures ' +
                'WHERE sensor = %s AND channel = %s ORDER BY time DESC LIMIT 1000;',
                (sensor_id, channel_id))
    rows = cur.fetchall()
    return rows


def insertMeasure(conn, sensor_id, channel_id, time_stamp, value):
    """
    insert a new measure sample for a specific sensor id and channel
    
    :param conn: DB connection
    :param sensor_id: sensor id as int
    :param channel_id: channel id as int
    :param time_stamp: sample time stamp as float (number of seconds since 1st of january 1970)
    :param value: sample value as float
    """
    cur = conn.cursor()
    cur.execute('INSERT INTO measures (sensor, channel, time, value)'
                'VALUES (%s, %s, %s, %s)',
                (sensor_id, channel_id, time_stamp, value))
    conn.commit()
    cur.close()

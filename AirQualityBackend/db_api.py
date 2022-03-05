import psycopg2 # postgres

def get_db_connection():
    conn = psycopg2.connect(
        host="localhost",
        database="AirQuality",
        user='postgres', #os.environ['PGLogin'],
        password='admin' #os.environ['PGPassword']
        )
    return conn

def getLastMeasures(conn):
    cur = conn.cursor()
    cur.execute('SELECT time, serial_number, name, value, unit FROM measures ' +
                'INNER JOIN channels ON measures.channel = channels.id ' +
                'INNER JOIN sensors ON measures.sensor = sensors.id ' +
                'ORDER BY time DESC LIMIT 100;')
    measures = cur.fetchall()
    cur.close()
    return measures

def getLastSensorMeasures(conn, sensor_type, serial_number):
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
    sensor_row = getSensor(conn, sensor_type, serial_number)
    if (sensor_row is None):
        sensor_id = insertSensor(conn, sensor_type, serial_number, "rogue")
    else:
        sensor_id = sensor_row[0]
    return sensor_id

def getSensor(conn, sensor_type, serial_number):
    cur = conn.cursor()
    cur.execute('SELECT id FROM sensors WHERE serial_number = %s AND sensor_type = %s',
                (serial_number,sensor_type))
    row = cur.fetchone()
    cur.close()
    return row

def getChannel(conn, sensor_type, name):
    cur = conn.cursor()
    cur.execute('SELECT * FROM channels WHERE name = %s AND sensor_type = %s',
                (name,sensor_type))
    row = cur.fetchone()
    cur.close()
    return row

def getChannels(conn, sensor_type):
    cur = conn.cursor()
    cur.execute('SELECT * FROM channels WHERE sensor_type = %s', (sensor_type,))
    rows = cur.fetchall()
    cur.close()
    return rows

def getChannelGraphData(cur, sensor_id, channel_id):
    cur.execute('SELECT time, value FROM measures ' +
                'WHERE sensor = %s AND channel = %s ORDER BY time DESC LIMIT 1000;',
                (sensor_id, channel_id))
    rows = cur.fetchall()
    return rows

def insertSensor(conn, sensor_type, serial_number, mcu_mac):
    cur = conn.cursor()
    cur.execute('INSERT INTO sensors (serial_number, sensor_type, mcu_mac) VALUES (%s, %s, %s) RETURNING id',
                (serial_number,sensor_type, mcu_mac))
    id = cur.fetchone()[0]
    conn.commit()
    cur.close()
    return id

def insertMeasure(conn, sensor_id, channel_id, time_stamp, value):
    cur = conn.cursor()
    cur.execute('INSERT INTO measures (sensor, channel, time, value)'
                'VALUES (%s, %s, %s, %s)',
                (sensor_id, channel_id, time_stamp, value))
    conn.commit()
    cur.close()

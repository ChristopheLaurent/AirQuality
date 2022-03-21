# Create AirQuality project tables
# It assumes that AirQuality database itself is already created (using pgAdmin for example)

import psycopg2
from db_api import *
import datetime

conn = conn = get_db_connection()

cur = conn.cursor()

# Creates new tables
cur.execute('DROP TABLE IF EXISTS locations CASCADE;')
cur.execute('CREATE TABLE locations (id serial PRIMARY KEY,'
                                 'building varchar (15) NOT NULL,'
                                 'floor integer NOT NULL,'
                                 'room varchar (30),'
                                 'comment text);'
                                 )

cur.execute('DROP TABLE IF EXISTS sensor_types CASCADE;')
cur.execute('CREATE TABLE sensor_types (sensor_type varchar (12) PRIMARY KEY,'
                                'description TEXT);'
                                )

cur.execute('DROP TABLE IF EXISTS channels CASCADE;')
cur.execute('CREATE TABLE channels (id serial PRIMARY KEY,'
                                'sensor_type varchar (12) REFERENCES sensor_types (sensor_type),'
                                'name varchar (12) NOT NULL,'
                                'unit varchar (12),'
                                'description TEXT);'
                                )

cur.execute('DROP TABLE IF EXISTS sensors CASCADE;')
cur.execute('CREATE TABLE sensors (id serial PRIMARY KEY,'
                                 'sensor_type varchar (12) REFERENCES sensor_types (sensor_type),'
                                 'serial_number varchar (12) NOT NULL,'
                                 'mcu_mac varchar (12) NOT NULL);'
                                 )

cur.execute('DROP TABLE IF EXISTS periods;')
cur.execute('CREATE TABLE periods (id serial PRIMARY KEY,'
                                 'location integer REFERENCES locations (id),'
                                 'sensor integer REFERENCES sensors (id),'
                                 'start_time date,'
                                 'end_time date);'
                                 )

cur.execute('DROP TABLE IF EXISTS measures;')
cur.execute('CREATE TABLE measures (sensor integer REFERENCES sensors (id),'
                                 'channel integer REFERENCES channels(id),'
                                 'time TIMESTAMP NOT NULL,'
                                 'value float);'
                                 )

# populate tables
cur.execute('INSERT INTO sensor_types (sensor_type, description)'
            'VALUES (%s, %s)',
            ('SCD41', 'CO2 Sensor from Sensirion'))
cur.execute('INSERT INTO channels (sensor_type, name, unit, description)'
            'VALUES (%s, %s, %s, %s)',
            ('SCD41', 'CO2', 'ppm', 'CO2 concentration'))
cur.execute('INSERT INTO channels (sensor_type, name, unit, description)'
            'VALUES (%s, %s, %s, %s)',
            ('SCD41', 'Temp', 'degC', 'Temperature'))
cur.execute('INSERT INTO channels (sensor_type, name, unit, description)'
            'VALUES (%s, %s, %s, %s)',
            ('SCD41', 'H2O', '%', 'Humidity'))
cur.execute('INSERT INTO channels (sensor_type, name, unit, description)'
            'VALUES (%s, %s, %s, %s)',
            ('SCD41', 'Voltage', 'V', 'Battery voltage'))
cur.execute('INSERT INTO channels (sensor_type, name, unit, description)'
            'VALUES (%s, %s, %s, %s)',
            ('SCD41', 'Load', '%', 'Battery load estimation'))


conn.commit()

cur.close()
conn.close()
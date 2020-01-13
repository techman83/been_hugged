#!/usr/bin/env python3
import os
import paho.mqtt.client as mqtt
from time import sleep

INTERVAL = int(os.environ.get('INTERVAL',5))

def main():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.username_pw_set(os.environ.get('MQTT_USER'), os.environ.get('MQTT_PASS'))
    client.connect(os.environ.get('MQTT_HOST','mosquitto'), int(os.environ.get('MQTT_PORT', 1883)), 60)

    while True:
        client.publish('/heartbeat', 'ping')
        sleep(INTERVAL)

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

if __name__ == '__main__':
    main()

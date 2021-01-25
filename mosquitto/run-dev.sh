#!/bin/bash
touch /etc/mosquitto/pwfile
mosquitto_passwd -b /etc/mosquitto/pwfile $MQTT_USER $MQTT_PASS 
mosquitto -c /etc/mosquitto/mosquitto.conf

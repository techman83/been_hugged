#!/bin/bash
touch /etc/mosquitto/pwfile
mosquitto_passwd -b /etc/mosquitto/pwfile $mqtt_user $mqtt_pass 
mosquitto -c /etc/mosquitto/mosquitto.conf

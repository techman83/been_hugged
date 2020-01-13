#!/bin/sh

echo Provsioning MQTT
touch /etc/mosquitto/pwfile
mosquitto_passwd -b /etc/mosquitto/pwfile $mqtt_user $mqtt_pass
mosquitto_passwd -b /etc/mosquitto/pwfile $mqtt_badge_user $mqtt_badge_pass
mosquitto -c /etc/mosquitto/mosquitto.conf

#!/bin/bash

echo Retrieving Secrets
while IFS== read -r key value; do
        export "$key"="$value"
done < <(aws secretsmanager get-secret-value --secret-id been_hugged --region ap-southeast-2|jq --raw-output .SecretString|jq -r '.|to_entries[]| [.key,(.value)] | join("=")')

echo Provsioning MQTT
touch /etc/mosquitto/pwfile
mosquitto_passwd -b /etc/mosquitto/pwfile $mqtt_user $mqtt_pass
mosquitto_passwd -b /etc/mosquitto/pwfile $mqtt_badge_user $mqtt_badge_pass
mosquitto -c /etc/mosquitto/mosquitto.conf

#!/bin/bash

while IFS== read -r key value; do
	export "$key"="$value"
done < <(aws secretsmanager get-secret-value --secret-id been_hugged --region ap-southeast-2|jq --raw-output .SecretString|jq -r '.|to_entries[]| [.key,(.value)] | join("=")')

heartbeat.py

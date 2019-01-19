#!/bin/bash

while IFS== read -r key value; do
	        export "$key"="$value"
	done < <(aws secretsmanager get-secret-value --secret-id been_hugged|jq --raw-output .SecretString|jq -r '.|to_entries[]| [.key,(.value|@sh)] | join("=")')

beenhugged.py

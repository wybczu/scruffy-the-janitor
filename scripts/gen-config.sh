#!/bin/bash

set -e

read -p "WiFi SSID: " WIFI_SSID
read -p "WiFi Password [${WIFI_SSID}]: " WIFI_PASS
read -p "Slack API token: " SLACK_API_TOKEN
read -p "Slack Channel: " SLACK_API_CHANNEL

cat <<EOF
#define WIFI_SSID "${WIFI_SSID}"
#define WIFI_PASS "${WIFI_PASS}"

#define SLACK_API_TOKEN "${SLACK_API_TOKEN}"
#define SLACK_API_CHANNEL "${SLACK_API_CHANNEL}"
EOF

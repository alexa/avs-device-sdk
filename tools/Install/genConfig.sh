#!/usr/bin/env bash

#
# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#  http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#

LOCALE=${LOCALE:-'en-US'}

if [ $# -ne 5 ]
then
    echo  'Usage: genConfig.sh <config.json file> <device_serial_number> <db path> <SDK Source Directory> <output AlexaClientSDKConfig.json file>'
    echo  '1) <config.json file> can be downloaded from developer portal and must contain the following:'
    echo  '   "clientId": "<OAuth client ID>"'
    echo  '   "productId": "<your product name for device>"'
    echo  '2) <device_serial_number> specifies the deviceInfo deviceSerialNumber in the output json file.'
    echo  '3) <db path> specifies the path to where the databases will be located.'
    echo  '4) <SDK Source Directory> specifies the root directory to where the avs-device-sdk source code is located.'
    echo  '5) <output AlexaClientSDKConfig.json file> output file'
  exit 1
fi

CONFIG_JSON_FILE=$1
if [ ! -f "$CONFIG_JSON_FILE" ]; then
    echo "Config json file not found!"
    exit 2
fi

DEVICE_SERIAL_NUMBER=$2
if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]; then
   echo 'Device serial number is invalid!'
   exit 2
fi

CONFIG_DB_PATH=$3

SDK_SRC_PATH=$4
if [ ! -d "$SDK_SRC_PATH" ]; then
    echo 'Alexa Device Source directory not found!'
    exit 2
fi
INPUT_CONFIG_FILE="$SDK_SRC_PATH/Integration/AlexaClientSDKConfig.json"
if [ ! -f "$INPUT_CONFIG_FILE" ]; then
    echo "AlexaClientSDKConfig.json file not found in source directory!"
    exit 2
fi

OUTPUT_CONFIG_FILE=$5
# Check if output file exists, if yes, create empty file.
if [ -f $OUTPUT_CONFIG_FILE ]; then
  echo -n "" > $OUTPUT_CONFIG_FILE
fi

# Convert to Windows path format if using MingGW
uname_str=$(uname -a)
if [[ "$uname_str" ==  "MINGW64"* ]]; then
  INPUT_CONFIG_FILE=$(cygpath.exe -m ${INPUT_CONFIG_FILE})
  OUTPUT_CONFIG_FILE=$(cygpath.exe -m ${OUTPUT_CONFIG_FILE})
fi

# Read config.json file
#-------------------------------------------------------
# Read key value from config.json file
#-------------------------------------------------------
# Arguments are: json_key
read_config_json()
{
python << EOF
import json
value = ""
with open("${CONFIG_JSON_FILE}", "r") as f:
    dict = json.load(f)
    if "deviceInfo" in dict and "${1}" in dict["deviceInfo"]:
        value = dict["deviceInfo"]["${1}"]
    print(value)
EOF
}

CLIENT_ID=$(read_config_json "clientId")
if [[ ! "$CLIENT_ID" =~ amzn1\.application-oa2-client\.[0-9a-z]{32} ]]
then
   echo 'client ID is invalid!'
   exit 2
fi

PRODUCT_ID=$(read_config_json "productId")
if [[ ! "$PRODUCT_ID" =~ [0-9a-zA-Z_]+ ]]
then
   echo 'product ID is invalid!'
   exit 2
fi

########################################################################################################################
# Start of setting variables for generating $OUTPUT_CONFIG_FILE.
# All variables that needs to be substituted must be defined below "set -a" line!
########################################################################################################################
set -a

# Set variables for configuration file
# Variables for cblAuthDelegate
SDK_CBL_AUTH_DELEGATE_DATABASE_FILE_PATH=$CONFIG_DB_PATH/cblAuthDelegate.db

# Variables for deviceInfo
SDK_CONFIG_DEVICE_SERIAL_NUMBER=$DEVICE_SERIAL_NUMBER
SDK_CONFIG_CLIENT_ID=$CLIENT_ID
SDK_CONFIG_PRODUCT_ID=$PRODUCT_ID

# Variables for miscDatabase
SDK_MISC_DATABASE_FILE_PATH=$CONFIG_DB_PATH/miscDatabase.db

# Variables for alertsCapabilityAgent
SDK_SQLITE_DATABASE_FILE_PATH=$CONFIG_DB_PATH/alerts.db

# Variables for settings
SDK_SQLITE_SETTINGS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/settings.db
SETTING_LOCALE_VALUE=$LOCALE

# Variables for device settings
SDK_SQLITE_DEVICE_SETTINGS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/deviceSettings.db

# Variables for bluetooth
SDK_BLUETOOTH_DATABASE_FILE_PATH=$CONFIG_DB_PATH/bluetooth.db

# Variables for certifiedSender
SDK_CERTIFIED_SENDER_DATABASE_FILE_PATH=$CONFIG_DB_PATH/certifiedSender.db

# Variables for notifications
SDK_NOTIFICATIONS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/notifications.db

########################################################################################################################
# End of setting variables for generating $OUTPUT_CONFIG_FILE
# All variables that needs to be substituted must be defined above "set +a" line!
########################################################################################################################
set +a

# Use python template substitute to generate $OUTPUT_CONFIG_FILE
python << EOF
import os
from string import Template
with open("${INPUT_CONFIG_FILE}", "r") as f, open("${OUTPUT_CONFIG_FILE}", "w") as o:
    o.write(Template(f.read()).safe_substitute(os.environ))
EOF

echo 'Completed generation of config file:' ${OUTPUT_CONFIG_FILE}

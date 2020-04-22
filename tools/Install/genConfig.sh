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

set -o errexit  # Exit the script if any statement fails.
set -o nounset  # Exit the script if any uninitialized variable is used.

LOCALE=${LOCALE:-'en-US'}

#---- Indices used to save the extra configuration variables.
# The manufacturer name.
INDEX_MANUFACTURER_NAME=0
# The device description.
INDEX_DEVICE_DESCRIPTION=1

function printUsageAndExit() {
    echo  'Usage: genConfig.sh <config.json file> <device_serial_number> <db path> <SDK Source Directory>'
    echo '<output AlexaClientSDKConfig.json file> [-D<variable>=<value>]+'
    echo  '1) <config.json file> can be downloaded from developer portal and must contain the following:'
    echo  '   "clientId": "<OAuth client ID>"'
    echo  '   "productId": "<your product name for device>"'
    echo  '2) <device_serial_number> specifies the deviceInfo deviceSerialNumber in the output json file.'
    echo  '3) <db path> specifies the path to where the databases will be located.'
    echo  '4) <SDK Source Directory> specifies the root directory to where the avs-device-sdk source code is located.'
    echo  '5) <output AlexaClientSDKConfig.json file> output file'
    echo  '6) Configure different variables that can be used to fill up extra information in the final configuration.' \
    'Avaiable variables are:'
    echo  '   "SDK_CONFIG_MANUFACTURER_NAME": The name of the device manufacturer. This variable is required.'
    echo  '   "SDK_CONFIG_DEVICE_DESCRIPTION": The description of the device. This variable is required.'
    exit 1
}

if [ $# -lt 7 ]
then
  echo "[ERROR] Missing parameters!"
  printUsageAndExit
fi

CONFIG_JSON_FILE=$1
if [ ! -f "$CONFIG_JSON_FILE" ]; then
    echo "[ERROR] Config json file not found!"
    printUsageAndExit
fi

DEVICE_SERIAL_NUMBER=$2
if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]; then
   echo '[ERROR] Device serial number is invalid!'
   printUsageAndExit
fi

CONFIG_DB_PATH=$3

SDK_SRC_PATH=$4
if [ ! -d "$SDK_SRC_PATH" ]; then
    echo '[ERROR] Alexa Device Source directory not found!'
    printUsageAndExit
fi
INPUT_CONFIG_FILE="$SDK_SRC_PATH/Integration/AlexaClientSDKConfig.json"
if [ ! -f "$INPUT_CONFIG_FILE" ]; then
    echo "[ERROR] AlexaClientSDKConfig.json file not found in source directory!"
    printUsageAndExit
fi

OUTPUT_CONFIG_FILE=$5
# Check if output file exists, if yes, create empty file.
if [ -f $OUTPUT_CONFIG_FILE ]; then
  echo -n "" > $OUTPUT_CONFIG_FILE
fi

shift 5
declare -a extra_variables
for variable in "$@"; do
  if [[ "${variable:0:2}" != "-D" ]]; then
    echo "[ERROR] Wrong variable declaration format ($variable). Declaration should start with '-D'"
    printUsageAndExit
  fi

  pair=${variable:2}
  variable_name=${pair%=*}
  variable_value=${pair#*=}
  case "$variable_name" in
    SDK_CONFIG_DEVICE_DESCRIPTION )
      extra_variables[${INDEX_DEVICE_DESCRIPTION}]=${variable_value}
      ;;
    SDK_CONFIG_MANUFACTURER_NAME )
      extra_variables[${INDEX_MANUFACTURER_NAME}]=${variable_value}
      ;;
    * )
      echo "[ERROR] Unknown configuration variable ${variable_name}"
      printUsageAndExit
      ;;
    esac
done

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

# Variables for capabilitiesDelegate
SDK_CAPABILITIES_DELEGATE_DATABASE_FILE_PATH=$CONFIG_DB_PATH/capabilitiesDelegate.db

# Variables for alertsCapabilityAgent
SDK_SQLITE_DATABASE_FILE_PATH=$CONFIG_DB_PATH/alerts.db

# Variables for device settings
SDK_SQLITE_DEVICE_SETTINGS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/deviceSettings.db

# Variables for bluetooth
SDK_BLUETOOTH_DATABASE_FILE_PATH=$CONFIG_DB_PATH/bluetooth.db

# Variables for certifiedSender
SDK_CERTIFIED_SENDER_DATABASE_FILE_PATH=$CONFIG_DB_PATH/certifiedSender.db

# Variables for notifications
SDK_NOTIFICATIONS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/notifications.db

# Variables for device info
SDK_CONFIG_MANUFACTURER_NAME=${extra_variables[${INDEX_MANUFACTURER_NAME}]}
SDK_CONFIG_DEVICE_DESCRIPTION=${extra_variables[${INDEX_DEVICE_DESCRIPTION}]}

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

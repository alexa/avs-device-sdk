#!/bin/bash

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

PORT_AUDIO_FILE="pa_stable_v190600_20161030.tgz"
PORT_AUDIO_DOWNLOAD_URL="http://www.portaudio.com/archives/$PORT_AUDIO_FILE"

TEST_MODEL_DOWNLOAD="https://github.com/Sensory/alexa-rpi/blob/master/models/spot-alexa-rpi-31000.snsr"

BUILD_TESTS=${BUILD_TESTS:-'true'}

CURRENT_DIR="$(pwd)"
INSTALL_BASE=${INSTALL_BASE:-"$CURRENT_DIR"}
SOURCE_FOLDER=${SDK_LOC:-''}
THIRD_PARTY_FOLDER=${THIRD_PARTY_LOC:-'third-party'}
BUILD_FOLDER=${BUILD_FOLDER:-'build'}
SOUNDS_FOLDER=${SOUNDS_FOLDER:-'sounds'}
DB_FOLDER=${DB_FOLDER:-'db'}

SOURCE_PATH="$INSTALL_BASE/$SOURCE_FOLDER"
THIRD_PARTY_PATH="$INSTALL_BASE/$THIRD_PARTY_FOLDER"
BUILD_PATH="$INSTALL_BASE/$BUILD_FOLDER"
SOUNDS_PATH="$INSTALL_BASE/$SOUNDS_FOLDER"
DB_PATH="$INSTALL_BASE/$DB_FOLDER"
CONFIG_DB_PATH="$DB_PATH"
UNIT_TEST_MODEL_PATH="$INSTALL_BASE/avs-device-sdk/KWD/inputs/SensoryModels/"
UNIT_TEST_MODEL="$THIRD_PARTY_PATH/alexa-rpi/models/spot-alexa-rpi-31000.snsr"
INPUT_CONFIG_FILE="$SOURCE_PATH/avs-device-sdk/Integration/AlexaClientSDKConfig.json"
OUTPUT_CONFIG_FILE="$BUILD_PATH/Integration/AlexaClientSDKConfig.json"
TEMP_CONFIG_FILE="$BUILD_PATH/Integration/tmp_AlexaClientSDKConfig.json"
TEST_SCRIPT="$INSTALL_BASE/test.sh"
LIB_SUFFIX="a"

GSTREAMER_AUDIO_SINK="autoaudiosink"

build_port_audio() {
  # build port audio
  echo
  echo "==============> BUILDING PORT AUDIO =============="
  echo
  pushd $THIRD_PARTY_PATH
  wget -c $PORT_AUDIO_DOWNLOAD_URL
  tar zxf $PORT_AUDIO_FILE

  pushd portaudio
  ./configure --without-jack
  make
  popd
  popd
}

get_platform() {
  uname_str=`uname -a`
  result=""

  if [[ "$uname_str" ==  "Linux "* ]] && [[ -f /etc/os-release ]]
  then
    sys_id=`cat /etc/os-release | grep "^ID="`
    if [[ "$sys_id" == "ID=raspbian" ]]
    then
      echo "Raspberry pi"
    fi
  elif [[ "$uname_str" ==  "MINGW64"* ]]
  then
    echo "Windows mingw64"
  fi
}

if [ $# -eq 0 ]
then
  echo  'bash setup.sh <config-file>'
  echo  'the config file must contain the following:'
  echo  '   CLIENT_ID=<OAuth client ID>'
  echo  '   PRODUCT_ID=<your product name for device>'
  echo  '   DEVICE_SERIAL_NUMBER=<your device serial number>'

  exit 1
fi

source $1

# The target platform for the build.
PLATFORM=${PLATFORM:-$(get_platform)}

if [ "$PLATFORM" == "Raspberry pi" ]
then
  source pi.sh
elif [ "$PLATFORM" == "Windows mingw64" ]
then
  source mingw.sh
  else
    PLATFORM_LOWER=$(echo "${PLATFORM}" | tr '[:upper:]' '[:lower:]')
    if [ "$PLATFORM_LOWER" == "android" ]
    then
      PLATFORM="Android"
      source android.sh
    else
      echo "The installation script doesn't support current system. (System: $(uname -a))"
      exit 1
    fi
fi

if [[ ! "$CLIENT_ID" =~ amzn1\.application-oa2-client\.[0-9a-z]{32} ]]
then
  echo 'client ID is invalid!'
  exit 1
fi

if [[ ! "$PRODUCT_ID" =~ [0-9a-zA-Z_]+ ]]
then
  echo 'product ID is invalid!'
  echo $PRODUCT_ID
  exit 1
fi

if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]
then
  echo 'device serial number is invalid!'
  exit 1
fi

echo "################################################################################"
echo "################################################################################"
echo ""
echo ""
echo "AVS Device SDK $PLATFORM Script - Terms and Agreements"
echo ""
echo ""
echo "The AVS Device SDK is dependent on several third-party libraries, environments, "
echo "and/or other software packages that are installed using this script from "
echo "third-party sources (\"External Dependencies\"). These are terms and conditions "
echo "associated with the External Dependencies "
echo "(available at https://github.com/alexa/avs-device-sdk/wiki/Dependencies) that "
echo "you need to agree to abide by if you choose to install the External Dependencies."
echo ""
echo ""
echo "If you do not agree with every term and condition associated with the External "
echo "Dependencies, enter \"QUIT\" in the command line when prompted by the installer."
echo "Else enter \"AGREE\"."
echo ""
echo ""
echo "################################################################################"
echo "################################################################################"

read input
input=$(echo $input | awk '{print tolower($0)}')
if [ $input == 'quit' ]
then
  exit 1
elif [ $input == 'agree' ]
then
  echo "################################################################################"
  echo "Proceeding with installation"
  echo "################################################################################"
else
  echo "################################################################################"
  echo 'Unknown option'
  echo "################################################################################"
  exit 1
fi

if [ ! -d "$BUILD_PATH" ]
then

  # Make sure required packages are installed
  echo "==============> INSTALLING REQUIRED TOOLS AND PACKAGE ============"
  echo

  install_dependencies

  # create / paths
  echo
  echo "==============> CREATING PATHS AND GETTING SOUND FILES ============"
  echo

  mkdir -p $SOURCE_PATH
  mkdir -p $THIRD_PARTY_PATH
  mkdir -p $SOUNDS_PATH
  mkdir -p $DB_PATH

  run_os_specifics

  if [ ! -d "${SOURCE_PATH}/avs-device-sdk" ]
  then
    #get sdk
    echo
    echo "==============> CLONING SDK =============="
    echo

    cd $SOURCE_PATH
    git clone --single-branch git://github.com/alexa/avs-device-sdk.git
  fi

  # make the SDK
  echo
  echo "==============> BUILDING SDK =============="
  echo

  mkdir -p $BUILD_PATH
  cd $BUILD_PATH
  cmake "$SOURCE_PATH/avs-device-sdk" \
      -DCMAKE_BUILD_TYPE=DEBUG \
      "${CMAKE_PLATFORM_SPECIFIC[@]}"

  cd $BUILD_PATH
  make SampleApp -j2

else
  cd $BUILD_PATH
  make SampleApp -j2
fi

echo
echo "==============> SAVING CONFIGURATION FILE =============="
echo

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

# Variables for bluetooth
SDK_BLUETOOTH_DATABASE_FILE_PATH=$CONFIG_DB_PATH/bluetooth.db

# Variables for certifiedSender
SDK_CERTIFIED_SENDER_DATABASE_FILE_PATH=$CONFIG_DB_PATH/certifiedSender.db

# Variables for notifications
SDK_NOTIFICATIONS_DATABASE_FILE_PATH=$CONFIG_DB_PATH/notifications.db

# Create configuration file with audioSink configuration at the beginning of the file
cat << EOF > "$OUTPUT_CONFIG_FILE"
 {
    "gstreamerMediaPlayer":{
        "audioSink":"$GSTREAMER_AUDIO_SINK"
    },
EOF

# Check if temporary file exists
if [ -f $TEMP_CONFIG_FILE ]; then
  rm $TEMP_CONFIG_FILE
fi

# Create temporary configuration file with variables filled out
while IFS='' read -r line || [[ -n "$line" ]]; do
    while [[ "$line" =~ (\$\{[a-zA-Z_][a-zA-Z_0-9]*\}) ]]; do
        LHS=${BASH_REMATCH[1]}
        RHS="$(eval echo "\"$LHS\"")"
        line=${line//$LHS/$RHS}
    done
    echo "$line" >> $TEMP_CONFIG_FILE
done < $INPUT_CONFIG_FILE

# Delete first line from temp file to remove opening bracket
sed -i -e "1d" $TEMP_CONFIG_FILE

# Append temp file to configuration file
cat $TEMP_CONFIG_FILE >> $OUTPUT_CONFIG_FILE

# Delete temp file
rm $TEMP_CONFIG_FILE

echo
echo "==============> FINAL CONFIGURATION  =============="
echo
cat $OUTPUT_CONFIG_FILE

generate_start_script

cat << EOF > "$TEST_SCRIPT"
echo
echo "==============> BUILDING Tests =============="
echo
mkdir -p "$UNIT_TEST_MODEL_PATH"
cp "$UNIT_TEST_MODEL" "$UNIT_TEST_MODEL_PATH"
cd $BUILD_PATH
make all test -j2
chmod +x "$START_SCRIPT"
EOF

echo " **** Completed Configuration/Build ***"


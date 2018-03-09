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
CONFIG_FILE="$BUILD_PATH/Integration/AlexaClientSDKConfig.json"
START_AUTH_SCRIPT="$INSTALL_BASE/startauth.sh"
TEST_SCRIPT="$INSTALL_BASE/test.sh"
LIB_SUFFIX="a"

get_platform() {
  uname_str=`uname -a`

  if [[ "$uname_str" ==  "Linux raspberrypi"* ]]
  then
    result="pi"
  elif [[ "$uname_str" ==  "MINGW64"* ]]
  then
    result="mingw64"
  else
    result=""
  fi
}

get_platform
PLATFORM=$result

if [ "$PLATFORM" == "pi" ]
then
  source pi.sh
elif [ "$PLATFORM" == "mingw64" ]
then
  source mingw.sh
else
  echo "The installation script doesn't support current system. (System: $(uname -a))"
  exit 1
fi

echo "################################################################################"
echo "################################################################################"
echo ""
echo ""
echo "AVS Device SDK Raspberry Pi Script - Terms and Agreements"
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

if [ $# -eq 0 ]
then
  echo  'bash setup.sh <config-file>'
  echo  'the config file must contain the following:'
  echo  '   CLIENT_ID=<OAuth client ID>'
  echo  '   CLIENT_SECRET=<OAuth client secret>'  
  echo  '   PRODUCT_NAME=<your product name for device>'
  echo  '   DEVICE_SERIAL_NUMBER=<your device serial number>'

  exit 1
fi

source $1

set -e

if [[ ! "$CLIENT_ID" =~ amzn1\.application-oa2-client\.[0-9a-z]{32} ]]
then
   echo 'client ID is invalid!'
   exit 1
fi

if [[ ! "$CLIENT_SECRET" =~ [0-9a-f]{64} ]]
then 
   echo 'client secret is invalid!'
   exit 1
fi

if [[ ! "$PRODUCT_ID" =~ [0-9a-zA-Z_]+ ]]
then 
   echo 'product ID is invalid!'
   exit 1
fi

if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]
then 
   echo 'device serial number is invalid!'
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
    mkdir -p $BUILD_PATH
    mkdir -p $SOUNDS_PATH
    mkdir -p $DB_PATH

    # build port audio
    echo
    echo "==============> BUILDING PORT AUDIO =============="
    echo

    cd $THIRD_PARTY_PATH
    wget -c $PORT_AUDIO_DOWNLOAD_URL
    tar zxf $PORT_AUDIO_FILE

    cd portaudio
    ./configure --without-jack
    make

    run_os_specifics

    #get sdk 
    echo
    echo "==============> CLONING SDK =============="
    echo

    cd $SOURCE_PATH
    git clone git://github.com/alexa/avs-device-sdk.git

    # make the SDK
    echo
    echo "==============> BUILDING SDK =============="
    echo

    cd $BUILD_PATH
    cmake "$SOURCE_PATH/avs-device-sdk" \
    "${CMAKE_PLATFORM_SPECIFIC[@]}" \
    -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
    -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
    -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
    -DCMAKE_BUILD_TYPE=DEBUG

    cd $BUILD_PATH
    make SampleApp -j2

else
    cd $BUILD_PATH
    make SampleApp -j2
fi

echo
echo "==============> SAVING CONFIGURATION FILE =============="
echo

cat << EOF > "$CONFIG_FILE"
{
    "alertsCapabilityAgent":{
        "databaseFilePath":"$CONFIG_DB_PATH/alerts.db"
    },
    "certifiedSender":{
        "databaseFilePath":"$CONFIG_DB_PATH/certifiedSender.db"
    },
    "settings":{
        "databaseFilePath":"$CONFIG_DB_PATH/settings.db",
        "defaultAVSClientSettings":{
            "locale":"$LOCALE"
        }
    },
    "notifications":{
        "databaseFilePath":"$CONFIG_DB_PATH/notifications.db"
    },
    "authDelegate":{
      "clientId":"$CLIENT_ID",
        "clientSecret":"$CLIENT_SECRET",
        "deviceSerialNumber":"$DEVICE_SERIAL_NUMBER",
        "refreshToken":"",
        "productId":"$PRODUCT_ID"
    }
}
EOF

echo
echo "==============> FINAL CONFIGURATION  =============="
echo
cat $CONFIG_FILE

cat << EOF > "$START_AUTH_SCRIPT"
cd "$BUILD_PATH"
python AuthServer/AuthServer.py
EOF

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
chmod +x "$START_AUTH_SCRIPT"
EOF

echo " **** Completed Configuration/Build ***"


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
UNIT_TEST_MODEL_PATH="$INSTALL_BASE/avs-device-sdk/KWD/inputs/SensoryModels/"
UNIT_TEST_MODEL="$THIRD_PARTY_PATH/alexa-rpi/models/spot-alexa-rpi-31000.snsr"

WAKE_WORD_ENGINE="sensory" # "snowboy" For Sensory engine
SENSORY_PATH="$THIRD_PARTY_PATH/alexa-rpi"
SENSORY_MODELS_PATH="$SENSORY_PATH/models"
SNOWBOY_PATH="$THIRD_PARTY_PATH/snowboy"
SNOWBOY_MODELS_PATH="$SNOWBOY_PATH/resources/alexa/alexa-sdk-resources"
SNOWBOY_WAKEWORD="snowboy" # "alexa" for Alexa wake word

CONFIG_FILE="$BUILD_PATH/Integration/AlexaClientSDKConfig.json"
SOUND_CONFIG="$HOME/.asoundrc"
START_SCRIPT="$INSTALL_BASE/startsample.sh"
START_AUTH_SCRIPT="$INSTALL_BASE/startauth.sh"
TEST_SCRIPT="$INSTALL_BASE/test.sh"

if [ ! -d "$BUILD_PATH" ]
then

    echo "==============> INSTALLING REQUIRED TOOLS AND PACKAGE ============"
    echo

    sudo apt-get update
    sudo apt-get -y install git gcc cmake \
    build-essential \
    libsqlite3-dev \
    libcurl4-openssl-dev \
    libfaad-dev \
    libsoup2.4-dev \
    libgcrypt20-dev \
    libgstreamer-plugins-bad1.0-dev \
    gstreamer1.0-plugins-bad \
    mpg123 \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-good \
    libgstreamer-plugins-base0.10-0 libgstreamer-plugins-base0.10-dev  \
    gstreamer1.0-tools gstreamer1.0-alsa \
    libatlas-base-dev \
    libasound2-dev \
    sox \
    vim \
    python-pip

    sudo pip install flask
    sudo pip install commentjson
    sudo pip install requests 

    echo
    echo "==============> CREATING PATHS AND GETTING SOUND FILES ============"
    echo

    mkdir -p $SOURCE_PATH
    mkdir -p $THIRD_PARTY_PATH
    mkdir -p $BUILD_PATH
    mkdir -p $SOUNDS_PATH
    mkdir -p $DB_PATH

    echo
    echo "==============> BUILDING PORT AUDIO =============="
    echo

    cd $THIRD_PARTY_PATH
    wget -c $PORT_AUDIO_DOWNLOAD_URL
    tar zxf $PORT_AUDIO_FILE

    cd portaudio
    ./configure --without-jack
    make -j4

    echo
    echo "==============> CLONING SDK =============="
    echo

    cd $SOURCE_PATH
    git clone https://github.com/matrix-io/avs-device-sdk.git

    echo
    echo "==============> CLONING AND BUILDING SENSORY =============="
    echo

    if [ ! -d "$THIRD_PARTY_PATH/alexa-rpi" ]
    then
        cd $THIRD_PARTY_PATH
        git clone https://github.com/Sensory/alexa-rpi.git
        bash ./alexa-rpi/bin/license.sh
    fi
    
    echo
    echo "==============> CLONING AND BUILDING SNOWBOY =============="
    echo

    if [ ! -d "$THIRD_PARTY_PATH/snowboy" ]
    then
        cd $THIRD_PARTY_PATH
        git clone https://github.com/Kitt-AI/snowboy.git
        cd snowboy
        mkdir resources/alexa/alexa-sdk-resources
        cp ./resources/common.res ./resources/alexa/alexa-sdk-resources

        if  [ $SNOWBOY_WAKEWORD = "alexa" ]
        then
            cp ./resources/alexa/alexa_02092017.umdl ./resources/alexa/alexa-sdk-resources/alexa.umdl
        elif [ $SNOWBOY_WAKEWORD = "snowboy" ]
        then
            cp ./resources/models/snowboy.umdl ./resources/alexa/alexa-sdk-resources/alexa.umdl 
        else
            echo " ******* Wrong Wake Word Selected *********"
            exit 1
        fi
    fi

    echo
    echo "==============> BUILDING SDK =============="
    echo

    if [ $WAKE_WORD_ENGINE = "sensory" ]
    then
        cd $BUILD_PATH
        cmake "$SOURCE_PATH/avs-device-sdk" \
        -DSENSORY_KEY_WORD_DETECTOR=ON \
        -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH="$THIRD_PARTY_PATH/alexa-rpi/lib/libsnsr.a" \
        -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR="$THIRD_PARTY_PATH/alexa-rpi/include" \
        -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
        -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.a" \
        -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
        -DACSDK_EMIT_SENSITIVE_LOGS=ON \
        -DCMAKE_BUILD_TYPE=DEBUG

    elif [ $WAKE_WORD_ENGINE = "snowboy" ]
    then
        cd $BUILD_PATH
        cmake "/home/pi/avs-device-sdk" \
        -DKITTAI_KEY_WORD_DETECTOR=ON \
        -DKITTAI_KEY_WORD_DETECTOR_LIB_PATH="$THIRD_PARTY_PATH/snowboy/lib/rpi/libsnowboy-detect.a" \
        -DKITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR="$THIRD_PARTY_PATH/snowboy/include" \
        -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
        -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.a" \
        -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
        -DACSDK_EMIT_SENSITIVE_LOGS=ON \
        -DCMAKE_BUILD_TYPE=DEBUG

    else
        echo 
        echo "Please select a WakeWord Engine."
        echo 
        exit 1
    fi

    cd $BUILD_PATH
    make SampleApp -j4

else
    cd $BUILD_PATH
    make SampleApp -j4
fi

echo
echo "==============> Saving Configuration File =============="
echo

cat << EOF > "$CONFIG_FILE"
{
    "alertsCapabilityAgent":{
        "databaseFilePath":"$DB_PATH/alerts.db"
    },
    "certifiedSender":{
        "databaseFilePath":"$DB_PATH/certifiedSender.db"
    },
    "settings":{
        "databaseFilePath":"$DB_PATH/settings.db",
        "defaultAVSClientSettings":{
            "locale":"$LOCALE"
        }
    },
    "notifications":{
        "databaseFilePath":"$DB_PATH/notifications.db"
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
echo "==============> Final Configuration  =============="
echo
cat $CONFIG_FILE

echo
echo "==============> Backup /etc/asound.conf =============="
echo

if [ -f "/etc/asound.conf" ]
then
  sudo mv -v /etc/asound.conf /etc/asound.conf.backup
fi

echo
echo "==============> Saving Audio Configuration File =============="
echo

cat << EOF > "$SOUND_CONFIG"
pcm.sc {
    type hw
    card 2
}
pcm.!default
{
  type asym
  playback.pcm {
    type hw
    card 0
    device 0
  }
  capture.pcm {
    type hw
    card 2
    device 0
  }
}
EOF

echo
echo "==============> Create StartSampleApp Script =============="
echo


if [ $WAKE_WORD_ENGINE = "sensory"  ]
then

cat << EOF > "$START_SCRIPT"
cd "$BUILD_PATH/SampleApp/src"
./SampleApp "$CONFIG_FILE" "$SENSORY_MODELS_PATH" DEBUG9
EOF

elif [ $WAKE_WORD_ENGINE = "snowboy"  ]
then

cat << EOF > "$START_SCRIPT"
cd "$BUILD_PATH/SampleApp/src"
./SampleApp "$CONFIG_FILE" "$SNOWBOY_MODELS_PATH" DEBUG9
EOF

else
    echo 
    echo "Please select a WakeWord Engine."
    echo 
    exit 1
fi

echo
echo "==============> Create Auth Script =============="
echo

cat << EOF > "$START_AUTH_SCRIPT"
cd "$BUILD_PATH"
python AuthServer/AuthServer.py
EOF

echo " **** Completed Configuration/Build ***"

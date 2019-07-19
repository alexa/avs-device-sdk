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

CLONE_URL=${CLONE_URL:- 'git://github.com/alexa/avs-device-sdk.git'}

PORT_AUDIO_FILE="pa_stable_v190600_20161030.tgz"
PORT_AUDIO_DOWNLOAD_URL="http://www.portaudio.com/archives/$PORT_AUDIO_FILE"


BUILD_TESTS=${BUILD_TESTS:-'true'}

pushd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null
CURRENT_DIR="$( pwd )"
THIS_SCRIPT="$CURRENT_DIR/setup.sh"
popd > /dev/null

INSTALL_BASE=${INSTALL_BASE:-"$HOME/sdk-folder"}
SOURCE_FOLDER=${SDK_LOC:-''}
THIRD_PARTY_FOLDER=${THIRD_PARTY_LOC:-'third-party'}
BUILD_FOLDER=${BUILD_FOLDER:-'sdk-build'}
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
ANDROID_CONFIG_FILE=""

# Default device serial number if nothing is specified
DEVICE_SERIAL_NUMBER="123456"

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

PI_HAT_CTRL_PATH="$THIRD_PARTY_PATH/pi_hat_ctrl"

SENSORY_MODEL_HASH=5d811d92fb89043f4a4a7b7d0d26d7c3c83899b0
ALIASES=$HOME/.bash_aliases


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

show_help() {
  echo  'Usage: setup.sh <config-json-file> [OPTIONS]'
  echo  'The <config-json-file> can be downloaded from developer portal and must contain the following:'
  echo  '   "clientId": "<OAuth client ID>"'
  echo  '   "productId": "<your product name for device>"'
  echo  ''
  echo  'Optional parameters'
  echo  '  -s <serial-number>  If nothing is provided, the default device serial number is 123456'
  echo  '  -a <file-name>      The file that contains Android installation configurations (e.g. androidConfig.txt)'
  echo  '  -h                  Display this help and exit'
}

if [[ $# -lt 1 ]]; then
    show_help
    exit 1
fi

CONFIG_JSON_FILE=$1
if [ ! -f "$CONFIG_JSON_FILE" ]; then
    echo "Config json file not found!"
    show_help
    exit 1
fi
shift 1

OPTIONS=s:a:h
while getopts "$OPTIONS" opt ; do
    case $opt in
        s )
            DEVICE_SERIAL_NUMBER="$OPTARG"
            ;;
        a )
            ANDROID_CONFIG_FILE="$OPTARG"
            if [ ! -f "$ANDROID_CONFIG_FILE" ]; then
                echo "Android config file is not found!"
                exit 1
            fi
            source $ANDROID_CONFIG_FILE
            ;;
        h )
            show_help
            exit 1
            ;;
    esac
done

if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]; then
   echo 'Device serial number is invalid!'
   exit 1
fi

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

AUTH_DETAILS=$CURRENT_DIR/.auth_details
if [[ -e $AUTH_DETAILS ]]; then
    echo "==============> PREVIOUS AUTH DETAILS ============"
    echo
    cat $AUTH_DETAILS
    while true; do
        read -p "Do you wish to use the auth details listed above (y/n)? " ANSWER
        case ${ANSWER} in
            y|Y|yes|YES )
                source $AUTH_DETAILS
                break;;
            n|N|no|NO )
                break;;
        esac
    done
fi

config_userinput(){
    while [[ -z $CLIENT_ID ]] ; do
        echo "Enter your Client ID:"
        read CLIENT_ID
        if [[ ! "$CLIENT_ID" =~ amzn1\.application-oa2-client\.[0-9a-z]{32} ]]
        then
           echo 'Client ID is invalid!'
           unset CLIENT_ID
        fi
    done
    while [[ -z $CLIENT_SECRET ]] ; do
        echo "Enter your Client Secret:"
        read CLIENT_SECRET
        if [[ ! "$CLIENT_SECRET" =~ [0-9a-f]{64} ]]
        then
           echo 'Client Secret is invalid!'
           unset CLIENT_SECRET
        fi
    done
    while [[ -z $PRODUCT_ID ]] ; do
        echo "Enter your Product ID:"
        read PRODUCT_ID
        if [[ ! "$PRODUCT_ID" =~ [0-9a-zA-Z_]+ ]]
        then
           echo 'Product ID is invalid!'
           unset PRODUCT_ID
        fi
    done
    while [[ -z $DEVICE_SERIAL_NUMBER ]] ; do
        echo "Enter your Device Serial Number (this can be any number):"
        read DEVICE_SERIAL_NUMBER
        if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]
        then
           echo 'device serial number is invalid!'
           unset DEVICE_SERIAL_NUMBER
        fi
    done
    while [[ ( "$LOCALE" != "en-US" ) && \
             ( "$LOCALE" != "en-GB" ) && \
             ( "$LOCALE" != "de-DE" ) ]] ; do
        echo "Enter your desired locale (valid values are en-US, en-GB, de-DE):"
        read LOCALE
    done
}
config_userinput

# Store auth details
cat << EOF > "$AUTH_DETAILS"
CLIENT_ID="$CLIENT_ID"
CLIENT_SECRET="$CLIENT_SECRET"
PRODUCT_ID="$PRODUCT_ID"
DEVICE_SERIAL_NUMBER="$DEVICE_SERIAL_NUMBER"
LOCALE="$LOCALE"
EOF
chmod a+rx $AUTH_DETAILS


# Create autostart script
AUTOSTART_SESSION="avsrun"
AUTOSTART_DIR=$HOME/.config/lxsession/LXDE-pi
AUTOSTART=$AUTOSTART_DIR/autostart
if [ ! -f $AUTOSTART ]; then
    mkdir -p $AUTOSTART_DIR
    cp /etc/xdg/lxsession/LXDE-pi/autostart $AUTOSTART
fi
STARTUP_SCRIPT=$CURRENT_DIR/.avsrun-startup.sh
cat << EOF > "$STARTUP_SCRIPT"
#!/bin/bash
$BUILD_PATH/SampleApp/src/SampleApp $CONFIG_FILE $THIRD_PARTY_PATH/alexa-rpi/models
\$SHELL
EOF
chmod a+rx $STARTUP_SCRIPT
while true; do
    read -p "Automatically run AVS SDK at startup (y/n)? " ANSWER
    case ${ANSWER} in
        n|N|no|NO )
            grep $AUTOSTART_SESSION $AUTOSTART > /dev/null 2>&1
            if [ $? == 0 ]; then
                # Remove startup script from autostart file
                sed -i '/'"$AUTOSTART_SESSION"'/d' $AUTOSTART
            fi
            break;;
        y|Y|yes|YES )
            grep $AUTOSTART_SESSION $AUTOSTART > /dev/null 2>&1
            if [ $? != 0 ]; then #avsrun not present
                if ! grep "vocalfusion_3510_sales_demo" $AUTOSTART ; then #vocalfusion_3510_sales_demo not present
                    # Append startup script if not already in autostart file
                    echo "@lxterminal -t $AUTOSTART_SESSION --geometry=150x50 -e $STARTUP_SCRIPT" >> $AUTOSTART
                fi
            else #avsrun present
                if grep "vocalfusion_3510_sales_demo" $AUTOSTART ; then #vocalfusion_3510_sales_demo present
                    # Remove startup script from autostart file
                    echo "Warning: Not adding avsrun in autostart since offline demo is already present. Start AVS by following instructions on vocalfusion_3510_sales_demo startup"
                    sed -i '/'"$AUTOSTART_SESSION"'/d' $AUTOSTART
                fi
            fi
            break;;
    esac
done

set -e

if [ ! -d "$BUILD_PATH" ]
then

  # Make sure required packages are installed
  echo "==============> INSTALLING REQUIRED TOOLS AND PACKAGE ============"
  echo

    sudo apt-get update
    sudo apt-get -y install git gcc cmake build-essential libsqlite3-dev libcurl4-openssl-dev libfaad-dev libsoup2.4-dev libgcrypt20-dev libgstreamer-plugins-bad1.0-dev gstreamer1.0-plugins-good libasound2-dev sox gedit vim python3-pip
    pip install flask commentjson

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
    git clone -b xmos_v1.6 git://github.com/xmos/avs-device-sdk.git


    if [ $# -ge 1 ] && [ $1 = "xvf3510" ] ; then
        echo
        echo "==============> BUILDING PI HAT CONTROL =============="
        echo

        mkdir -p $PI_HAT_CTRL_PATH
        pushd $SOURCE_PATH/avs-device-sdk/ThirdParty/pi_hat_ctrl > /dev/null
        gcc pi_hat_ctrl.c -o $PI_HAT_CTRL_PATH/pi_hat_ctrl -lwiringPi -lm
        popd > /dev/null
    fi

  # make the SDK
  echo
  echo "==============> BUILDING SDK =============="
  echo

    cd $BUILD_PATH
    if [ $# -ge 1 ] && [ $1 = "xvf3510" ] ; then
        cmake "$SOURCE_PATH/avs-device-sdk" \
        -DSENSORY_KEY_WORD_DETECTOR=ON \
        -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=$THIRD_PARTY_PATH/alexa-rpi/lib/libsnsr.a \
        -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=$THIRD_PARTY_PATH/alexa-rpi/include \
        -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
        -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
        -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
        -DCMAKE_BUILD_TYPE=DEBUG \
        -DPI_HAT_CTRL=ON
    else
        cmake "$SOURCE_PATH/avs-device-sdk" \
        -DSENSORY_KEY_WORD_DETECTOR=ON \
        -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=$THIRD_PARTY_PATH/alexa-rpi/lib/libsnsr.a \
        -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=$THIRD_PARTY_PATH/alexa-rpi/include \
        -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
        -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
        -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include" \
        -DCMAKE_BUILD_TYPE=DEBUG
    fi

  cd $BUILD_PATH
  make SampleApp -j2

else
  cd $BUILD_PATH
  make SampleApp -j2
fi

echo
echo "==============> SAVING CONFIGURATION FILE =============="
echo

# Create configuration file with audioSink configuration at the beginning of the file
cat << EOF > "$OUTPUT_CONFIG_FILE"
 {
    "gstreamerMediaPlayer":{
        "audioSink":"$GSTREAMER_AUDIO_SINK"
    },
EOF

cd $INSTALL_BASE
bash genConfig.sh config.json $DEVICE_SERIAL_NUMBER $CONFIG_DB_PATH $SOURCE_PATH/avs-device-sdk $TEMP_CONFIG_FILE

# Delete first line from temp file to remove opening bracket
sed -i -e "1d" $TEMP_CONFIG_FILE

# Append temp file to configuration file
cat $TEMP_CONFIG_FILE >> $OUTPUT_CONFIG_FILE

# Delete temp file
rm $TEMP_CONFIG_FILE

echo
echo "==============> FINAL CONFIGURATION  =============="
echo
cat $CONFIG_FILE

cat << EOF > "$START_AUTH_SCRIPT"
pushd $BUILD_PATH > /dev/null
sudo fuser -k -TERM -n tcp 3000
python AuthServer/AuthServer.py | grep -v '400' &
chromium-browser http://localhost:3000 > /dev/null 2>&1
wait
popd > /dev/null
EOF
chmod +x "$START_AUTH_SCRIPT"

cat << EOF > "$TEST_SCRIPT"
echo
echo "==============> BUILDING Tests =============="
echo
mkdir -p "$UNIT_TEST_MODEL_PATH"
cp "$UNIT_TEST_MODEL" "$UNIT_TEST_MODEL_PATH"
pushd $BUILD_PATH > /dev/null
make all test -j2
popd > /dev/null
EOF
chmod +x "$TEST_SCRIPT"

if [ ! -f $ALIASES ] ; then
echo "Create .bash_aliases file"
cat << EOF > "$ALIASES"
EOF
fi
echo "Delete any existing avs aliases and rewrite them"
sed -i '/avsrun/d' $ALIASES > /dev/null
sed -i '/avsunit/d' $ALIASES > /dev/null
sed -i '/avssetup/d' $ALIASES > /dev/null
sed -i '/avsauth/d' $ALIASES > /dev/null
sed -i '/AVS/d' $ALIASES > /dev/null
sed -i '/AlexaClientSDKConfig.json/d' $ALIASES > /dev/null
sed -i '/Remove/d' $ALIASES > /dev/null

echo "alias avsrun="$BUILD_PATH/SampleApp/src/SampleApp $CONFIG_FILE $THIRD_PARTY_PATH/alexa-rpi/models"" >> $ALIASES
echo "alias avsunit="$TEST_SCRIPT"" >> $ALIASES
echo "alias avssetup="$THIS_SCRIPT"" >> $ALIASES
echo "alias avsauth="$START_AUTH_SCRIPT"" >> $ALIASES
echo "echo "Available AVS aliases:"" >> $ALIASES
echo "echo -e "avsrun, avsunit, avssetup, avsauth"" >> $ALIASES
echo "echo "If authentication fails, please check $BUILD_PATH/Integration/AlexaClientSDKConfig.json"" >> $ALIASES
echo "echo "Remove .bash_aliases and open a new terminal to remove bindings"" >> $ALIASES


echo
echo "==============> AUTHENTICATION =============="
echo

$START_AUTH_SCRIPT

echo " **** Completed Configuration/Build ***"

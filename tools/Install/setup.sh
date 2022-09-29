#!/usr/bin/env bash

# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

set -o errexit  # Exit the script if any statement fails.
set -o nounset  # Exit the script if any uninitialized variable is used.

CLONE_URL=${CLONE_URL:-'https://github.com/alexa/avs-device-sdk.git'}

PORT_AUDIO_FILE="pa_stable_v190600_20161030.tgz"
PORT_AUDIO_DOWNLOAD_URL="http://www.portaudio.com/archives/$PORT_AUDIO_FILE"

CURL_VER=7.67.0
CURL_DOWNLOAD_URL="https://github.com/curl/curl/releases/download/curl-7_67_0/curl-${CURL_VER}.tar.gz"

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
INPUT_CONFIG_FILE="$SOURCE_PATH/avs-device-sdk/Integration/AlexaClientSDKConfig.json"
OUTPUT_CONFIG_FILE="$BUILD_PATH/Integration/AlexaClientSDKConfig.json"
TEMP_CONFIG_FILE="$BUILD_PATH/Integration/tmp_AlexaClientSDKConfig.json"
TEST_SCRIPT="$INSTALL_BASE/test.sh"
LIB_SUFFIX="a"
ANDROID_CONFIG_FILE=""

# Default device serial number if nothing is specified
DEVICE_SERIAL_NUMBER="123456"

# Default device manufacturer name
DEVICE_MANUFACTURER_NAME=${DEVICE_MANUFACTURER_NAME:-"Test Manufacturer"}

# Default device description
DEVICE_DESCRIPTION=${DEVICE_DESCRIPTION:-"Test Device"}

GSTREAMER_AUDIO_SINK="autoaudiosink"

# Defaults for HSM integration
ACSDK_PKCS11_MODULE=
ACSDK_PKCS11_KEY=
ACSDK_PKCS11_PIN=
ACSDK_PKCS11_TOKEN=

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

show_help() {
  echo  'Usage: setup.sh <config-json-file> [OPTIONS]'
  echo  'The <config-json-file> can be downloaded from developer portal and must contain the following:'
  echo  '   "clientId": "<OAuth client ID>"'
  echo  '   "productId": "<your product name for device>"'
  echo  ''
  echo  'Optional parameters'
  echo  '  -s <serial-number>  If nothing is provided, the default device serial number is 123456'
  echo  '  -a <file-name>      The file that contains Android installation configurations (e.g. androidConfig.txt)'
  echo  '  -d <description>    The description of the device.'
  echo  '  -m <manufacturer>   The device manufacturer name.'
  echo  '  -p <module-path>    Absolute path to PKCS#11 interface library.'
  echo  '  -t <token-name>     PKCS#11 token name.'
  echo  '  -i <user-pin>       PKCS#11 user pin to access key object functions.'
  echo  '  -k <key-name>       PKCS#11 key object label.'
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

OPTIONS=s:a:m:d:hp:k:i:t:
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
        d )
            DEVICE_DESCRIPTION="$OPTARG"
            ;;
        m )
            DEVICE_MANUFACTURER_NAME="$OPTARG"
            ;;
        h )
            show_help
            exit 1
            ;;
        p )
            ACSDK_PKCS11_MODULE="$OPTARG"
            ;;
        k )
            ACSDK_PKCS11_KEY="$OPTARG"
            ;;
        i )
            ACSDK_PKCS11_PIN="$OPTARG"
            ;;
        t )
            ACSDK_PKCS11_TOKEN="$OPTARG"
            ;;
    esac
done

if [[ ! "$DEVICE_SERIAL_NUMBER" =~ [0-9a-zA-Z_]+ ]]; then
   echo 'Device serial number is invalid!'
   exit 1
fi

if [ -z "$ACSDK_PKCS11_MODULE" ] && [ -z "$ACSDK_PKCS11_KEY" ] && [ -z "$ACSDK_PKCS11_PIN" ] && [ -z "$ACSDK_PKCS11_TOKEN" ]
then
  echo "PKCS11 parameters are not specified. Hardware security module integration is disabled."
  ACSDK_PKCS11=OFF
  ACSDK_PKCS11_MODULE=__undefined__
  ACSDK_PKCS11_KEY=__undefined__
  ACSDK_PKCS11_PIN=__undefined__
  ACSDK_PKCS11_TOKEN=__undefined__
else
  echo "PKCS11 parameters are specified. Hardware security module integration is enabled."
  ACSDK_PKCS11=ON

  if [ -z "$ACSDK_PKCS11_MODULE" ]
  then
    echo "PKCS11 module path is not specified, but other PKCS11 parameters are present."
    exit 1
  elif [ ! -f "$ACSDK_PKCS11_MODULE" ]
  then
    echo "PKCS11 module path is specified, but library is not found."
    exit 1
  fi

  if [ -z "$ACSDK_PKCS11_KEY" ]
  then
    echo "PKCS11 key name is not specified, but other PKCS11 parameters are present."
    exit 1
  fi

  if [ -z "$ACSDK_PKCS11_PIN" ]
  then
    echo "PKCS11 pin is not specified, but other PKCS11 parameters are present."
    exit 1
  fi

  if [ -z "$ACSDK_PKCS11_TOKEN" ]
  then
    echo "PKCS11 token name is not specified, but other PKCS11 parameters are present."
    exit 1
  fi

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
    git clone --single-branch $CLONE_URL avs-device-sdk
  fi

  # make the SDK
  echo
  echo "==============> BUILDING SDK =============="
  echo

  mkdir -p $BUILD_PATH
  cd $BUILD_PATH
  cmake "$SOURCE_PATH/avs-device-sdk" \
      -DCMAKE_BUILD_TYPE=DEBUG \
      -DPKCS11=$ACSDK_PKCS11 \
      -DINSTALL_COMMON_SAMPLE_LIBS=ON \
      "${CMAKE_PLATFORM_SPECIFIC[@]}"

  cd $BUILD_PATH
  make SampleApp -j2
  make all -j2

else
  cd $BUILD_PATH
  make SampleApp -j2
  make all -j2
fi

echo
echo "==============> SAVING CONFIGURATION FILE =============="
echo

GSTREAMER_CONFIG="{\\n    \"gstreamerMediaPlayer\":{\\n        \"audioSink\":\"$GSTREAMER_AUDIO_SINK\"\\n    },"

cd $INSTALL_BASE
bash genConfig.sh config.json $DEVICE_SERIAL_NUMBER $CONFIG_DB_PATH $SOURCE_PATH/avs-device-sdk $TEMP_CONFIG_FILE \
  -DSDK_CONFIG_MANUFACTURER_NAME="$DEVICE_MANUFACTURER_NAME" -DSDK_CONFIG_DEVICE_DESCRIPTION="$DEVICE_DESCRIPTION" \
  -DPKCS11_MODULE_PATH=$ACSDK_PKCS11_MODULE -DPKCS11_TOKEN_NAME=$ACSDK_PKCS11_TOKEN \
  -DPKCS11_USER_PIN=$ACSDK_PKCS11_PIN -DPKCS11_KEY_NAME=$ACSDK_PKCS11_KEY

# Replace the first opening bracket in the AlexaClientSDKConfig.json file with GSTREAMER_CONFIG variable.
awk -v config="$GSTREAMER_CONFIG" 'NR==1,/{/{sub(/{/,config)}1' $TEMP_CONFIG_FILE > $OUTPUT_CONFIG_FILE

# Delete temp file
rm $TEMP_CONFIG_FILE

echo
echo "==============> FINAL CONFIGURATION  =============="
echo
cat $OUTPUT_CONFIG_FILE

generate_start_script

generate_test_script

echo " **** Completed Configuration/Build ***"

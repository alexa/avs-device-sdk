#!/bin/sh

DIR=`pwd`

AI_DAEMON_CONFIG_JSON=${DIR}/AIDaemon.json
ALEXA_CONFIG_JSON=${DIR}/AlexaClientSDKConfig.json
DIR_AVS_DB=${DIR}/AVS-DB
DEVICESERIALNUMBER=`cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 10 | head -n 1`
CLIENTID="amzn1.application-oa2-client.0b485e8c035d4c64ae67bab79777ebec"
PRODUCTID="Nissan_VPA_POC"
NISSAN_LOCALE="en-US"

if [ ! -e "${AI_DAEMON_CONFIG_JSON}" ]; then
  cp "${ALEXA_CONFIG_JSON}" "${AI_DAEMON_CONFIG_JSON}"
  rm -rf "$DIR_AVS_DB"
fi

sed -i 's/"deviceSerialNumber":""/"deviceSerialNumber":"'${DEVICESERIALNUMBER}'"/' ${AI_DAEMON_CONFIG_JSON}
sed -i 's/"clientId":""/"clientId":"'${CLIENTID}'"/' ${AI_DAEMON_CONFIG_JSON}
sed -i 's/"productId":""/"productId":"'${PRODUCTID}'"/' ${AI_DAEMON_CONFIG_JSON}

if [ ! -d "$DIR_AVS_DB" ]; then
  mkdir "$DIR_AVS_DB"
fi

sed -i 's|"databaseFilePath1":""|"databaseFilePath":"'${DIR_AVS_DB}'/cblAuthDelegate.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath2":""|"databaseFilePath":"'${DIR_AVS_DB}'/miscDatabase.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath3":""|"databaseFilePath":"'${DIR_AVS_DB}'/alerts.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath4":""|"databaseFilePath":"'${DIR_AVS_DB}'/settings.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath5":""|"databaseFilePath":"'${DIR_AVS_DB}'/bluetooth.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath6":""|"databaseFilePath":"'${DIR_AVS_DB}'/certifiedsender.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"databaseFilePath7":""|"databaseFilePath":"'${DIR_AVS_DB}'/notifications.db"|' ${AI_DAEMON_CONFIG_JSON}
sed -i 's|"locale":""|"locale":"'${NISSAN_LOCALE}'"|' ${AI_DAEMON_CONFIG_JSON}

if test -z "$DBUS_SESSION_BUS_ADDRESS" ; then
  ## if not found, launch a new one
  eval `dbus-launch --sh-syntax`
fi

echo "D-Bus per-session daemon address is: $DBUS_SESSION_BUS_ADDRESS"
echo "$DBUS_SESSION_BUS_ADDRESS" > /tmp/sa_dbus_session.dat

pulseaudio --kill
pulseaudio --start

export LD_LIBRARY_PATH=${DIR}/lib/avs:${DIR}/lib/curl:${DIR}/lib/nghttp2:${DIR}/lib/avs/portaudio:/usr/lib:${LD_LIBRARY_PATH}
echo "${LD_LIBRARY_PATH}"

./SampleApp ${AI_DAEMON_CONFIG_JSON} DEBUG9

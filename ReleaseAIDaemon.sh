#!/bin/sh

DIR=`pwd`
SDK_BUILD=${DIR}/../../sdk-build
PACKAGE_X86=${DIR}/package/x86
APP_NAME=SampleApp
APP_LIBRARY=lib
AVS_LIBRARY=${PACKAGE_X86}/${APP_LIBRARY}/avs
CURL_LIRBRARY=${PACKAGE_X86}/${APP_LIBRARY}/curl
NGHTTP2_LIRBRARY=${PACKAGE_X86}/${APP_LIBRARY}/nghttp2
PORTAUDIO_LIRBRARY=${PACKAGE_X86}/${APP_LIBRARY}/portaudio

THIRDPARTY_LIBRARY=${SDK_BUILD}/../third-party
THIRDPARTY_LIBRARY_CURL=${THIRDPARTY_LIBRARY}/curl-7.54.0/lib/.libs
THIRDPARTY_LIBRARY_NGHTTP2=${THIRDPARTY_LIBRARY}/nghttp2/lib/.libs
THIRDPARTY_LIBRARY_PORTAUDIO=${THIRDPARTY_LIBRARY}/portaudio/lib/.libs

rm -rf ${PACKAGE_X86}/${APP_NAME}
cp ${SDK_BUILD}/${APP_NAME}/src/${APP_NAME} ${PACKAGE_X86}

rm -rf ${PACKAGE_X86}/${APP_LIBRARY}
mkdir ${PACKAGE_X86}/${APP_LIBRARY}

mkdir ${AVS_LIBRARY}
cp ${SDK_BUILD}/ACL/src/libACL.so                                           ${AVS_LIBRARY}
cp ${SDK_BUILD}/ADSL/src/libADSL.so                                         ${AVS_LIBRARY}
cp ${SDK_BUILD}/AFML/src/libAFML.so                                         ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/AIP/src/libAIP.so                          ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/Alerts/src/libAlerts.so                    ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/AudioPlayer/src/libAudioPlayer.so          ${AVS_LIBRARY}
cp ${SDK_BUILD}/ApplicationUtilities/Resources/Audio/src/libAudioResources.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/AVSCommon/libAVSCommon.so                                   ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/System/src/libAVSSystem.so                 ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/Bluetooth/src/libBluetooth.so              ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilitiesDelegate/src/libCapabilitiesDelegate.so         ${AVS_LIBRARY}
cp ${SDK_BUILD}/SampleApp/Authorization/CBLAuthDelegate/src/libCBLAuthDelegate.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/CertifiedSender/src/libCertifiedSender.so                   ${AVS_LIBRARY}
cp ${SDK_BUILD}/ContextManager/src/libContextManager.so                     ${AVS_LIBRARY}
cp ${SDK_BUILD}/ApplicationUtilities/DefaultClient/src/libDefaultClient.so  ${AVS_LIBRARY}
cp ${SDK_BUILD}/ESP/src/libESP.so                                           ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/ExternalMediaPlayer/src/libExternalMediaPlayer.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/InteractionModel/src/libInteractionModel.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/KWD/src/libKWD.so                                           ${AVS_LIBRARY}
cp ${SDK_BUILD}/MediaPlayer/GStreamerMediaPlayer/src/libMediaPlayer.so      ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/MRM/src/libMRM.so                          ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/Notifications/src/libNotifications.so      ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/PlaybackController/src/libPlaybackController.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/PlaylistParser/src/libPlaylistParser.so                     ${AVS_LIBRARY}
cp ${SDK_BUILD}/RegistrationManager/src/libRegistrationManager.so           ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/Settings/src/libSettings.so                ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/SpeakerManager/src/libSpeakerManager.so    ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/SpeechSynthesizer/src/libSpeechSynthesizer.so ${AVS_LIBRARY}
cp ${SDK_BUILD}/Storage/SQLiteStorage/src/libSQLiteStorage.so               ${AVS_LIBRARY}
cp ${SDK_BUILD}/CapabilityAgents/TemplateRuntime/src/libTemplateRuntime.so ${AVS_LIBRARY}

mkdir ${CURL_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_CURL}/libcurl.a         ${CURL_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_CURL}/libcurl.so        ${CURL_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_CURL}/libcurl.so.4      ${CURL_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_CURL}/libcurl.so.4.4.0  ${CURL_LIRBRARY}

mkdir ${NGHTTP2_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_NGHTTP2}/libnghttp2.a           ${NGHTTP2_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_NGHTTP2}/libnghttp2.so          ${NGHTTP2_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_NGHTTP2}/libnghttp2.so.14       ${NGHTTP2_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_NGHTTP2}/libnghttp2.so.14.17.0  ${NGHTTP2_LIRBRARY}

mkdir ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.a           ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.exp         ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.la          ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.lai         ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.so          ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.so.2        ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.so.2.0.0    ${PORTAUDIO_LIRBRARY}
cp ${THIRDPARTY_LIBRARY_PORTAUDIO}/libportaudio.ver         ${PORTAUDIO_LIRBRARY}

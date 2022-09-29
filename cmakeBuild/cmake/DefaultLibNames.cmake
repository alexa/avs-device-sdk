#
# Provide default library names for libraries that may be overriden either by custom
# implementations added via EXTENSION_PATHS or by alternate implementations available
# in the SDK.

macro(UseDefaultIfNotSet var default)
    if (NOT ${var})
        set(${var} ${default})
    endif()
    message(STATUS "UseDefaultIfNotSet: ${var} set to ${${var}}.")
endmacro()

UseDefaultIfNotSet(ACSDKALEXACOMMUNICATIONS_LIB acsdkLibcurlAlexaCommunications)
UseDefaultIfNotSet(ACSDKAUDIOINPUTSTREAM_LIB acsdkAudioInputStream)
UseDefaultIfNotSet(ACSDKAUTHORIZATIONDELEGATE_LIB acsdkCBLAuthorizationDelegate)
UseDefaultIfNotSet(ACSDKDEVICESETTINGSMANAGER_LIB acsdkDefaultDeviceSettingsManager)
UseDefaultIfNotSet(ACSDKHTTPCONTENTFETCHER_LIB acsdkLibcurlHTTPContentFetcher)
UseDefaultIfNotSet(ACSDKINTERNETCONNECTIONMONITOR_LIB acsdkDefaultInternetConnectionMonitor)
UseDefaultIfNotSet(ACSDKMETRICRECORDER_LIB acsdkNullMetricRecorder)
UseDefaultIfNotSet(ACSDKSYSTEMTIMEZONE_LIB acsdkNullSystemTimeZone)

#
# Default audio pipeline factory implementation depends on which media player is being used.
#
if (GSTREAMER_MEDIA_PLAYER)
    UseDefaultIfNotSet(ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_LIB acsdkGstreamerApplicationAudioPipelineFactory)
elseif(ANDROID)
    # When building the SDK for Android, ANDROID_MEDIA_PLAYER is ON by default (whether explicitly specified or not).
    if(NOT ANDROID_MEDIA_PLAYER STREQUAL "OFF")
        UseDefaultIfNotSet(ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_LIB acsdkAndroidApplicationAudioPipelineFactory)
    endif()
elseif(CUSTOM_MEDIA_PLAYER)
    UseDefaultIfNotSet(ACSDKAPPLICATIONAUDIOPIPELINEFACTORY_LIB acsdkCustomApplicationAudioPipelineFactory)
endif()

#
# When Bluetooth is enabled, the default implementation is BlueZ. When disabled, use the null implementation.
#
if (BLUETOOTH_ENABLED)
    if (BLUETOOTH_BLUEZ)
        UseDefaultIfNotSet(ACSDKBLUETOOTHIMPLEMENTATIONS_LIB acsdkBlueZBluetoothImplementation)
    else()
        # Log a warning for this case, but not a fatal error. Applications that use the deprecated DefaultClient::create
        # method may use their own Bluetooth implementation by passing in their implementation of BluetoothDeviceManagerInterface,
        # without specifying a custom ACSDKBLUETOOTHIMPLEMENTATIONS_LIB here.
        message(WARNING "No default library available for ACSDKBLUETOOTHIMPLEMENTATIONS_LIB when BLUETOOTH_ENABLED=ON BLUETOOTH_BLUEZ=OFF")
    endif()
else()
    set(ACSDKBLUETOOTHIMPLEMENTATIONS_LIB acsdkNullBluetoothImplementation)
endif()

if (OPUS)
    UseDefaultIfNotSet(ACSDKSPEECHENCODER_LIB acsdkOpusSpeechEncoder)
else()
    UseDefaultIfNotSet(ACSDKSPEECHENCODER_LIB acsdkNullSpeechEncoder)
endif()

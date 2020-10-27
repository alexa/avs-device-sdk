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
UseDefaultIfNotSet(ACSDKAUTHORIZATIONDELEGATE_LIB acsdkCBLAuthorizationDelegate)
UseDefaultIfNotSet(ACSDKHTTPCONTENTFETCHER_LIB acsdkLibcurlHTTPContentFetcher)
UseDefaultIfNotSet(ACSDKINTERNETCONNECTIONMONITOR_LIB acsdkDefaultInternetConnectionMonitor)
UseDefaultIfNotSet(ACSDKMETRICRECORDER_LIB acsdkNullMetricRecorder)


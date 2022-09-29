#
# Setup the Logging build.
#
# To enable logging in the SDK, include the following option on the cmake command line:
#     -DACSDK_LOG=ON
# Note: This is enabled by default for DEBUG build types
#
# To enable logging of Debug level logs, include the following option on the cmake command line:
#     -DACSDK_DEBUG_LOG=ON
# Note: This is enabled by default for DEBUG build types
#
# To enable logging of sensitive data, include the following option on the cmake command line:
#     -DACSDK_EMIT_SENSITIVE_LOGS=ON
# Note that this option is only honored in DEBUG builds.
#
# Older Android releases redefine __func__ as __PRETTY_FUNCTION__, which produces unnecessarily large output logs.
# Logger.h now undefines this macro unless ACSDK_LOGS_KEEP_FUNC_MACRO is set to OFF. To keep the macro definition
# intact, include the following option on the cmake command line:
#     -DACSDK_LOGS_KEEP_FUNC_MACRO=ON
#

option(ACSDK_LOG "Enabled logging within the SDK" OFF)
option(ACSDK_DEBUG_LOG "Enables logging of DEBUG level logs" OFF)
option(ACSDK_EMIT_SENSITIVE_LOGS "Enable Logging of sensitive information." OFF)
option(ACSDK_LOGS_KEEP_FUNC_MACRO "Keep __func__ macro if defined." OFF)

if (ACSDK_EMIT_SENSITIVE_LOGS)
    string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
    if (BUILD_TYPE_UPPER STREQUAL DEBUG)
        message("WARNING: Logging of sensitive information enabled!")
        add_definitions(-DACSDK_EMIT_SENSITIVE_LOGS)
    else()
        message(FATAL_ERROR "FATAL_ERROR: ACSDK_EMIT_SENSITIVE_LOGS=ON in non-DEBUG build.")
    endif()
endif()

if (ACSDK_LOG)
    add_definitions(-DACSDK_LOG_ENABLED)
    if (NOT buildType STREQUAL "DEBUG")
    message(WARNING
            "\nWARNING: Logging has been enabled. Logging should not be enabled for non-DEBUG Builds. "
            "Doing so is in violation of the Alexa Service Requirements Point 6. "
            "https://www.developer.amazon.com/support/legal/alexa/alexa-voice-service/terms-and-agreements#alexa-service-requirements \n")
    endif()
endif()

if (ACSDK_LATENCY_LOG)
    add_definitions(-DACSDK_LATENCY_LOG_ENABLED)
endif()

if (ACSDK_DEBUG_LOG)
    add_definitions(-DACSDK_DEBUG_LOG_ENABLED)
    if (NOT buildType STREQUAL "DEBUG")
        message("WARNING: Logging of debug logging enabled")
    endif()
endif()

if (ACSDK_LOGS_KEEP_FUNC_MACRO)
    add_definitions(-DACSDK_LOGS_KEEP_FUNC_MACRO)
endif()

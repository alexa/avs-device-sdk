#
# Setup the Logging build.
#
# To enable logging of sensitive data, include the following option on the cmake command line:
#     -DACSDK_EMIT_SENSITIVE_LOGS=ON
# Note that this option is only honored in DEBUG builds.
#

option(ACSDK_EMIT_SENSITIVE_LOGS "Enable Logging of sensitive information." OFF)

if (ACSDK_EMIT_SENSITIVE_LOGS)
    string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
    if (BUILD_TYPE_UPPER STREQUAL DEBUG)
        message("WARNING: Logging of sensitive information enabled!")
        add_definitions(-DACSDK_EMIT_SENSITIVE_LOGS)
    else()
        message(FATAL_ERROR "FATAL_ERROR: ACSDK_EMIT_SENSITIVE_LOGS=ON in non-DEBUG build.")
    endif()
endif()

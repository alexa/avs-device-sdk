#
# Set up Diagnostics for the sample app.
#
# If DIAGNOSTICS is ON then all related components are enabled by default. To disable only one component, set its option
# to OFF. For example:
#     cmake <path-to-source>
#       -DDIAGNOSTICS=ON
#       -DPROTOCOL_TRACE=OFF
#       ...etc.
#
# Note that file-based microphone is a specific implementation of a microphone for use with diagnostics. If another
# microphone type is provided (eg PortAudio, Android), then file-based microphone will default to OFF even in diagnostic
# mode.

include(CMakeDependentOption)

option(DIAGNOSTICS "Enable diagnostics." OFF)
cmake_dependent_option(PROTOCOL_TRACE "Record directives and events processed by the SDK." ON "DIAGNOSTICS" OFF)
cmake_dependent_option(DEVICE_PROPERTIES "Retrieve SDK properties." ON "DIAGNOSTICS" OFF)
cmake_dependent_option(AUDIO_INJECTION "Inject audio for diagnostic purposes." ON "DIAGNOSTICS" OFF)

if(DIAGNOSTICS)
    string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE_UPPER)
    if (BUILD_TYPE_UPPER STREQUAL "DEBUG")
        if ((NOT PROTOCOL_TRACE) AND (NOT DEVICE_PROPERTIES) AND (NOT AUDIO_INJECTION))
            message(WARNING "Diagnostics cannot be enabled without also enabling at least one of
                PROTOCOL_TRACE, DEVICE_PROPERTIES, or AUDIO_INJECTION.")
        else()
            add_definitions("-DDIAGNOSTICS")

            if (PROTOCOL_TRACE)
                add_definitions("-DPROTOCOL_TRACE")
            endif()

            if (DEVICE_PROPERTIES)
                add_definitions("-DDEVICE_PROPERTIES")
            endif()

            if (AUDIO_INJECTION)
                if (PORTAUDIO OR ANDROID_MICROPHONE)
                    message(WARNING "Audio injection is not available with the provided microphone module.")
                else()
                    add_definitions("-DAUDIO_INJECTION")
                endif()
            endif()
        endif()
    else()
        message(FATAL_ERROR "\nDIAGNOSTICS MUST NOT BE ENABLED IN RELEASE BUILDS.\nDIAGNOSTICS ONLY AVAILABLE IN DEBUG BUILDS.\nDO NOT RELEASE DEVICES IN DEBUG MODE.\n\n")
    endif()
endif()
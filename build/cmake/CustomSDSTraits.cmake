#
# Setup the customized traits options for AudioInputStream.
#
# To build with Custom audio input stream traits support, include the following option on
# the cmake command line.
#     cmake <path-to-source> -DCUSTOM_AUDIO_INPUT_STREAM_TRAITS=ON
#                            -DCUSTOMSDSTRAITS_LIB_NAME=<CustomSDSTraits-lib-name>
#                            -DCUSTOMSDSTRAITS_LIB_DIR=<path-to-CustomSDSTraits-lib-dir>
#                            -DCUSTOMSDSTRAITS_HEADER=<CustomSDSTraits-header-file>
#                            -DCUSTOMSDSTRAITS_CLASS=<CustomSDSTraits-class>
#
option(CUSTOM_AUDIO_INPUT_STREAM_TRAITS "Use a customized audio input stream traits." OFF)

if(CUSTOM_AUDIO_INPUT_STREAM_TRAITS)
    message("Creating ${PROJECT_NAME} with a customized audio input stream traits")
    if(NOT CUSTOMSDSTRAITS_LIB_NAME)
        message(FATAL_ERROR "Must pass library name to use a customized audio input stream traits.")
    endif()
    if(NOT CUSTOMSDSTRAITS_LIB_DIR)
        message(FATAL_ERROR "Must pass library dir to use a customized audio input stream traits.")
    endif()
    if(NOT CUSTOMSDSTRAITS_HEADER)
        message(FATAL_ERROR "Must pass header file to use a customized audio input stream traits.")
    endif()
    add_definitions(-DCUSTOM_SDS_TRAITS_HEADER=<${CUSTOMSDSTRAITS_HEADER}>)
    if(NOT CUSTOMSDSTRAITS_CLASS)
        message(FATAL_ERROR "Must pass class name to use a customized audio input stream traits.")
    endif()
    add_definitions(-DCUSTOM_SDS_TRAITS_CLASS=${CUSTOMSDSTRAITS_CLASS})
endif()
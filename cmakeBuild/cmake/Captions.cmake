#
# Set up Captions specific configurations for the sample app.
#
# To build with Captions, run the following command,
#     cmake <path-to-source>
#       -DCAPTIONS=ON
#           -DLIBWEBVTT_LIB_PATH=<path-to-libwebvtt-lib>
#           -DLIBWEBVTT_INCLUDE_DIR=<path-to-libwebvtt-include-dir>
#

option(CAPTIONS "Enable captions support." OFF)

if(CAPTIONS)
    if(NOT LIBWEBVTT_LIB_PATH)
        message(FATAL_ERROR "Must pass path to the external WebVTT parsing library to enable captions support.")
    endif()
    if(NOT LIBWEBVTT_INCLUDE_DIR)
        message(FATAL_ERROR "Must provide the path to the directory containg the header files of the "
                "WebVTT parsing library to enable captions support.")
    endif()
    message("Creating ${PROJECT_NAME} with captions enabled")
    add_definitions("-DENABLE_CAPTIONS")
endif()
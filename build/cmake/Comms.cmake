#
# Setup the Comms compiler options.
#
# To build with Comms support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -DCOMMS=ON
#           -DCOMMS_LIB_PATH=<path-to-comms-lib>
#           -DCOMMS_INCLUDE_DIR=<path-to-comms-include-dir>
#

option(COMMS "Enable Alexa Comms (Calling)." OFF)

if(COMMS)
    if(NOT COMMS_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of AlexaCommsLib to enable Comms.")
    endif()
    if(NOT COMMS_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of AlexaCommsLib to enable Comms.")
    endif()
    message("Creating ${PROJECT_NAME} with Alexa Comms (Calling)")
    add_definitions(-DENABLE_COMMS)
endif()

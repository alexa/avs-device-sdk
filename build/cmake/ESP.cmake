#
# Setup the ESP compiler options.
#
# To build with ESP support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -DESP_PROVIDER=ON
#           -DESP_LIB_PATH=<path-to-esp-lib>
#           -DESP_INCLUDE_DIR=<path-to-esp-include-dir>
#

option(ESP_PROVIDER "Enable Echo Spatial Perception (ESP)." OFF)

if(ESP_PROVIDER)
    if(NOT ESP_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of ESP to enable it.")
    endif()
    if(NOT ESP_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of ESP to enable it.")
    endif()
    message("Creating ${PROJECT_NAME} with Echo Spatial Perception (ESP)")
    add_definitions(-DENABLE_ESP)
endif()

#
# Setup the RTCSessionController compiler options.
#
# To build with RTCSessionController capabilities, specify:
# cmake <path-to-source> -DRTCSC=ON \
#           -DRTCSC_LIB_PATH=<path-to-rtcsc-lib> \
#           -DRTCSC_INCLUDE_DIR=<path-to-rtcsc-include-dir>

option(RTCSC "Enable the RTCSessionController functionality" OFF)

if(RTCSC)
    if(NOT RTCSC_LIB_PATH)
        message(FATAL_ERROR "Must pass library path for RTCSC to enable RTCSC.")
    endif()
    if(NOT RTCSC_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path for RTCSC to enable RTCSC.")
    endif()
    message("Creating ${PROJECT_NAME} with RTCSessionController")
    add_definitions(-DENABLE_RTCSC)
endif()

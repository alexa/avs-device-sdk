# Setup Low Power Mode compiler options.
#
# To build with Low Power Mode capabilties, specify:
# cmake <path-to-source> -DLPM

option(LPM "Enable Low Power Mode functionality" OFF)

if(LPM)
    message("Enabling Low Power Mode on ${PROJECT_NAME}")
    add_definitions(-DENABLE_LPM)
endif()

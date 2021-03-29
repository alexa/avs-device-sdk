#
# Setup the PhoneCallController compiler options.
#
# To build with PhoneCallController capabilties, specify:
# cmake <path-to-source> -DPCC

option(PCC "Enable the PhoneCallController functionality" OFF)

if(PCC)
    message("Creating ${PROJECT_NAME} with PhoneCallController")
    add_definitions(-DENABLE_PCC)
endif()

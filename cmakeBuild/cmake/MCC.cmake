#
# Setup the MeetingClientController compiler options.
#
# To build with MeetingClientController capabilties, specify:
# cmake <path-to-source> -DMCC

option(MCC "Enable the MeetingClientController functionality" OFF)

if(MCC)
    message("Creating ${PROJECT_NAME} with MeetingClientController")
    add_definitions(-DENABLE_MCC)
endif()

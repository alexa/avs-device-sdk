#
# Setup the MessagingController compiler options.
#
# To build with MessagingController capability, specify:
# cmake <path-to-source> -DMC=ON -DEXTENSION_PATHS=</path/to/messagingcontroller>;</path/to/other/extensions>

option(MC "Enable the MessagingController functionality" OFF)
if(MC)
    message("Creating ${PROJECT_NAME} with MessagingController")
    add_definitions(-DENABLE_MC)
endif()

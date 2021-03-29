#
# Setup the Metrics compiler options.
#
# To build with Metrics support, include the following option on the cmake command line.
#     cmake <path-to-source> -DMETRICS=ON
#
option(METRICS "Enable Metrics upload." OFF)

if(METRICS)
    message("Creating ${PROJECT_NAME} with Metrics upload")
    add_definitions(-DACSDK_ENABLE_METRICS_RECORDING)
endif()
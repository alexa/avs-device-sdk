#
# Setup the OPUS compiler options.
#
# To build with OPUS support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -OPUS=ON
#

option(OPUS "Enable OPUS encoding." OFF)

if(OPUS)
    message("Creating ${PROJECT_NAME} with OPUS encoding")
    add_definitions(-DENABLE_OPUS)
endif()

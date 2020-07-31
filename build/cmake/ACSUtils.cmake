#
# Set up to consume options provided by ACS
#
# To build with ACS utilities, run the following command,
#     cmake <path-to-source> -DACS_UTILS=ON
# and include the cmake project for ACS utilities in -DEXTENSION_PATHS
#

option(ACS_UTILS "Enable ACS utiltiies." OFF)

if(ACS_UTILS)
    add_definitions(-DACSDK_ACS_UTILS)
endif()

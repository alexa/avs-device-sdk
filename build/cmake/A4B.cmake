#
# Setup the A4B build to enable Alexa For Business.
#
# To build with Alexa For Business support, run the following command,
#     cmake <path-to-source> -DA4B=ON.
#

option(A4B "Enable Alexa for Business support." OFF)

if(A4B)
    add_definitions(-DENABLE_REVOKE_AUTH)
endif()

#
# Set up PortAudio libraries for the sample app.
#
# To build with PortAudio, run the following command,
#     cmake <path-to-source> 
#       -DPORTAUDIO=ON 
#           -DPORTAUDIO_LIB_PATH=<path-to-portaudio-lib> 
#           -DPORTAUDIO_INCLUDE_DIR=<path-to-portaudio-include-dir>
#

option(PORTAUDIO "Enable PortAudio for the sample app." OFF)

if(PORTAUDIO)
    if(NOT PORTAUDIO_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of PortAudio to enable it.")
    endif()
    if(NOT PORTAUDIO_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of PortAudio to enable it.")
    endif()
endif()

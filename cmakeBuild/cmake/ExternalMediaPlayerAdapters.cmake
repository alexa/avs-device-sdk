#
# Set up ExternalMediaPlayerAdapter library for the Sample App.
#
# To build with PortAudio, run the following command,
#     cmake <path-to-source>
#       -DEXTERNAL_MEDIA_ADAPTERS=ON \
#       -DEXTENSION_PATH=<path to adapter library>
#
# See ExtensionPath.cmake for more details on how to use the EXTENSION_PATH
# cmake variable.

option(EXTERNAL_MEDIA_ADAPTERS "Enable external media player adapters" OFF)

if (EXTERNAL_MEDIA_ADAPTERS)
    message("Enabling external media player adapters on ${PROJECT_NAME}")
    add_definitions("-DEXTERNAL_MEDIA_ADAPTERS")
    set(EXTERNAL_MEDIA_ADAPTERS ON)
endif()
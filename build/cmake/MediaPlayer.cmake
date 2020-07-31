#
# Setup the MediaPlayer build.
#
# To build the GStreamer based MediaPlayer, run the following command,
#     cmake <path-to-source> -DGSTREAMER_MEDIA_PLAYER=ON.
#
# To build a custom media player, run the following command,
#     cmake <path-to-source> -DCUSTOM_MEDIA_PLAYER=ON -DEXTENSION_PATH=<Path to custom media player>

option(GSTREAMER_MEDIA_PLAYER "Enable GStreamer based media player." OFF)
option(CUSTOM_MEDIA_PLAYER "Enable Custom media player." OFF)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
if(GSTREAMER_MEDIA_PLAYER)
    find_package(PkgConfig)
    pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.8 gstreamer-app-1.0>=1.8 gstreamer-controller-1.0>=1.8)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
    message("Building with Gstreamer enabled")
elseif(CUSTOM_MEDIA_PLAYER)
    message("Building with Custom media player enabled")
endif()

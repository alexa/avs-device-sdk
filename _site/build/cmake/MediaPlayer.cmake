#
# Setup the MediaPlayer build.
#
# To build the GStreamer based MediaPlayer, run the following command,
#     cmake <path-to-source> -DGSTREAMER_MEDIA_PLAYER=ON.
#
#

option(GSTREAMER_MEDIA_PLAYER "Enable GStreamer based media player." OFF)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
if(GSTREAMER_MEDIA_PLAYER)
    find_package(PkgConfig)
    pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.8 gstreamer-app-1.0>=1.8)
    add_definitions(-DGSTREAMER_MEDIA_PLAYER)
endif()

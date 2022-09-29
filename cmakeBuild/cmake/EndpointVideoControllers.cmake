# Setup the video controller capabilities compiler options.
#
# To build with all endpoint video controller capabilities, specify:
#   cmake <path-to-source>
#       -DENABLE_ALL_VIDEO_CONTROLLERS=ON
#
# Or, you could specify each controller capabilities`
#   cmake <path-to-source>
#       -DVIDEO_CONTROLLERS_ALEXA_LAUNCHER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_PLAYBACK_STATE_REPORTER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER=ON
#       -DVIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER=ON
#
# Also, you can specify all and opt out capabilities which are not required,
# by specifying it as 'OFF'. For instance, you can enable all except the Alexa
# Launcher capability by setting -DVIDEO_CONTROLLERS_ALEXA_LAUNCHER=OFF
# For example:
#   cmake <path-to-source>
#       -DENABLE_ALL_VIDEO_CONTROLLERS=ON
#           -DVIDEO_CONTROLLERS_ALEXA_LAUNCHER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_PLAYBACK_STATE_REPORTER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER=OFF
#           -DVIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER=OFF

# Default is OFF
option(ENABLE_ALL_VIDEO_CONTROLLERS "Enable all video endpoint controller capabilities" OFF)

# Enable controller capabilities based on the 'ENABLE_ALL_ENDPOINT_CONTROLLERS'
if(ENABLE_ALL_VIDEO_CONTROLLERS)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_LAUNCHER "Enable video endpoint controller Alexa Launcher capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER "Enable video endpoint controller Alexa Keypad Controller capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER "Enable video endpoint controller Alexa Playback Controller capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_PLAYBACK_STATE_REPORTER "Enable video endpoint controller Alexa Playback State Reporter capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER "Enable video endpoint controller Alexa Seek Controller capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER "Enable video endpoint controller Alexa Video Recorder capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER "Enable video endpoint controller Alexa Channel Controller capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER "Enable video endpoint controller Alexa Record Controller capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
    cmake_dependent_option(VIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER "Enable video endpoint controller Alexa Remote Video Player capability" ON "ENABLE_ALL_VIDEO_CONTROLLERS" OFF)
endif()

if(VIDEO_CONTROLLERS_ALEXA_LAUNCHER)
    message("Creating ${PROJECT_NAME} with Alexa Launcher capability")
    add_definitions(-DALEXA_LAUNCHER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER)
    message("Creating ${PROJECT_NAME} with Alexa Keypad Controller capability")
    add_definitions(-DALEXA_KEYPAD_CONTROLLER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER)
    message("Creating ${PROJECT_NAME} with Alexa Playback Controller capability")
    add_definitions(-DALEXA_PLAYBACK_CONTROLLER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_PLAYBACK_STATE_REPORTER)
    if(NOT VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER)
        message(FATAL_ERROR "Must enable Alexa Playback Controller capability to enable Alexa Playback State Reporter capability")
    endif()
    message("Creating ${PROJECT_NAME} with Alexa Playback State Reporter capability")
    add_definitions(-DALEXA_PLAYBACK_STATE_REPORTER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER)
    message("Creating ${PROJECT_NAME} with Alexa Seek Controller capability")
    add_definitions(-DALEXA_SEEK_CONTROLLER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER)
    message("Creating ${PROJECT_NAME} with Alexa Video Recorder capability")
    add_definitions(-DALEXA_VIDEO_RECORDER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER)
    message("Creating ${PROJECT_NAME} with Alexa Channel Controller capability")
    add_definitions(-DALEXA_CHANNEL_CONTROLLER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER)
    message("Creating ${PROJECT_NAME} with Alexa Record Controller capability")
    add_definitions(-DALEXA_RECORD_CONTROLLER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER)
    message("Creating ${PROJECT_NAME} with Alexa Remote Video Player capability")
    add_definitions(-DALEXA_REMOTE_VIDEO_PLAYER)
endif()

if(VIDEO_CONTROLLERS_ALEXA_LAUNCHER
    OR VIDEO_CONTROLLERS_ALEXA_KEYPAD_CONTROLLER
    OR VIDEO_CONTROLLERS_ALEXA_PLAYBACK_CONTROLLER
    OR VIDEO_CONTROLLERS_ALEXA_PLAYBACK_STATE_REPORTER
    OR VIDEO_CONTROLLERS_ALEXA_SEEK_CONTROLLER
    OR VIDEO_CONTROLLERS_ALEXA_VIDEO_RECORDER
    OR VIDEO_CONTROLLERS_ALEXA_CHANNEL_CONTROLLER
    OR VIDEO_CONTROLLERS_ALEXA_RECORD_CONTROLLER
    OR VIDEO_CONTROLLERS_ALEXA_REMOTE_VIDEO_PLAYER)
    add_definitions(-DENABLE_VIDEO_CONTROLLERS)
endif()

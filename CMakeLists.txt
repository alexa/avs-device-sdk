cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Set project information
project(AlexaClientSDK VERSION 1.6.0 LANGUAGES CXX)
set(PROJECT_BRIEF "A cross-platform, modular SDK for interacting with the Alexa Voice Service")
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CapabilityAgents/ExternalMediaPlayer/src/ExternalMediaPlayerAdapters")
    set(HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS ON)
endif()

include(build/BuildDefaults.cmake)
include(tools/Testing.cmake)

# Set variables for target install and .pc pkg-config file
include(build/cmake/PrepareInstall.cmake)


configure_file (
	"${PROJECT_SOURCE_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKVersion.h.in"
	"${PROJECT_SOURCE_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKVersion.h"
	)


# Alexa Client SDK targets.
add_subdirectory("ThirdParty")
add_subdirectory("AVSCommon")
add_subdirectory("ACL")
add_subdirectory("AuthDelegate")
add_subdirectory("ADSL")
add_subdirectory("AFML")
add_subdirectory("CertifiedSender")
add_subdirectory("ContextManager")
add_subdirectory("MediaPlayer")
add_subdirectory("PlaylistParser")
add_subdirectory("KWD")
add_subdirectory("CapabilityAgents")
if (ACSDK_EXCLUDE_TEST_FROM_ALL)
    add_subdirectory("Integration" EXCLUDE_FROM_ALL)
else()
    add_subdirectory("Integration")
endif()
add_subdirectory("ApplicationUtilities")
add_subdirectory("ESP")
add_subdirectory("RegistrationManager")
add_subdirectory("SampleApp")
add_subdirectory("Storage")
add_subdirectory("doc")

# Create .pc pkg-config file
include(build/cmake/GeneratePkgConfig.cmake)

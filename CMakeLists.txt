cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Set project information
project(AlexaClientSDK VERSION 1.18.0 LANGUAGES CXX)
set(PROJECT_BRIEF "A cross-platform, modular SDK for interacting with the Alexa Voice Service")
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CapabilityAgents/ExternalMediaPlayer/src/ExternalMediaPlayerAdapters")
    set(HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS ON)
endif()

# This variable should be used by extension packages to include cmake files from this project.
get_filename_component(AVS_CORE . ABSOLUTE)

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
add_subdirectory("Metrics")
add_subdirectory("ACL")
add_subdirectory("ADSL")
add_subdirectory("AFML")
add_subdirectory("Captions")
add_subdirectory("AVSGatewayManager")
add_subdirectory("CertifiedSender")
add_subdirectory("Endpoints")
add_subdirectory("Settings")
add_subdirectory("BluetoothImplementations")
add_subdirectory("EqualizerImplementations")
add_subdirectory("ContextManager")
add_subdirectory("CapabilitiesDelegate")
add_subdirectory("InterruptModel")
add_subdirectory("PlaylistParser")
add_subdirectory("KWD")
add_subdirectory("CapabilityAgents")
add_subdirectory("Integration")
add_subdirectory("ApplicationUtilities")
add_subdirectory("MediaPlayer")
add_subdirectory("RegistrationManager")
add_subdirectory("SampleApp")
add_subdirectory("SpeechEncoder")
add_subdirectory("Storage")
add_subdirectory("SynchronizeStateSender")
add_subdirectory("doc")

include(build/cmake/ExtensionPath.cmake)
add_extension_projects()

# Create .pc pkg-config file
include(build/cmake/GeneratePkgConfig.cmake)

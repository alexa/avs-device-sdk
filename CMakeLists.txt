cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Set project information
project(AlexaClientSDK VERSION 3.0.0 LANGUAGES CXX)
set(PROJECT_BRIEF "A cross-platform, modular SDK for interacting with the Alexa Voice Service")

# This variable should be used by extension packages to include cmake files from this project.
get_filename_component(AVS_CORE . ABSOLUTE)
get_filename_component(AVS_CORE_BINARY ${PROJECT_BINARY_DIR} ABSOLUTE)

# This variable should be used to get the cmake build files.
get_filename_component(AVS_CMAKE_BUILD cmakeBuild ABSOLUTE)

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)
include(tools/Testing.cmake)

# Set variables for target install and .pc pkg-config file
include(${AVS_CMAKE_BUILD}/cmake/PrepareInstall.cmake)

configure_file (
	"${PROJECT_SOURCE_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKVersion.h.in"
	"${PROJECT_BINARY_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKVersion.h"
	)

configure_file (
	"${PROJECT_SOURCE_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKConfig.h.in"
	"${PROJECT_BINARY_DIR}/AVSCommon/Utils/include/AVSCommon/Utils/SDKConfig.h"
)

# Create Integration directory in output
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/Integration)

# Add utils under ThirdParty first so extensions can also utilize them
add_subdirectory("ThirdParty")

# Add extension point targets.
include(${AVS_CMAKE_BUILD}/cmake/ExtensionPath.cmake)
add_extension_projects()

# Alexa Client SDK targets.
add_subdirectory("applications")
add_subdirectory("AVSCommon")
add_subdirectory("Metrics")
add_subdirectory("ACL")
add_subdirectory("ADSL")
add_subdirectory("AFML")
add_subdirectory("Captions")
add_subdirectory("core")
add_subdirectory("AVSGatewayManager")
add_subdirectory("CertifiedSender")
add_subdirectory("Endpoints")
add_subdirectory("Settings")
add_subdirectory("BluetoothImplementations")
add_subdirectory("ContextManager")
add_subdirectory("CapabilitiesDelegate")
if (DIAGNOSTICS)
	add_subdirectory("Diagnostics")
endif()
add_subdirectory("InterruptModel")
add_subdirectory("PlaylistParser")
add_subdirectory("CapabilityAgents")
add_subdirectory("ApplicationUtilities")
add_subdirectory("MediaPlayer")
add_subdirectory("shared")
add_subdirectory("Storage")
add_subdirectory("SynchronizeStateSender")
add_subdirectory("SampleApplications")
add_subdirectory("doc")

# Create .pc pkg-config file
include(${AVS_CMAKE_BUILD}/cmake/GeneratePkgConfig.cmake)

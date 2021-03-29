# Commmon build settings across all AlexaClientSDK modules.

message(WARNING "The build directory is being deprecated.  Please use \${AVS_CMAKE_BUILD}/BuildDefaults.cmake instead.")

include(${AVS_CMAKE_BUILD}/BuildDefaults.cmake)

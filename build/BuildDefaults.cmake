# Commmon build settings across all AlexaClientSDK modules.

# Append custom CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Disallow out-of-source-builds.
include(DisallowOutOfSourceBuilds)

# Setup default build options, like compiler flags and build type.
include(BuildOptions)

# Setup code coverage environment. This must be called after BuildOptions since it uses the variables defined there.
include(CodeCoverage/CodeCoverage)

# Setup package requirement variables.
include(PackageConfigs)

# Setup logging variables.
include(Logger)

# Setup keyword requirement variables.
include(KeywordDetector)

# Setup playlist parser variables.
include(PlaylistParser)

# Setup media player variables.
include(MediaPlayer)

# Setup PortAudio variables.
include(PortAudio)

# Setup Test Options variables.
include(TestOptions)

# Setup platform dependant variables.
include (Platforms)

# Setup ESP variables.
include (ESP)

if (HAS_EXTERNAL_MEDIA_PLAYER_ADAPTERS)
    include (ExternalMediaPlayerAdapters)
endif()

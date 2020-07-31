# Commmon build settings across all AlexaClientSDK modules.

# Append custom CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

# Convenience function to get similar behaviour as CMAKE_DEPENDENT_OPTION
# for non boolean variables.
# Will mark "target" as advanced (hidden by default in most tools) unless
# "dependency" is defined and not equal to "".
function(mark_as_dependent target dependency)
    if (${dependency})
        mark_as_advanced(CLEAR ${target})
    elseif (NOT ${dependency})
        mark_as_advanced(FORCE ${target})
    endif()
endfunction(mark_as_dependent)

macro(include_once module)
    if(NOT DEFINED "BuildDefaults_Include_${module}_Set")
        set("BuildDefaults_Include_${module}_Set" ON)
        include("${module}")
    endif()
endmacro()

# Disallow out-of-source-builds.
include_once(DisallowOutOfSourceBuilds)

# Setup default build options, like compiler flags and build type.
include_once(BuildOptions)

# Setup code coverage environment. This must be called after BuildOptions since it uses the variables defined there.
include_once(CodeCoverage/CodeCoverage)

# Setup package requirement variables.
include_once(PackageConfigs)

# Setup logging variables.
include_once(Logger)

# Setup keyword requirement variables.
include_once(KeywordDetector)

# Setup media player variables.
include_once(MediaPlayer)

# Setup PortAudio variables.
include_once(PortAudio)

# Setup Curl variables.
include_once(Curl)

# Setup SQLite variables.
include_once(Sqlite)

# Setup Crypto variables.
include_once(Crypto)

# Setup Test Options variables.
include_once(TestOptions)

# Setup Bluetooth variables.
include_once(Bluetooth)

# Setup platform dependant variables.
include_once(Platforms)

# Setup Comms variables.
include_once(Comms)

# Setup PCC variables.
include_once(PCC)

# Setup MCC variables.
include_once(MCC)

# Setup android variables.
include_once(Android)

# Setup ffmpeg variables.
include_once(FFmpeg)

# Setup MRM variables.
include_once(MRM)

# Setup A4B variables.
include_once(A4B)

# Setup speech encoder variables.
include_once(SpeechEncoder)

# Setup Metrics variables.
include_once(Metrics)

# Setup Captions variables.
include_once(Captions)

# Setup Customized SDS traits variables.
include_once(CustomSDSTraits)

# Setup diagnostics variables.
include_once(Diagnostics)

# Setup Endpoint Controller capabilities
include_once(EndpointControllers)

# Functions to ExternalMediaPlayer Adapters
include_once(ExternalMediaPlayerAdapters)

# Speed up compilation using ccache
include_once(Ccache)

# Include ACS utilities
include_once(ACSUtils)

# Allow use of RTTI
include_once(UseRTTI)

# Setup rapidjson memory options.
include_once(Rapidjson)

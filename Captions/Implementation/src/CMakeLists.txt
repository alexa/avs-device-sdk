add_definitions("-DACSDK_LOG_MODULE=captions")

set(CaptionsLib_SOURCES)
list(APPEND CaptionsLib_SOURCES
        CaptionManager.cpp
        CaptionTimingAdapter.cpp
        SystemClockDelay.cpp
        TimingAdapterFactory.cpp)

if (CAPTIONS)
    list(APPEND CaptionsLib_SOURCES LibwebvttParserAdapter.cpp)
endif()

add_library(CaptionsLib SHARED ${CaptionsLib_SOURCES})

target_include_directories(CaptionsLib PUBLIC
        "${Captions_SOURCE_DIR}/Implementation/include")

if (CAPTIONS)
    target_include_directories(CaptionsLib PUBLIC
        "${LIBWEBVTT_INCLUDE_DIR}")
endif()

target_include_directories(CaptionsLib PUBLIC
        "${Captions_SOURCE_DIR}/Interface/include/ACSDK/avs_shared")

target_link_libraries(CaptionsLib
        AVSCommon
        Captions)

if (CAPTIONS)
    target_link_libraries(CaptionsLib
        "${LIBWEBVTT_LIB_PATH}")
endif()

# install target
asdk_install_target_implementation(CaptionsLib)
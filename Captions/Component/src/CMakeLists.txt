add_definitions("-DACSDK_LOG_MODULE=captions")

set(CaptionsComponent_SOURCES)
list(APPEND CaptionsComponent_SOURCES
        CaptionsComponent.cpp)

add_library(CaptionsComponent ${CaptionsComponent_SOURCES})

target_include_directories(CaptionsComponent PUBLIC
        "${Captions_SOURCE_DIR}/Component/include")

if (CAPTIONS)
    target_include_directories(CaptionsComponent PUBLIC
            "${LIBWEBVTT_INCLUDE_DIR}"
            "${Captions_SOURCE_DIR}/Implementation/include")
    target_link_libraries(CaptionsComponent CaptionsLib)
endif()

target_include_directories(CaptionsComponent PUBLIC
        "${Captions_SOURCE_DIR}/Interface/include")

target_link_libraries(CaptionsComponent
        acsdkManufactory
        acsdkShutdownManagerInterfaces
        AVSCommon
        Captions)

# install target
asdk_install_targets_with_headers("CaptionsComponent" TRUE "${Captions_SOURCE_DIR}/Component/include")

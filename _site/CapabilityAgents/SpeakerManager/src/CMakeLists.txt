add_definitions("-DACSDK_LOG_MODULE=speakerManager")

add_library(SpeakerManager SHARED SpeakerManager.cpp)

target_include_directories(SpeakerManager PUBLIC
        "${ContextManager_INCLUDE_DIRS}"
        "${SpeakerManager_SOURCE_DIR}/include")

    target_link_libraries(SpeakerManager AVSCommon)

# install target
asdk_install()

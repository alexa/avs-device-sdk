add_definitions("-DACSDK_LOG_MODULE=abstractKeywordDetector")
add_library(KWD SHARED
    AbstractKeywordDetector.cpp)
set_target_properties(KWD PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

include_directories(KWD "${KWD_SOURCE_DIR}/include")
target_link_libraries(KWD AVSCommon)

# install target
asdk_install()
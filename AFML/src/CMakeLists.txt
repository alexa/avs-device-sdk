add_library(AFML SHARED
    AudioActivityTracker.cpp
    Channel.cpp
    FocusManager.cpp
    VisualActivityTracker.cpp)
set_target_properties(AFML PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )
add_definitions("-DACSDK_LOG_MODULE=afml")
include_directories(AFML "${AFML_SOURCE_DIR}/include")
target_link_libraries(AFML AVSCommon)

# install target
asdk_install()

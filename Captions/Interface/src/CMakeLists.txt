add_definitions("-DACSDK_LOG_MODULE=captions")
add_library(Captions SHARED
        CaptionData.cpp
        CaptionLine.cpp
        CaptionFrame.cpp
        TextStyle.cpp)

target_include_directories(Captions PUBLIC
        "${Captions_SOURCE_DIR}/Interface/include")

target_link_libraries(Captions AVSCommon)

# install target
asdk_install_target_interface(Captions)
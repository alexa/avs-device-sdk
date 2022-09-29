add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaVideoCommon")

add_library(AlexaVideoCommon VideoEntityTypes.cpp)

target_include_directories(AlexaVideoCommon PUBLIC
        "${AlexaVideoCommon_SOURCE_DIR}/include")

target_link_libraries(AlexaVideoCommon AVSCommon)

# install target
asdk_install()

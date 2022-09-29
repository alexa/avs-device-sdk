add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaVideoRecorder")

add_library(acsdkAlexaVideoRecorder
        "AlexaVideoRecorderCapabilityAgent.cpp"
        "AlexaVideoRecorderFactory.cpp")

target_include_directories(acsdkAlexaVideoRecorder PRIVATE
        "${acsdkAlexaVideoRecorder_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaVideoRecorder PUBLIC
        "${acsdkAlexaVideoRecorder_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaVideoRecorder acsdkAlexaVideoRecorderInterfaces AVSCommon Alexa AlexaVideoCommon)

# install target
asdk_install()

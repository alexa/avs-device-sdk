add_definitions("-DACSDK_LOG_MODULE=acsdkAlexaRecordController")

add_library(acsdkAlexaRecordController SHARED
        AlexaRecordControllerCapabilityAgent.cpp
        AlexaRecordControllerFactory.cpp)

target_include_directories(acsdkAlexaRecordController PRIVATE
        "${acsdkAlexaRecordController_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkAlexaRecordController PUBLIC
        "${acsdkAlexaRecordController_SOURCE_DIR}/include")

target_link_libraries(acsdkAlexaRecordController AVSCommon acsdkAlexaRecordControllerInterfaces)

# install target
asdk_install()

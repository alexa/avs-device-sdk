add_definitions("-DACSDK_LOG_MODULE=acsdkSampleInteractionManager")

add_library(acsdkSampleInteractionManager
    InteractionManager.cpp
    KeywordObserver.cpp
    UIManager.cpp)

target_include_directories(acsdkSampleInteractionManager PUBLIC
    "${acsdkSampleInteractionManager_SOURCE_DIR}/include"
    "${acsdkSamplePeripheralEndpoint_SOURCE_DIR}/include"
    "${acsdkSampleConsole_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleInteractionManager
    acsdkKWDImplementations
    acsdkSampleApplicationInterfaces
    acsdkSampleConsole
    acsdkSamplePeripheralEndpoint
    AVSCommon
    DefaultClient
    RegistrationManager
)

if (PCC)
    target_link_libraries(acsdkSampleInteractionManager acsdkPhoneCallControllerAdapter)
endif()

if (MCC)
    target_link_libraries(acsdkSampleInteractionManager acsdkMeetingClientControllerAdapter)
endif()

# install target only if sample libraries are installed
asdk_sample_install()

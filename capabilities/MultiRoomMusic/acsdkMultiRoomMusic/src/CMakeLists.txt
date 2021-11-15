add_definitions("-DACSDK_LOG_MODULE=acsdkMultiRoomMusic")

add_library(acsdkMultiRoomMusic
        MRMCapabilityAgent.cpp)

target_include_directories(acsdkMultiRoomMusic PUBLIC
        "${acsdkMultiRoomMusic_SOURCE_DIR}/include")

target_link_libraries(acsdkMultiRoomMusic AVSCommon)

if (AUXCONTROLLER)
    target_link_libraries(acsdkMultiRoomMusic AuxControllerCA)
endif()

if (MRM AND MRM_STANDALONE_APP)
    target_link_libraries(acsdkMultiRoomMusic MRMHandlerProxy)
elseif (MRM)
    target_link_libraries(acsdkMultiRoomMusic MRMHandler)
endif()

# install target
asdk_install()

set(acsdkDavsClient_SOURCES
    DavsClient.cpp
    DavsEndpointHandlerV3.cpp
    DavsHandler.cpp
    )
set(acsdkDavsClient_INCLUDES
    ${acsdkDavsClient_SOURCE_DIR}/include
    )
set(acsdkDavsClient_LIBRARIES
    AVSCommon
    acsdkDavsClientInterfaces
    acsdkAssetsInterfaces
    acsdkAssetsCommon
    )

# Setup the Library first
add_library(acsdkDavsClient ${acsdkDavsClient_SOURCES})
target_include_directories(acsdkDavsClient PUBLIC ${acsdkDavsClient_INCLUDES})
target_link_libraries(acsdkDavsClient ${acsdkDavsClient_LIBRARIES})
target_compile_definitions(acsdkDavsClient PRIVATE ACSDK_LOG_MODULE=DavsClient)

# install target
asdk_install()

# Setup the Testing library
add_library(acsdkDavsClientForTesting ${acsdkDavsClient_SOURCES})
target_include_directories(acsdkDavsClientForTesting PUBLIC ${acsdkDavsClient_INCLUDES})
target_link_libraries(acsdkDavsClientForTesting ${acsdkDavsClient_LIBRARIES})
target_compile_definitions(acsdkDavsClientForTesting PUBLIC UNIT_TEST=1 PRIVATE ACSDK_LOG_MODULE=DavsClientMock)

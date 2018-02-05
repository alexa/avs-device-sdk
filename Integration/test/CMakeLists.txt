find_package(GTest ${GTEST_PACKAGE_CONFIG})

set(INCLUDE_PATH "${ACL_SOURCE_DIR}/include"
     "${AuthDelegate_INCLUDE_DIR}/test/"
     "${GTEST_INCLUDE_DIRS}"
     "${KWD_SOURCE_DIR}/include"
     "${KITTAI_SOURCE_DIR}/include"
     "${KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR}"
     "${SENSORY_SOURCE_DIR}/include"
     "${SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR}"
     "${AIP_SOURCE_DIR}/include"
     "${SpeechSynthesizer_SOURCE_DIR}/include"
     "${Alerts_SOURCE_DIR}/include"
     "${AVSSystem_SOURCE_DIR}/include"
     "${GST_INCLUDE_DIRS}"
     "${AVSCommon_INCLUDE_DIRS}"
     "${AudioPlayer_INCLUDE_DIRS}"
     "${AudioResources_INCLUDE_DIRS}"
     "${MediaPlayer_SOURCE_DIR}/include"
     "${PlaybackController_SOURCE_DIR}/include"
     "${CONTEXTMANAGER_SOURCE_DIR}/include")

    set(LINK_PATH ACL AuthDelegate AFML ADSL AIP Alerts AudioPlayer AVSSystem ContextManager KWD SpeechSynthesizer Integration gtest gmock PlaybackController)

    if(KITTAI_KEY_WORD_DETECTOR)
        SET(LINK_PATH ${LINK_PATH} KITTAI)
    endif()
    if(SENSORY_KEY_WORD_DETECTOR)
        SET(LINK_PATH ${LINK_PATH} SENSORY)
    endif()
    if(GSTREAMER_MEDIA_PLAYER)
        SET(LINK_PATH ${LINK_PATH} MediaPlayer)
        add_definitions(-DGSTREAMER_MEDIA_PLAYER)
    endif()

# Define a variable for the audio inputs.
set(INTEGRATION_INPUTS "${Integration_SOURCE_DIR}/inputs")

# Copy SDK Json file to the build directory.
set(SDK_CONFIG_FILENAME "AlexaClientSDKConfig.json")
set(SDK_CONFIG_FILE_SOURCE "${Integration_SOURCE_DIR}/${SDK_CONFIG_FILENAME}")
set(SDK_CONFIG_FILE_TARGET "${Integration_BINARY_DIR}/${SDK_CONFIG_FILENAME}")
# Use configure_file to support variable substitution later.
configure_file("${SDK_CONFIG_FILE_SOURCE}" "${SDK_CONFIG_FILE_TARGET}")

# Copy AuthServer to the build directory.
set(AUTHSERVER_SCRIPT_FILENAME "AuthServer.py")
set(AUTHSERVER_SCRIPT_FILE_SOURCE "${AlexaClientSDK_SOURCE_DIR}/tools/AuthServer/${AUTHSERVER_SCRIPT_FILENAME}")
set(AUTHSERVER_SCRIPT_FILE_TARGET "${AlexaClientSDK_BINARY_DIR}/AuthServer/${AUTHSERVER_SCRIPT_FILENAME}")
# Use configure_file to pass SDK_CONFIG_FILE_TARGET as the default config file.
configure_file("${AUTHSERVER_SCRIPT_FILE_SOURCE}" "${AUTHSERVER_SCRIPT_FILE_TARGET}")

if(BUILD_TESTING)
    set(testSourceFiles
        "${CMAKE_CURRENT_SOURCE_DIR}/AlexaAuthorizationDelegateTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AlexaCommunicationsLibraryTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AlexaDirectiveSequencerLibraryTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AudioInputProcessorIntegrationTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ServerDisconnectIntegrationTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/SpeechSynthesizerIntegrationTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AlertsIntegrationTest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/AudioPlayerIntegrationTest.cpp")
    # file(GLOB_RECURSE testSourceFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*Test.cpp")
    foreach (testSourceFile IN LISTS testSourceFiles)
        get_filename_component(testName ${testSourceFile} NAME_WE)
        add_executable(${testName} ${testSourceFile})
        target_include_directories(${testName} PUBLIC "${INCLUDE_PATH}")
        target_link_libraries(${testName} "${LINK_PATH}")
    endforeach()

    include(${Integration_BINARY_DIR}/CTestCustom.cmake OPTIONAL)
    separate_arguments(CTEST_CUSTOM_PRE_TEST UNIX_COMMAND "${CTEST_CUSTOM_PRE_TEST}")
    separate_arguments(CTEST_CUSTOM_POST_TEST UNIX_COMMAND "${CTEST_CUSTOM_POST_TEST}")
    add_custom_target(integration
            COMMAND ${CTEST_CUSTOM_PRE_TEST}
            COMMAND AlexaAuthorizationDelegateTest "${SDK_CONFIG_FILE_TARGET}" 
            COMMAND AlexaCommunicationsLibraryTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND AlexaDirectiveSequencerLibraryTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND AudioInputProcessorIntegrationTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND ServerDisconnectIntegrationTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND SpeechSynthesizerIntegrationTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND AlertsIntegrationTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND AudioPlayerIntegrationTest "${SDK_CONFIG_FILE_TARGET}" "${INTEGRATION_INPUTS}"
            COMMAND ${CTEST_CUSTOM_POST_TEST})
    message(STATUS "Please fill ${SDK_CONFIG_FILE_TARGET} before you execute integration tests.")

    if(NETWORK_INTEGRATION_TESTS AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
        set(networkTestSourceFile
            "${CMAKE_CURRENT_SOURCE_DIR}/NetworkIntegrationTests.cpp")
        get_filename_component(networkTestName ${networkTestSourceFile} NAME_WE)
        add_executable(${networkTestName} ${networkTestSourceFile})
        target_include_directories(${networkTestName} PUBLIC "${INCLUDE_PATH}")
        target_link_libraries(${networkTestName} "${LINK_PATH}")
        add_custom_target(networkIntegration
                COMMAND NetworkIntegrationTests "${SDK_CONFIG_FILE_TARGET}" "${NETWORK_INTERFACE}")
        message(STATUS "Please fill ${SDK_CONFIG_FILE_TARGET} before you execute integration tests.")
    else()
        message("Skipping build of NetworkIntegrationTests.cpp.")
    endif()
endif()

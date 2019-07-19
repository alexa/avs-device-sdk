find_package(GTest ${GTEST_PACKAGE_CONFIG})

set(INCLUDE_PATH "${ACL_SOURCE_DIR}/include"
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
     "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test"
     "${AudioPlayer_INCLUDE_DIRS}"
     "${AudioResources_INCLUDE_DIRS}"
     "${MediaPlayer_SOURCE_DIR}/include"
     "${PlaybackController_SOURCE_DIR}/include"
     "${CONTEXTMANAGER_SOURCE_DIR}/include")

    set(LINK_PATH ACL ADSL AFML AIP Alerts AudioPlayer AVSSystem CBLAuthDelegate ContextManager Integration gtest gmock KWD PlaybackController SpeechSynthesizer)

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

# Copy Adapter SDK Json file to the build directory.
set(SDK_ADAPTERS_CONFIG_FILENAME "ExternalMediaPlayerAdapters.json")
set(SDK_ADAPTERS_CONFIG_FILE_SOURCE "${Integration_SOURCE_DIR}/${SDK_ADAPTERS_CONFIG_FILENAME}")
set(SDK_ADAPTERS_CONFIG_FILE_TARGET "${Integration_BINARY_DIR}/${SDK_ADAPTERS_CONFIG_FILENAME}")

if(EXISTS "${SDK_ADAPTERS_CONFIG_FILE_SOURCE}")
    # Use configure_file to support variable substitution later.
    configure_file("${SDK_ADAPTERS_CONFIG_FILE_SOURCE}" "${SDK_ADAPTERS_CONFIG_FILE_TARGET}")
endif()

if(BUILD_TESTING)
    add_custom_target(integration
        COMMAND ${CMAKE_CTEST_COMMAND} -L Integration -C Integration --output-on-failure)
    include(${Integration_BINARY_DIR}/CTestCustom.cmake OPTIONAL)
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
        add_test(NAME ${testName}
            COMMAND ${testName} ${SDK_CONFIG_FILE_TARGET} ${INTEGRATION_INPUTS}
            CONFIGURATIONS Integration)
        set_tests_properties(${testName} PROPERTIES LABELS "Integration")
        add_dependencies(integration ${testName})
    endforeach()

    message(STATUS "Please fill ${SDK_CONFIG_FILE_TARGET} before you execute integration tests.")
    if(EXISTS "${SDK_ADAPTERS_CONFIG_FILE_SOURCE}")
        # Use configure_file to support variable substitution later.
        message(STATUS "Please fill ${SDK_ADAPTERS_CONFIG_FILE_TARGET} before you execute integration tests.")
    endif()

    if(NETWORK_INTEGRATION_TESTS AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
        set(networkTestSourceFile
            "${CMAKE_CURRENT_SOURCE_DIR}/NetworkIntegrationTests.cpp")
        get_filename_component(networkTestName ${networkTestSourceFile} NAME_WE)
        add_executable(${networkTestName} ${networkTestSourceFile})
        target_include_directories(${networkTestName} PUBLIC "${INCLUDE_PATH}")
        target_link_libraries(${networkTestName} "${LINK_PATH}")
        add_custom_target(networkIntegration
                COMMAND NetworkIntegrationTests "${SDK_CONFIG_FILE_TARGET}" "${NETWORK_INTERFACE}")
    else()
        message("Skipping build of NetworkIntegrationTests.cpp.")
    endif()
endif()

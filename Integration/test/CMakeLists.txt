find_package(GTest ${GTEST_PACKAGE_CONFIG})

set(INCLUDE_PATH "${ACL_SOURCE_DIR}/include"
     "${AuthDelegate_SOURCE_DIR}/test/"
     "${GTEST_INCLUDE_DIRS}")

set(LINK_PATH ACL AuthDelegate ADSL Integration gtest gmock)

if(BUILD_TESTING)
    file(GLOB_RECURSE testSourceFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*Test.cpp")
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
        COMMAND AlexaAuthorizationDelegateTest "${Integration_SOURCE_DIR}/AuthDelegate.config"
        COMMAND AlexaCommunicationsLibraryTest "${Integration_SOURCE_DIR}/AuthDelegate.config"
            "${Integration_SOURCE_DIR}/inputs"
        COMMAND AlexaDirectiveSequencerLibraryTest "${Integration_SOURCE_DIR}/AuthDelegate.config"
            "${Integration_SOURCE_DIR}/inputs"
        COMMAND ${CTEST_CUSTOM_POST_TEST})
endif()

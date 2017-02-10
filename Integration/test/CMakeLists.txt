cmake_minimum_required(VERSION 3.0)

find_package(GTest)

set(INCLUDE_PATH "${ACL_SOURCE_DIR}/include"
     "${AuthDelegate_SOURCE_DIR}/test/"
     "${GTEST_INCLUDE_DIRS}")

set(LINK_PATH ACL AuthDelegate Integration gtest gmock)

if(BUILD_TESTING)
    file(GLOB_RECURSE testSourceFiles RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "*Test.cpp")
    foreach (testSourceFile IN LISTS testSourceFiles)
        get_filename_component(testName ${testSourceFile} NAME_WE)
        add_executable(${testName} ${testSourceFile})
        target_include_directories(${testName} PUBLIC "${INCLUDE_PATH}")
        target_link_libraries(${testName} "${LINK_PATH}")
    endforeach()

    add_custom_target(integration COMMAND AlexaAuthorizationDelegateTest "${Integration_SOURCE_DIR}/AuthDelegate.config"
                                  COMMAND AlexaCommunicationsLibraryTest "${Integration_SOURCE_DIR}/AuthDelegate.config"
        "${Integration_SOURCE_DIR}/inputs")
endif()

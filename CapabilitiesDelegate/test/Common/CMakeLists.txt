cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_library(CapabilitiesDelegateTestLib
            TestableAuthDelegate.cpp
            TestableCapabilityProvider.cpp
            TestableHttpPut.cpp
            TestableMiscStorage.cpp)

target_include_directories(CapabilitiesDelegateTestLib PUBLIC
        "${CapabilitiesDelegate_SOURCE_DIR}/include")

target_link_libraries(CapabilitiesDelegateTestLib AVSCommon)

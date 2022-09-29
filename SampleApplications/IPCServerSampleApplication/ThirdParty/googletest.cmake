if ((DEFINED GTEST_ROOT) AND (EXISTS ${GTEST_ROOT}/googletest))
    add_subdirectory(${GTEST_ROOT} EXCLUDE_FROM_ALL ${CMAKE_BINARY_DIR}/googletest)
else()
    set(GTEST_ROOT ThirdParty/googletest-release-1.8.0)
    add_subdirectory(${GTEST_ROOT} EXCLUDE_FROM_ALL)
endif()

include_directories(${GTEST_ROOT}/googlemock/include
    ${GTEST_ROOT}/googletest/include)

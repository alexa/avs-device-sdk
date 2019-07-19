set(GTEST_ROOT ThirdParty/googletest-release-1.8.0)

add_subdirectory(${GTEST_ROOT} EXCLUDE_FROM_ALL)
include_directories(${GTEST_ROOT}/googlemock/include
    ${GTEST_ROOT}/googletest/include)

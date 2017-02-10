set(GTEST_ROOT ThirdParty/googletest-release-1.8.0)

add_subdirectory(${GTEST_ROOT})
include_directories(${GTEST_ROOT}/include)
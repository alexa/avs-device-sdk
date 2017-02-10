cmake_minimum_required(VERSION 3.0)

file(GLOB_RECURSE INTEGRATION_SRC "*.cpp" "${CMAKE_SOURCE_DIR}/Utils/src/*.cpp")
add_library(Integration SHARED "${INTEGRATION_SRC}")
target_include_directories(Integration PUBLIC "${ACL_SOURCE_DIR}/include")
target_include_directories(Integration PUBLIC "${AuthDelegate_SOURCE_DIR}/include")
target_include_directories(Integration PUBLIC "${Integration_SOURCE_DIR}/include")
target_include_directories(Integration PUBLIC "${CMAKE_SOURCE_DIR}/Utils/include")
target_link_libraries(Integration ACL AuthDelegate)

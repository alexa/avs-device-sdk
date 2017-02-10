cmake_minimum_required(VERSION 3.0)

find_package(CURL 7.43.0 REQUIRED)

message("${AVSUtils_SOURCE_DIR}")
file(GLOB_RECURSE AVSUtils_SRC "${AVSUtils_SOURCE_DIR}/src/*.cpp")
add_library(AVSUtils SHARED ${AVSUtils_SRC})
target_include_directories(AVSUtils PUBLIC ${CURL_INCLUDE_DIRS} "${AVSUtils_SOURCE_DIR}/include")
target_link_libraries(AVSUtils ${CURL_LIBRARIES})

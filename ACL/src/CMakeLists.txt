cmake_minimum_required(VERSION 3.0)

find_package(CURL 7.43.0 REQUIRED)
find_package(Threads)

message("${ACL_SOURCE_DIR}")
file(GLOB_RECURSE ACL_SRC "${ACL_SOURCE_DIR}/src/*.cpp")
add_library(ACL SHARED ${ACL_SRC})
target_include_directories(ACL PUBLIC "${CMAKE_SOURCE_DIR}/ThirdParty/MultipartParser")
target_include_directories(ACL PUBLIC ${CURL_INCLUDE_DIRS})
target_include_directories(ACL PUBLIC "${ACL_SOURCE_DIR}/include")
target_include_directories(ACL PUBLIC "${AuthDelegate_SOURCE_DIR}/include")
target_include_directories(ACL PUBLIC "${AVSUtils_SOURCE_DIR}/include")
target_link_libraries(ACL ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} AVSUtils)

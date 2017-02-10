cmake_minimum_required(VERSION 3.0)

find_package(CURL REQUIRED)
find_package(Threads)

add_library(AuthDelegate SHARED
    AuthDelegate.cpp
    Config.cpp
    HttpPost.cpp)
target_include_directories(AuthDelegate PUBLIC
    "${AuthDelegate_SOURCE_DIR}/include")
target_include_directories(AuthDelegate PRIVATE
    "${CMAKE_SOURCE_DIR}/CurlUtil/include")
target_link_libraries(AuthDelegate ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
# TODO: ACSDK-81 Figure out why AuthDelegate needs to depend upon the ACL library
# It should be enough to add "${ACL_SOURC_DIR}/include" to the include path.
target_link_libraries(AuthDelegate ACL)

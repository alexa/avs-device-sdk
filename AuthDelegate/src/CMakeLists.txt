find_package(CURL ${CURL_PACKAGE_CONFIG})
find_package(Threads ${THREADS_PACKAGE_CONFIG})

add_library(AuthDelegate SHARED
    AuthDelegate.cpp
    HttpPost.cpp)
target_include_directories(AuthDelegate PUBLIC
    "${AuthDelegate_SOURCE_DIR}/include")
target_include_directories(AuthDelegate PRIVATE
    "${CMAKE_SOURCE_DIR}/CurlUtil/include" ${RAPIDJSON_INCLUDE_DIR})
target_link_libraries(AuthDelegate ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
# TODO: ACSDK-81 Figure out why AuthDelegate needs to depend upon the ACL library
# It should be enough to add "${ACL_SOURC_DIR}/include" to the include path.
target_link_libraries(AuthDelegate ACL)

find_package(CURL ${CURL_PACKAGE_CONFIG})
find_package(Threads ${THREADS_PACKAGE_CONFIG})

add_definitions("-DACSDK_LOG_MODULE=authDelegate")
add_library(AuthDelegate SHARED
    AuthDelegate.cpp)
target_include_directories(AuthDelegate PUBLIC
    ${AuthDelegate_SOURCE_DIR}/include)
target_include_directories(AuthDelegate PRIVATE
    ${RAPIDJSON_INCLUDE_DIR})
target_link_libraries(AuthDelegate AVSCommon ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

# install target
asdk_install()

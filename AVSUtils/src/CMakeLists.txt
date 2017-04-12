find_package(CURL ${CURL_PACKAGE_CONFIG})
find_package(Threads ${THREADS_PACKAGE_CONFIG})

file(GLOB_RECURSE AVSUtils_SRC "${AVSUtils_SOURCE_DIR}/src/*.cpp")
add_library(AVSUtils SHARED ${AVSUtils_SRC})
target_include_directories(AVSUtils PUBLIC ${CURL_INCLUDE_DIRS} "${AVSUtils_SOURCE_DIR}/include")
target_link_libraries(AVSUtils ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

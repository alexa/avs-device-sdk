find_package(CURL ${CURL_PACKAGE_CONFIG})
find_package(Threads ${THREADS_PACKAGE_CONFIG})

file(GLOB_RECURSE ACL_SRC "${ACL_SOURCE_DIR}/src/*.cpp")
add_definitions("-DACSDK_LOG_MODULE=acl")
add_definitions("-DACSDK_OPENSSL_MIN_VER_REQUIRED=${OPENSSL_MIN_VERSION}")
add_library(ACL SHARED ${ACL_SRC})
target_include_directories(ACL PUBLIC "${MultipartParser_SOURCE_DIR}")
target_include_directories(ACL PUBLIC ${CURL_INCLUDE_DIRS})
target_include_directories(ACL PUBLIC "${ACL_SOURCE_DIR}/include")
target_include_directories(ACL PUBLIC "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(ACL ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} AVSCommon)

# install target
asdk_install()
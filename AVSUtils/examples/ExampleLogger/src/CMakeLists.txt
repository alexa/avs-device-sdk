find_package(Threads ${THREADS_PACKAGE_CONFIG})
add_library(ExampleLogger SHARED ExampleLogger.cpp)
target_include_directories(ExampleLogger PUBLIC
        "${AVSUtils_SOURCE_DIR}/examples/ExampleLogger/include")
target_link_libraries(ExampleLogger ${CMAKE_THREAD_LIBS_INIT} AVSUtils)

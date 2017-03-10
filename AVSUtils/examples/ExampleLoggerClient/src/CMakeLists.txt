add_executable(ExampleLoggerClient ExampleLoggerClient.cpp)
target_link_libraries(ExampleLoggerClient
        ExampleLogger
        AVSUtils)

add_library(ExampleLogger SHARED ExampleLogger.cpp)
target_include_directories(ExampleLogger PUBLIC
        "${AVSUtils_SOURCE_DIR}/examples/ExampleLogger/include")
target_link_libraries(ExampleLogger AVSUtils)


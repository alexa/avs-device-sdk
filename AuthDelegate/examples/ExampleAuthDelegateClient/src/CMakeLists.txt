add_definitions("-DACSDK_LOG_MODULE=authDelegateExampleClient")
add_executable(ExampleAuthDelegateClient
    ExampleAuthDelegateClient.cpp)
target_include_directories(ExampleAuthDelegateClient PUBLIC
    ${AuthDelegate_SOURCE_DIR}/include
    ${AVSCommon_SOURCE_DIR}/AVS/include
    ${AVSCommon_SOURCE_DIR}/Utils/include)
target_link_libraries(ExampleAuthDelegateClient
    AuthDelegate AVSCommon)

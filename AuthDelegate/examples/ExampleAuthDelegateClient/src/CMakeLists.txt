cmake_minimum_required(VERSION 3.0)

add_executable(ExampleAuthDelegateClient
    ExampleAuthDelegateClient.cpp)
target_link_libraries(ExampleAuthDelegateClient
    AuthDelegate)

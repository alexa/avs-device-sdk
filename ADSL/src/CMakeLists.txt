cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

find_package(Threads ${THREADS_PACKAGE_CONFIG})
add_library(ADSL SHARED
    DirectiveProcessor.cpp
    DirectiveRouter.cpp
    DirectiveSequencer.cpp
    HandlerAndPolicy.cpp
    MessageInterpreter.cpp
    NamespaceAndName.cpp)
target_include_directories(ADSL PUBLIC
    "${ACL_SOURCE_DIR}/include"
    "${ADSL_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(ADSL
    ${CMAKE_THREAD_LIBS_INIT}
    AVSCommon
    ACL
    AVSUtils)

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_package(Threads ${THREADS_PACKAGE_CONFIG})
add_definitions("-DACSDK_LOG_MODULE=adsl")
add_library(ADSL SHARED
    DirectiveProcessor.cpp
    DirectiveRouter.cpp
    DirectiveSequencer.cpp
    MessageInterpreter.cpp)
target_include_directories(ADSL PUBLIC
    "${ADSL_SOURCE_DIR}/include"
    "${AVSCommon_INCLUDE_DIRS}")
target_link_libraries(ADSL
    ${CMAKE_THREAD_LIBS_INIT}
    AVSCommon)

# install target
asdk_install()
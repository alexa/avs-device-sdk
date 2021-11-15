add_definitions("-DACSDK_LOG_MODULE=acsdkInputController")

set(InputController_SOURCES)
list(APPEND InputController_SOURCES
    InputControllerCapabilityAgent.cpp
    InputControllerFactory.cpp)

add_library(acsdkInputController
        "${InputController_SOURCES}")

target_include_directories(acsdkInputController PUBLIC
        "${acsdkInputController_SOURCE_DIR}/include")

target_link_libraries(acsdkInputController
        acsdkInputControllerInterfaces
        AVSCommon)

# install target
asdk_install()

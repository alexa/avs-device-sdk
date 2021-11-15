add_library(AFML
    AudioActivityTracker.cpp
    Channel.cpp
    FocusManagementComponent.cpp
    FocusManager.cpp
    VisualActivityTracker.cpp)

add_definitions("-DACSDK_LOG_MODULE=afml")
target_include_directories(AFML PUBLIC "${AFML_SOURCE_DIR}/include" "{InterruptModel_SOURCE_DIR}/include")
target_link_libraries(AFML AVSCommon InterruptModel acsdkManufactory acsdkShutdownManagerInterfaces)

# install target
asdk_install()

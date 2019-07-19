add_definitions("-DACSDK_LOG_MODULE=registrationManager")

add_library(RegistrationManager SHARED RegistrationManager.cpp CustomerDataManager.cpp CustomerDataHandler.cpp)

target_include_directories(RegistrationManager PUBLIC
    "${RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(RegistrationManager AVSCommon)

# install target
asdk_install()

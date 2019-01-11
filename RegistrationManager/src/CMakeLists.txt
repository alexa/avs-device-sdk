add_definitions("-DACSDK_LOG_MODULE=registrationManager")

add_library(RegistrationManager SHARED RegistrationManager.cpp CustomerDataManager.cpp CustomerDataHandler.cpp)
set_target_properties(RegistrationManager PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(RegistrationManager PUBLIC
    "${RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(RegistrationManager AVSCommon)

# install target
asdk_install()

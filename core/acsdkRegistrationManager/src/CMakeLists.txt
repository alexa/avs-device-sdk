add_definitions("-DACSDK_LOG_MODULE=RegistrationManager")

add_library(RegistrationManager
        CustomerDataHandler.cpp
        CustomerDataManager.cpp
        CustomerDataManagerFactory.cpp
        RegistrationManagerFactory.cpp
        RegistrationManager.cpp
        RegistrationManagerComponent.cpp
        RegistrationNotifier.cpp)

target_include_directories(RegistrationManager PUBLIC
        "${RegistrationManager_SOURCE_DIR}/include")

target_include_directories(RegistrationManager PRIVATE
        "${RegistrationManager_SOURCE_DIR}/privateInclude")

target_link_libraries(RegistrationManager PUBLIC
        acsdkManufactory
        acsdkNotifier
        RegistrationManagerInterfaces
        AVSCommon)

# install target
asdk_install()

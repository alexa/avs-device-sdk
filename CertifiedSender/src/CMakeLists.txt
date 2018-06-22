add_definitions("-DACSDK_LOG_MODULE=certifiedSender")
add_library(CertifiedSender SHARED
        CertifiedSender.cpp
        SQLiteMessageStorage.cpp)

target_include_directories(CertifiedSender PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${CertifiedSender_SOURCE_DIR}/include"
        "${RegistrationManager_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include")

target_link_libraries(CertifiedSender AVSCommon RegistrationManager SQLiteStorage)

# install target
asdk_install()
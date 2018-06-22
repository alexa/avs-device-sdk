add_definitions("-DACSDK_LOG_MODULE=Settings")

add_library(Settings SHARED
        Settings.cpp
        SettingsUpdatedEventSender.cpp
        SQLiteSettingStorage.cpp)

target_include_directories(Settings PUBLIC
        "${Settings_SOURCE_DIR}/include"
        "${SpeechSynthesizer_SOURCE_DIR}/include"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "{RegistrationManager_SOURCE_DIR}/include")

target_link_libraries(Settings AVSCommon SQLiteStorage RegistrationManager)

# install target
asdk_install()
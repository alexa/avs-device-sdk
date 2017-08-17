add_definitions("-DACSDK_LOG_MODULE=alerts")

add_library(Alerts SHARED
        Renderer/Renderer.cpp
        Storage/SQLiteAlertStorage.cpp
        Storage/SQLiteStatement.cpp
        Storage/SQLiteUtils.cpp
        Alarm.cpp
        Alert.cpp
        AlertsCapabilityAgent.cpp
        Timer.cpp)

target_include_directories(Alerts PUBLIC
        "${Alerts_SOURCE_DIR}/include"
        "${SQLITE_INCLUDE_DIRS}")

target_link_libraries(Alerts AVSCommon "${SQLITE_LDFLAGS}")

# install target
asdk_install()

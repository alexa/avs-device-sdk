add_definitions("-DACSDK_LOG_MODULE=sqliteStorage")
add_library(SQLiteStorage ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE}
        SQLiteDatabase.cpp
        SQLiteMiscStorage.cpp
        SQLiteStatement.cpp
        SQLiteUtils.cpp)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)

target_include_directories(SQLiteStorage PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${SQLITE_INCLUDE_DIRS}")

target_link_libraries(SQLiteStorage AVSCommon "${SQLITE_LDFLAGS}")

# install target
asdk_install()

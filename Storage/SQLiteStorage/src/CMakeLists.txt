add_definitions("-DACSDK_LOG_MODULE=sqliteStorage")
add_library(SQLiteStorage SHARED
        SQLiteDatabase.cpp
        SQLiteStatement.cpp
        SQLiteUtils.cpp)

set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
find_package(PkgConfig)
pkg_check_modules(SQLITE REQUIRED sqlite3)

target_include_directories(SQLiteStorage PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        "${SQLiteStorage_SOURCE_DIR}/include"
        "${SQLITE_INCLUDE_DIRS}")

target_link_libraries(SQLiteStorage AVSCommon "${SQLITE_LDFLAGS}")

# install target
asdk_install()

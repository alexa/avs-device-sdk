#
# Custom SQLite library usage.
#
# To build with a customized version of SQLite, run the following command,
#     cmake <path-to-source> 
#           -DSQLITE_LIBRARY=<path-to-sqlite-library>
#           -DSQLITE_INCLUDE_DIR=<path-to-sqlite-include-dir>
#

set(SQLITE_LIBRARY "" CACHE FILEPATH "SQLITE library path.")
set(SQLITE_INCLUDE_DIR "" CACHE PATH "SQLITE include directory.")

mark_as_advanced(SQLITE_INCLUDE_DIR SQLITE_LIBRARY)

if (("${SQLITE_LIBRARY}" STREQUAL "") OR ("${SQLITE_INCLUDE_DIR}" STREQUAL ""))
    find_package(PkgConfig)
    pkg_check_modules(SQLITE REQUIRED sqlite3)
else()
    set(SQLITE_LDFLAGS ${SQLITE_LIBRARY})
    set(SQLITE_INCLUDE_DIRS ${SQLITE_INCLUDE_DIR})
    set(SQLITE_FOUND true)
endif()

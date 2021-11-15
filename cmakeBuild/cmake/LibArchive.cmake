#
# Custom LibArchive library usage.
#
# To build with a customized version of LibArchive, run the following command,
#     cmake <path-to-source>
#           -DLibArchive_LIBRARIES=<path-to-LibArchive-library>
#           -DLibArchive_INCLUDE_DIRS=<path-to-LibArchive-include-dir>
#

set(LibArchive_LIBRARIES "" CACHE FILEPATH "LibArchive library path.")
set(LibArchive_INCLUDE_DIRS "" CACHE PATH "LibArchive include directory.")

mark_as_advanced(LibArchive_INCLUDE_DIRS LibArchive_LIBRARIES)

if (("${LibArchive_LIBRARIES}" STREQUAL "") OR ("${LibArchive_INCLUDE_DIRS}" STREQUAL ""))
    find_package(LibArchive)
else()
    set(LibArchive_FOUND true)
endif()

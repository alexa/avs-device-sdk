#
# Set up and enable FileSystemUtils in AVSCommon.
#
# FileSystemUtils is enabled by default, to build without FileSystemUtils, run the following command,
#     cmake <path-to-source>
#       -DFILE_SYSTEM_UTILS=OFF
#
# If the existing default implementation does not work for your platform and wish to provide your own custom implementation
# of the FileSystemUtils APIs, you can provide the cpp file with the following command,
#     cmake <path-to-source>
#       -DFILE_SYSTEM_UTILS_CPP_PATH=<path-to-file-system-utils-cpp-impl-if-provided-impl-do-not-work>
#

option(FILE_SYSTEM_UTILS "Enable FileSystemUtils functionality." ON)
set(FILE_SYSTEM_UTILS_CPP_PATH "" CACHE FILEPATH "Optional: Custom FileSystemUtils implementation.")
mark_as_dependent(FILE_SYSTEM_UTILS_CPP_PATH FILE_SYSTEM_UTILS)

if(FILE_SYSTEM_UTILS)
    if (FILE_SYSTEM_UTILS_CPP_PATH)
        set(FileSystemUtils_SOURCE ${FILE_SYSTEM_UTILS_CPP_PATH})
    elseif((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Android") OR (${CMAKE_SYSTEM_NAME} MATCHES "Darwin"))
        set(FileSystemUtils_SOURCE Utils/src/FileSystem/FileSystemUtilsLinux.cpp)
    elseif(WIN32)
        set(FileSystemUtils_SOURCE Utils/src/FileSystem/FileSystemUtilsWindows.cpp)
    endif()

    if ("${FileSystemUtils_SOURCE}" STREQUAL "")
        message("FileSystemUtils is not supported on this platform")
    else()
        add_definitions("-DFILE_SYSTEM_UTILS_ENABLED")
    endif()
endif()
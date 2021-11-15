#
# Extend the AVS-SDK by adding paths to search for CMake projects.
#
# Suppose that the given directory structure exists.
#    <root>/
#    ├── <avs-device-sdk>/CMakeLists.txt
#    ├── <avs-extension>/CMakeLists.txt
#    ├── <avs-extension-basedir>/
#    │   ├── <feature-a>/CMakeLists.txt
#    │   ├── <feature-b>/CMakeLists.txt
#    │   ├── <feature-c>/CMakeLists.txt
#
# The SDK can load external modules by using the EXTENSION_PATHS variable.
#     cmake <root>/<avs-device-sdk> -DEXTENSION_PATHS=<root>/<avs-extension>;<root>/<avs-extension-basedir>
#

option(EXTENSION_PATH
    "A semi-colon separated list of paths to search for CMake projects.\
    DEPRECATED: Use EXTENSION_PATHS instead.")

# Default add the core capabilities directory to the extension paths list.
set(EXTENSION_PATHS_LIST "${AVS_CORE}/capabilities" CACHE STRING
    "A semi-colon separated list of PATHSs to search for CMake projects")

macro(add_extension_projects)
    if(EXTENSION_PATH)
        list(APPEND EXTENSION_PATHS_LIST ${EXTENSION_PATH})
    endif()
    if(EXTENSION_PATHS)
        list(APPEND EXTENSION_PATHS_LIST ${EXTENSION_PATHS})
    endif()
    list(REMOVE_DUPLICATES EXTENSION_PATHS_LIST)

    foreach(EXTENSION IN LISTS EXTENSION_PATHS_LIST)
        if(IS_DIRECTORY "${EXTENSION}")
            if(EXISTS "${EXTENSION}/CMakeLists.txt")
                get_filename_component(BUILD_BASENAME "${EXTENSION}" NAME)
                add_subdirectory("${EXTENSION}" "EXTENSION/${BUILD_BASENAME}")
            else()
                file(GLOB EXTENSION_ENTRIES "${EXTENSION}/*/")
                foreach(ENTRY IN LISTS EXTENSION_ENTRIES)
                    if(IS_DIRECTORY "${ENTRY}" AND EXISTS "${ENTRY}/CMakeLists.txt")
                        get_filename_component(BUILD_BASENAME "${ENTRY}" NAME)
                        add_subdirectory("${ENTRY}" "EXTENSION/${BUILD_BASENAME}")
                    endif()
                endforeach()
            endif()
        else()
            message(WARNING "Could not find extension ${EXTENSION}")
        endif()
    endforeach()
endmacro()

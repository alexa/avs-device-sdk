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
# The SDK can load external modules by using the EXTENSION_PATH variable.
#     cmake <root>/<avs-device-sdk> -DEXTENSION_PATH=<root>/<avs-extension>;<root>/<avs-extension-basedir>
#

option(EXTENSION_PATH "A semi-colon separated list of paths to search for CMake projects")
macro(add_extension_projects)
    if (EXTENSION_PATH)
        foreach(EXTENSION IN LISTS EXTENSION_PATH)
            if(IS_DIRECTORY "${EXTENSION}" AND EXISTS "${EXTENSION}/CMakeLists.txt")
                get_filename_component(BUILD_BASENAME "${EXTENSION}" NAME)
                add_subdirectory("${EXTENSION}" "EXTENSION/${BUILD_BASENAME}")
            else()
                file(GLOB EXTENSION_ENTIRES "${EXTENSION}/*/")
                foreach(ENTRY IN LISTS EXTENSION_ENTIRES)
                    if(IS_DIRECTORY "${ENTRY}" AND EXISTS "${ENTRY}/CMakeLists.txt")
                        get_filename_component(BUILD_BASENAME "${ENTRY}" NAME)
                        add_subdirectory("${ENTRY}" "EXTENSION/${BUILD_BASENAME}")
                    endif()
                endforeach()
            endif()
        endforeach()
    endif()
endmacro()

# In:
# adapter_paths The paths to search for adapters.
#
# Description:
# This function searches for adapters provided in the EXTERNAL_MEDIA_PLAYER_ADAPTER_PATHS variable.
# Adapters root CMakeLists.txt must set variables (scoping them using PARENT_SCOPE so this function has access):
#
# EMP_ADAPTER_TARGETS
# This contains a list of targets EMP should link to.
#
# EMP_ADAPTER_REGISTRATION_FILES
# This contains the location of registration files (if necessary).
#
# Values for respective variables will be concatenated and stored into CACHE variables for other scripts to access:
# -ALL_EMP_ADAPTER_TARGETS
# -ALL_EMP_ADAPTER_REGISTRATION_FILES

function(add_external_media_player_adapters adapter_paths)
    set(ALL_EMP_ADAPTER_TARGETS "")
    set(ALL_EMP_ADAPTER_REGISTRATION_FILES "")

    foreach(ADAPTER_PATH IN LISTS adapter_paths)
        set(EMP_ADAPTER_TARGETS "")
        set(EMP_ADAPTER_REGISTRATION_FILES "")

        if(IS_DIRECTORY "${ADAPTER_PATH}")
            if(EXISTS "${ADAPTER_PATH}/CMakeLists.txt")
                get_filename_component(BUILD_BASENAME "${ADAPTER_PATH}" NAME)
                add_subdirectory("${ADAPTER_PATH}" "EMP_ADAPTERS/${BUILD_BASENAME}")
            else()
                file(GLOB EXTENSION_ENTIRES "${ADAPTER_PATH}/*/")
                foreach(ENTRY IN LISTS EXTENSION_ENTIRES)
                    if(IS_DIRECTORY "${ENTRY}" AND EXISTS "${ENTRY}/CMakeLists.txt")
                        get_filename_component(BUILD_BASENAME "${ENTRY}" NAME)
                        add_subdirectory("${ENTRY}" "EMP_ADAPTERS/${BUILD_BASENAME}")
                    endif()
                endforeach()
            endif()
        else()
            message(WARNING "Could not find adapter ${ADAPTER_PATH}")
        endif()

        if (NOT ${EMP_ADAPTER_TARGETS} STREQUAL "")
            message(STATUS "EMP Targets Found: ${EMP_ADAPTER_TARGETS}")
            list(APPEND ALL_EMP_ADAPTER_TARGETS ${EMP_ADAPTER_TARGETS})
        endif()

        if (NOT ${EMP_ADAPTER_REGISTRATION_FILES} STREQUAL "")
            message(STATUS "EMP Registration Files Found: ${EMP_ADAPTER_REGISTRATION_FILES}")
            list(APPEND ALL_EMP_ADAPTER_REGISTRATION_FILES ${EMP_ADAPTER_REGISTRATION_FILES})
        endif()

    endforeach()

    set(ALL_EMP_ADAPTER_TARGETS ${ALL_EMP_ADAPTER_TARGETS} CACHE INTERNAL "" FORCE)
    set(ALL_EMP_ADAPTER_REGISTRATION_FILES ${ALL_EMP_ADAPTER_REGISTRATION_FILES} CACHE INTERNAL "" FORCE)
endfunction()

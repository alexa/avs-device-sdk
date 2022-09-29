cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Add all subdirectories within the current path
file(GLOB DIRECTORY_ENTRIES "${CMAKE_CURRENT_LIST_DIR}/*/")
foreach(DIRECTORY IN LISTS DIRECTORY_ENTRIES)
    if(IS_DIRECTORY "${DIRECTORY}")
        if(EXISTS "${DIRECTORY}/CMakeLists.txt")
            add_subdirectory("${DIRECTORY}")
        else()
            message(WARNING "Could not find CMakeLists.txt within ${DIRECTORY}")
        endif()
    endif()
endforeach()
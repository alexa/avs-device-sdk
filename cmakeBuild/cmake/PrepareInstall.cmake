#
# Setup variables and functions for target install and generation of the .pc pkg_config
# file for the SDK.
#

# Function to install the target library only.
function(asdk_install_lib)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    asdk_set_soname(${PROJECT_NAME})
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
endfunction()

# Function to install the target
function(asdk_install)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    asdk_set_soname(${PROJECT_NAME})
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
endfunction()

# Function to install the sample target
function(asdk_sample_install)
    if (INSTALL_COMMON_SAMPLE_LIBS)
        asdk_install()
    endif()
endfunction()

# Function to install an interface library
function(asdk_install_interface)
    install(DIRECTORY include DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
endfunction()

# Function to install the given set of targets
function(asdk_install_targets targets install_headers)
    foreach(target IN LISTS targets)
        SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${target}" CACHE INTERNAL "" FORCE)
        asdk_set_soname(${target})
        install(TARGETS "${target}" DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    endforeach()

    if (${install_headers})
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
    endif()
endfunction()

# Function to install the implementation of the given target
function(asdk_install_target_implementation target)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${target}" CACHE INTERNAL "" FORCE)
    asdk_set_soname(${target})
    install(TARGETS "${target}" DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/Implementation/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
endfunction()

# Function to install the interface of the given target
function(asdk_install_target_interface target)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${target}" CACHE INTERNAL "" FORCE)
    asdk_set_soname(${target})
    install(TARGETS "${target}" DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/Interface/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
endfunction()

# Function to install the given set of targets
function(asdk_install_targets targets install_headers)
    foreach(target IN LISTS targets)
        SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${target}" CACHE INTERNAL "" FORCE)
        asdk_set_soname(${target})
        install(TARGETS "${target}" DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    endforeach()

    if (${install_headers})
        install(DIRECTORY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
    endif()
endfunction()

# Function to install the given set of targets with header location
function(asdk_install_targets_with_headers targets install_headers header_location)
    foreach(target IN LISTS targets)
        SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${target}" CACHE INTERNAL "" FORCE)
        install(TARGETS "${target}" DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    endforeach()

    if (${install_headers})
        message(${header_location})
        install(DIRECTORY "${header_location}" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
    endif()
endfunction()


# Function to install the target with list of include paths
function(asdk_install_multiple path_list)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    asdk_set_soname(${PROJECT_NAME})
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    foreach(path IN LISTS path_list)
        install(DIRECTORY ${path} DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
    endforeach()
endfunction()

# Function to set the output name on the given target and include the specified suffix
function(asdk_set_soname target)
    if (NOT ${TARGET_SUFFIX} EQUAL "")
        set_target_properties(${target} PROPERTIES OUTPUT_NAME ${target}${TARGET_SUFFIX})
    endif()
endfunction()

# Setup pkg_config variables
SET(PKG_CONFIG_REQUIRES "libcurl sqlite3")
if(GSTREAMER_MEDIA_PLAYER)
    SET(PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES} gstreamer-1.0 gstreamer-app-1.0")
endif()
if(BLUETOOTH_BLUEZ)
    SET(PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES} gio-2.0")
    SET(PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES} gio-unix-2.0")
    SET(PKG_CONFIG_REQUIRES "${PKG_CONFIG_REQUIRES} sbc")
endif()
SET(PKG_CONFIG_LIBS         "-L\${libdir}" CACHE INTERNAL "" FORCE)
SET(PKG_CONFIG_LIBDIR       "\${prefix}/lib")
SET(PKG_CONFIG_INCLUDEDIR   "\${prefix}/include")
SET(PKG_CONFIG_CFLAGS       "-I\${includedir}")

# Set library and header files install directory
SET(ASDK_LIB_INSTALL_DIR     "${CMAKE_INSTALL_PREFIX}/lib")
SET(ASDK_INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}")

# Use this RPATH when installing.
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Set suffix to append to output targets
set(TARGET_SUFFIX "" CACHE INTERNAL "Suffix output targets with this string")


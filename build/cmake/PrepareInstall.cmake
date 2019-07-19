#
# Setup variables and functions for target install and generation of the .pc pkg_config
# file for the SDK.
#

# Function to install the target library only.
function(asdk_install_lib)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
endfunction()

# Function to install the target
function(asdk_install)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/include" DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
endfunction()

# Function to install the target with list of include paths
function(asdk_install_multiple path_list)
    SET(PKG_CONFIG_LIBS "${PKG_CONFIG_LIBS} -l${PROJECT_NAME}" CACHE INTERNAL "" FORCE)
    install(TARGETS ${PROJECT_NAME} DESTINATION "${ASDK_LIB_INSTALL_DIR}")
    foreach(path IN LISTS path_list)
        install(DIRECTORY ${path} DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}")
    endforeach()
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

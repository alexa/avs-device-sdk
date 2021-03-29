#
# Setup the rpath for target.
#
# To specify the rpath:
#     cmake <path-to-source>
#           -DTARGET_RPATH=<rpath-path>
#
option(TARGET_RPATH "Setting the rpath for SampleApp")

macro(add_rpath_to_target targetName)
    if(TARGET_RPATH)
        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
            target_link_libraries(${targetName} "-rpath ${TARGET_RPATH}")
        elseif((${CMAKE_SYSTEM_NAME} MATCHES "Linux") OR (${CMAKE_SYSTEM_NAME} MATCHES "Android"))
            target_link_libraries(${targetName} "-Wl,-rpath,${TARGET_RPATH}")
        endif()
    endif()
endmacro()


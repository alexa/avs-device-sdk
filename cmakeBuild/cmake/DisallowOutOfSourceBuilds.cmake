#
# Force out-of-source builds.
#
# Out-of-source builds are highly recommended because CMake does not track any files generated or created as part of
# running CMake.
#
# See https://cmake.org/Wiki/CMake_FAQ#CMake_does_not_generate_a_.22make_distclean.22_target._Why.3F for more details.
#
function(DisallowOutOfSourceBuilds)
    # Resolve any symlinks
    get_filename_component(sourceDir "${CMAKE_SOURCE_DIR}" REALPATH)
    get_filename_component(buildDir "${CMAKE_BINARY_DIR}" REALPATH)

    # Iterate over the build directory and its parents until the build and source directories match or the build
    # directory is the root directory.
    while((NOT buildDir STREQUAL sourceDir) AND (NOT buildDir STREQUAL lastBuildDir))
        set(lastBuildDir "${buildDir}")
        get_filename_component(buildDir "${buildDir}" DIRECTORY)
    endwhile()

    if(buildDir STREQUAL sourceDir)
        string(LENGTH "${sourceDir}" sourceDirLen)
        math(EXPR sourceDirPadLen "72 - ${sourceDirLen}")
        if(sourceDirPadLen GREATER 0)
            string(RANDOM LENGTH "${sourceDirPadLen}" ALPHABET " " sourceDirPad)
        endif()

        get_filename_component(buildDir "${CMAKE_BINARY_DIR}" REALPATH)
        string(LENGTH "${buildDir}" buildDirLen)
        math(EXPR buildDirPadLen "72 - ${buildDirLen}")
        if(buildDirPadLen GREATER 0)
            string(RANDOM LENGTH "${buildDirPadLen}" ALPHABET " " buildDirPad)
        endif()

        message("###############################################################################")
        message("#                                                                             #")
        message("# ERROR:                                                                      #")
        message("# AlexaClientSDK must not be built in the AlexaClientSDK source directory.    #")
        message("#                                                                             #")
        message("# You must run cmake in a separate build directory completely outside the     #")
        message("# the source directory.                                                       #")
        message("#                                                                             #")
        message("# Source Directory:                                                           #")
        message("#     ${sourceDir}${sourceDirPad}#")
        message("# Build Directory:                                                            #")
        message("#     ${buildDir}${buildDirPad}#")
        message("#                                                                             #")
        message("# NOTE:                                                                       #")
        message("# Since you have tried to create an in-source build already, CMake may have   #")
        message("# created some files and directories in the source tree. To find these files, #")
        message("# run:                                                                        #")
        message("#     $ git status                                                            #")
        message("#                                                                             #")
        message("# To remove these files, simply use 'rm' on the appropriate files and         #")
        message("# directories.                                                                #")
        message("#                                                                             #")
        message("###############################################################################")

        message(FATAL_ERROR "In-source-build detected, quitting!")
    endif()
endfunction()

DisallowOutOfSourceBuilds()

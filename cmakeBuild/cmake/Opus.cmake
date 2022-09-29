#
# Setup the OPUS compiler options.
#
# To build with OPUS support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -DOPUS=ON
#

option(OPUS "Enable OPUS encoding." OFF)

if (OPUS)
    find_path(OPUS_INCLUDE_DIR opus)
    find_library(OPUS_LIBRARY opus)
    if(NOT OPUS_LIBRARY)
        message(FATAL_ERROR "Cannot find libopus")
    endif()
    if (NOT EXISTS "${OPUS_INCLUDE_DIR}/opus/opus.h")
        message(FATAL_ERROR "Cannot find opus.h")
    endif()
    set(OPUS_INCLUDE_DIR ${OPUS_INCLUDE_DIR}/opus)

    message(STATUS "OPUS audio encoder is enabled")

    message(STATUS "\tLibrary Path: ${OPUS_LIBRARY}")
    message(STATUS "\tInclude Path: ${OPUS_INCLUDE_DIR}/")
else()
    message(STATUS "OPUS audio encoder is disabled")
endif()

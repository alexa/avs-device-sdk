#
# Setup the MRM compiler options.
#
# To build with MRM support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -DMRM=ON
#           -DMRM_LIB_PATH=<path-to-mrm-lib>
#           -DMRM_INCLUDE_DIR=<path-to-mrm-include-dir>
#

option(MRM "Enable Multi-Room-Music (MRM)." OFF)

if(MRM)
    if(NOT MRM_LIB_PATH)
        message(FATAL_ERROR "Must pass path to the external library to enable MRM support.")
    endif()
    if(NOT MRM_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include directory path to enable MRM support.")
    endif()
    message("Creating ${PROJECT_NAME} with Multi-Room-Music (MRM)")
    add_definitions(-DENABLE_MRM)
endif()
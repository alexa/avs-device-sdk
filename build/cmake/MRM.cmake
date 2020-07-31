#
# Setup the MRM compiler options.
#
# To build with MRM support, include the following option on the cmake command line.
#     cmake <path-to-source>
#       -DMRM=ON
#           -DMRM_LIB_PATH=<path-to-mrm-lib>
#           -DMRM_INCLUDE_DIR=<path-to-mrm-include-dir>
#           -DMRM_ESDK_LIB_PATH=<path-to-mrm-esdk-lib>
#
# To build with MRMApp, also include the following option on the cmake command line.
#       -DMRM_STANDALONE_APP=ON
#           -DNANOPB_LIB_PATH=<path-to-nanopb-lib>
#           -DNANOPB_INCLUDE_PATH=<path-to-nanopb-include>
#

option(MRM "Enable Multi-Room-Music (MRM)." OFF)
option(MRM_STANDALONE_APP "Enable Multi-Room Music (MRM) as a standalone app." OFF)

if(MRM)
    if(NOT MRM_LIB_PATH)
        message(FATAL_ERROR "Must pass path to the external library to enable MRM support.")
    endif()
    if(NOT MRM_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include directory path to enable MRM support.")
    endif()
    if (NOT MRM_ESDK_LIB_PATH)
        message(FATAL_ERROR "Must pass path to the eSDK library to enable MRM support.")
    endif()
    message("Creating ${PROJECT_NAME} with Multi-Room-Music (MRM).")
    add_definitions(-DENABLE_MRM)

    if(MRM_STANDALONE_APP)
        if (NOT NANOPB_LIB_PATH)
            message(FATAL_ERROR "Must pass path to the Nanopb library.")
        endif()
        if (NOT NANOPB_INCLUDE_PATH)
            message(FATAL_ERROR "Must pass path to the Nanopb include.")
        endif()
        message("Creating ${PROJECT_NAME} with Multi-Room-Music (MRM) App.")
        add_definitions(-DENABLE_MRM_STANDALONE_APP)
    endif()
endif()
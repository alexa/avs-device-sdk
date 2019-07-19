cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
        "${MRM_INCLUDE_DIRS}"
        "${AVSCommon_SOURCE_DIR}/AVS/test"
        "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test")

if (MRM)
    find_package(ALSA REQUIRED)
    if (ALSA_FOUND)
        discover_unit_tests("${INCLUDE_PATH}" "MRM;UtilsCommonTestLib;${ALSA_LIBRARIES}")
    else ()
        message(FATAL_ERROR, "ALSA library is required for MRM tests. Cannot proceed.")
    endif(ALSA_FOUND)
endif (MRM)
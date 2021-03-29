#
# Switch to allow the AVS CPP SDK to use RTTI
#
# Allow the AVS CPP SDK to use RTTI
#     -DACSDK_USE_RTTI=ON
#

if (WIN32)
    option(ACSDK_USE_RTTI "Allow AVS CPP SDK to use RTTI" ON)
    if (NOT ACSDK_USE_RTTI)
        message(FATAL_ERROR "FATAL ERROR: Use of RTTI is required for WIN32 builds.")
    endif()
else()
    option(ACSDK_USE_RTTI "Allow AVS CPP SDK to use RTTI" OFF)
endif()

if (ACSDK_USE_RTTI)
    message(STATUS "AVS CPP SDK code allowed to use RTTI.")
    add_definitions(-DACSDK_USE_RTTI)
endif()

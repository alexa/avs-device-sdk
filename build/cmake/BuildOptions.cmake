#
# Setup the build type and compiler options.
#
# To set the build type, run the following command with a build type of DEBUG, RELEASE, or MINSIZEREL:
#     cmake <path-to-source> -DCMAKE_BUILD_TYPE=<build-type>
#

# If no build type is specified by specifying it on the command line, default to debug.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: DEBUG, RELEASE, or MINSIZEREL." FORCE)
    message("No build type specified, defaulting to Release.")
endif()

# Verify the build type is valid.
set(buildTypes DEBUG RELEASE MINSIZEREL)

string(TOUPPER "${CMAKE_BUILD_TYPE}" buildType)

list(FIND buildTypes "${buildType}" buildTypeFound)
if (buildTypeFound EQUAL -1)
    string(LENGTH "${CMAKE_BUILD_TYPE}" buildTypeLen)
    math(EXPR buildTypePadLen "72 - ${buildTypeLen}")
    if (buildTypePadLen GREATER 0)
        string(RANDOM LENGTH "${buildTypePadLen}" ALPHABET " " buildTypePad)
    endif()

    message("###############################################################################")
    message("#                                                                             #")
    message("# ERROR:                                                                      #")
    message("# Unknown build type selected. Please select from DEBUG, RELEASE, or          #")
    message("# MINSIZEREL.                                                                 #")
    message("#                                                                             #")
    message("# Build Type:                                                                 #")
    message("#     ${buildType}${buildTypePad}#")
    message("#                                                                             #")
    message("###############################################################################")

    message(FATAL_ERROR "Unknown build type ${buildType}. Please select from DEBUG, RELEASE, or MINSIZEREL. Quitting!")
else()
    message("Creating the build directory for the ${PROJECT_NAME} with build type: ${buildType}")
endif()

# Set up the compiler flags.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine the platform and compiler dependent flags.
if (UNIX OR CMAKE_COMPILER_IS_GNUCXX)
    set(CXX_PLATFORM_DEPENDENT_FLAGS_DEBUG      "-DDEBUG -DACSDK_DEBUG_LOG_ENABLED -Wall -Werror -Wsign-compare -g")
    set(CXX_PLATFORM_DEPENDENT_FLAGS_RELEASE    "-DNDEBUG -Wall -Werror -O2")
    set(CXX_PLATFORM_DEPENDENT_FLAGS_MINSIZEREL "-DNDEBUG -Wall -Werror -Os")
elseif(MSVC)
    set(CXX_PLATFORM_DEPENDENT_FLAGS_DEBUG      "/DDEBUG -DACSDK_DEBUG_LOG_ENABLED /W4 /WX /Zi")
    set(CXX_PLATFORM_DEPENDENT_FLAGS_RELEASE    "/DNDEBUG /W4 /WX /O2")
    set(CXX_PLATFORM_DEPENDENT_FLAGS_MINSIZEREL "/DNDEBUG /W4 /WX /O1")
endif()

# Debug build, default.
set(CMAKE_CXX_FLAGS_DEBUG "${CXX_PLATFORM_DEPENDENT_FLAGS_DEBUG} -DRAPIDJSON_HAS_STDSTRING" CACHE INTERNAL "Flags used for DEBUG builds" FORCE)
set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} CACHE INTERNAL "Flags used for DEBUG builds" FORCE)

# Release build.
set(CMAKE_CXX_FLAGS_RELEASE "${CXX_PLATFORM_DEPENDENT_FLAGS_RELEASE} -DRAPIDJSON_HAS_STDSTRING" CACHE INTERNAL "Flags used for RELEASE builds" FORCE)
set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} CACHE INTERNAL "Flags used for RELEASE builds" FORCE)

# Minimum sized release build.
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CXX_PLATFORM_DEPENDENT_FLAGS_MINSIZEREL} -DRAPIDJSON_HAS_STDSTRING" CACHE INTERNAL "Flags used for minimum sized RELEASE builds" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_RELEASE} CACHE INTERNAL "Flags used for minimum sized RELEASE builds" FORCE)

if (ACSDK_LATENCY_LOG)
    add_definitions(-DACSDK_LATENCY_LOG_ENABLED)
endif()

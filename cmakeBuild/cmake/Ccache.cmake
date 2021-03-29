#
# Set up Ccache to speedup compilation
#
# To build with Ccache, use the following flag.
#       -DUSE_CCACHE=ON
#

option(USE_CCACHE "Use ccache to speed up compilation" OFF)
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Using ccache")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    else()
        message(WARNING "CCACHE requested to be used but ccache program not found")
    endif()
endif()

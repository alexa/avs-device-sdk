# Setup ducking compiler options.
#
# To disable local ducking, specify:
# cmake <path-to-source> -DDISABLE_DUCKING

option(DISABLE_DUCKING "Provides option to disable ducking" OFF)

if (DISABLE_DUCKING)
    add_definitions(-DDISABLE_DUCKING)
endif()

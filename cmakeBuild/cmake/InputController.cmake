#
# Setup the InputController compiler options.
#
# To build with InputController capabilities, specify:
# cmake <path-to-source> -DINPUT_CONTROLLER=ON

option(INPUT_CONTROLLER "Enable the Input Controller functionality" OFF)

if(INPUT_CONTROLLER)
    message("Enabling InputController CapabilityAgent")
    add_definitions(-DENABLE_INPUT_CONTROLLER)
endif()

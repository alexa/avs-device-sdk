#
# Setup the endpoint controller capabilities compiler options.
#
# To build with all endpoint controller capabilities, specify:
#   cmake <path-to-source> 
#       -DENABLE_ALL_ENDPOINT_CONTROLLERS=ON
#
# Or, you could specify each controller capabilities`
#   cmake <path-to-source> 
#       -DENDPOINT_CONTROLLERS_POWER_CONTROLLER=ON
#       -DENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER=ON
#       -DENDPOINT_CONTROLLERS_RANGE_CONTROLLER=ON
#       -DENDPOINT_CONTROLLERS_MODE_CONTROLLER=ON
#
# Also, you can specify all and opt out capabilities which are not required,
# by specifying it as 'OFF'. For instance, you can enable all except the Toggle 
# capability by setting -DENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER=OFF
# For example:
#   cmake <path-to-source> 
#       -DENABLE_ALL_ENDPOINT_CONTROLLERS=ON
#           -DENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER=OFF
#

# Default is OFF
option(ENABLE_ALL_ENDPOINT_CONTROLLERS "Enable all endpoint controller capabilities" OFF)

# Enable controller capabilities based on the 'ENABLE_ALL_ENDPOINT_CONTROLLERS'
if(ENABLE_ALL_ENDPOINT_CONTROLLERS)
     cmake_dependent_option(ENDPOINT_CONTROLLERS_POWER_CONTROLLER "Enable endpoint controller Power Controller capability" ON "ENABLE_ALL_ENDPOINT_CONTROLLERS" OFF)
     cmake_dependent_option(ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER "Enable endpoint controller Toggle Controller capability" ON "ENABLE_ALL_ENDPOINT_CONTROLLERS" OFF)
     cmake_dependent_option(ENDPOINT_CONTROLLERS_RANGE_CONTROLLER "Enable endpoint controller Range Controller capability" ON "ENABLE_ALL_ENDPOINT_CONTROLLERS" OFF)
     cmake_dependent_option(ENDPOINT_CONTROLLERS_MODE_CONTROLLER "Enable endpoint controller Mode Controller capability" ON "ENABLE_ALL_ENDPOINT_CONTROLLERS" OFF)
endif()

if(ENDPOINT_CONTROLLERS_POWER_CONTROLLER)
     message("Creating ${PROJECT_NAME} with Power Controller capability")
     add_definitions(-DPOWER_CONTROLLER)
endif()

if(ENDPOINT_CONTROLLERS_TOGGLE_CONTROLLER)
     message("Creating ${PROJECT_NAME} with Toggle Controller capability")
     add_definitions(-DTOGGLE_CONTROLLER)
endif()

if(ENDPOINT_CONTROLLERS_RANGE_CONTROLLER)
     message("Creating ${PROJECT_NAME} with Range Controller capability")
     add_definitions(-DRANGE_CONTROLLER)
endif()

if(ENDPOINT_CONTROLLERS_MODE_CONTROLLER)
     message("Creating ${PROJECT_NAME} with Mode Controller capability")
     add_definitions(-DMODE_CONTROLLER)
endif()



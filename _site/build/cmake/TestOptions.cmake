option(NETWORK_INTEGRATION_TESTS "Enable network integration tests for ACL." OFF)

if(NETWORK_INTEGRATION_TESTS AND (${CMAKE_SYSTEM_NAME} MATCHES "Linux"))
    if(NOT NETWORK_INTERFACE)
        message(FATAL_ERROR "Must pass network interface")
    endif()
    add_definitions(-DNETWORK_INTEGRATION_TESTS)
endif()
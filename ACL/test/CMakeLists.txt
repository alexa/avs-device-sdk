add_subdirectory("Transport")

set(LIBRARIES ACL ACLTransportCommonTestLib ${CMAKE_THREAD_LIBS_INIT})

discover_unit_tests("${ACL_SOURCE_DIR}/include" "${LIBRARIES}")

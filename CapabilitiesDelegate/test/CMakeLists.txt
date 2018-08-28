add_subdirectory("Common")

set(LIBRARIES CapabilitiesDelegate CapabilitiesDelegateTestLib ${CMAKE_THREAD_LIBS_INIT})
discover_unit_tests("${CapabilitiesDelegate_SOURCE_DIR}/include" "${LIBRARIES}")

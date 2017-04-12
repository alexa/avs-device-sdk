find_package(Threads ${THREADS_PACKAGE_CONFIG})

set(LIBRARIES AVSUtils ${CMAKE_THREAD_LIBS_INIT})

discover_unit_tests("${AVSUtils_SOURCE_DIR}/include" "${LIBRARIES}")

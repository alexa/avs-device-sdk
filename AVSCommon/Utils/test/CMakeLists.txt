cmake_minimum_required(VERSION 3.0)

find_package(Threads ${THREADS_PACKAGE_CONFIG})

set(INCLUDES "${AVSCommon_SOURCE_DIR}/Utils/include")

set(LIBRARIES AVSCommon ${CMAKE_THREAD_LIBS_INIT})

discover_unit_tests("${INCLUDES}" "${LIBRARIES}")
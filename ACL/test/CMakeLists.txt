add_subdirectory("Transport")

set(LIBRARIES ACL ACLTransportCommonTestLib ${CMAKE_THREAD_LIBS_INIT})
set(INCLUDE_PATH ${AVSCommon_INCLUDE_DIRS} "${ACL_SOURCE_DIR}/include")
discover_unit_tests( "${INCLUDE_PATH}" "${LIBRARIES}")

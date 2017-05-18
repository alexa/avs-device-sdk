cmake_minimum_required(VERSION 3.0)

add_subdirectory("Attachment")

find_package(Threads ${THREADS_PACKAGE_CONFIG})

set(INCLUDES "${AVSCommon_SOURCE_DIR}/AVS/include")

set(LIBRARIES AVSCommon AttachmentCommonTestLib ${CMAKE_THREAD_LIBS_INIT})

discover_unit_tests("${INCLUDES}" "${LIBRARIES}")

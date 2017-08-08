cmake_minimum_required(VERSION 3.0)

find_package(Threads ${THREADS_PACKAGE_CONFIG})

add_definitions("-DACSDK_LOG_MODULE=playlistParserTest")

set(INCLUDES "${PlaylistParser_SOURCE_DIR}/include")

set(LIBRARIES PlaylistParser ${CMAKE_THREAD_LIBS_INIT})

set(INPUT_FOLDER "${PlaylistParser_SOURCE_DIR}/inputs")

discover_unit_tests("${INCLUDES}" "${LIBRARIES}" "${INPUT_FOLDER}")

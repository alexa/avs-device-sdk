cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(LIBS acsdkKWDImplementations acsdkNotifierTestLib)

discover_unit_tests("${acsdkKWDImplementations_SOURCE_DIR}/include" "${LIBS}")

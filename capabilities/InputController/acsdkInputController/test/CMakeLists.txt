cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

set(INCLUDE_PATH
    "${acsdkInputController_SOURCE_DIR}/include")

set(LIBS
    "acsdkInputController"
    "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")

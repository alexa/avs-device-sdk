cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(LIBRARIES acsdkSDKClient SDKInterfacesTests)

discover_unit_tests("" "${LIBRARIES}")

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
	"${AVSCommon_SOURCE_DIR}/AVS/test"
	"${AVSCommon_SOURCE_DIR}/Utils/test"
	"${AVSCommon_INCLUDE_DIRS}"
	"${Diagnostics_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "Diagnostics;UtilsCommonTestLib;SDKInterfacesTests")

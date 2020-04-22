add_subdirectory("Common")

set(INCLUDE_PATH
	"${AVSCommon_INCLUDE_DIRS}")

discover_unit_tests("${INCLUDE_PATH}" "AVSCommon;UtilsCommonTestLib;SDKInterfacesTests")

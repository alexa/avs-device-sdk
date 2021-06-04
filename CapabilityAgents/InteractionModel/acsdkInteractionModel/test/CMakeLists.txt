cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH 
        "${acsdkInteractionModel_SOURCE_DIR}/include"
        "${acsdkInteractionModel_SOURCE_DIR}/privateInclude"
        "${AVSCommon_SOURCE_DIR}/Utils/test")

discover_unit_tests("${INCLUDE_PATH}" "acsdkInteractionModel;acsdkInteractionModelInterfaces;UtilsCommonTestLib;SDKInterfacesTests")

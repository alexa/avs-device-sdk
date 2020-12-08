cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

SET(LIBRARIES
        acsdkAlerts
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkManufactory
        acsdkShutdownManagerInterfaces
        ApplicationAudioPipelineFactoryTestLib
        AVSCommon
        AVSCommonTestLib
        DeviceSettingsTests
        RegistrationManager
        UtilsCommonTestLib
        SDKInterfacesTests)

discover_unit_tests("" "${LIBRARIES}")

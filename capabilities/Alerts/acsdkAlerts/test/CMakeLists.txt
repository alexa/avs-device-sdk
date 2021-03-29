cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

SET(LIBRARIES
        acsdkAlerts
        acsdkApplicationAudioPipelineFactoryInterfaces
        acsdkManufactory
        acsdkShutdownManagerInterfaces
        acsdkSystemClockMonitorInterfaces
        ApplicationAudioPipelineFactoryTestLib
        AVSCommon
        AVSCommonTestLib
        DeviceSettingsTests
        RegistrationManager
        UtilsCommonTestLib
        ShutdownManagerTestLib
        SDKInterfacesTests)

discover_unit_tests("" "${LIBRARIES}")

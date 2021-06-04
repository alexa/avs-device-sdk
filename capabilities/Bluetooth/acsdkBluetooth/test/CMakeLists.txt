cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(LIBS
        "acsdkBluetoothInterfacesTestUtils"
        "acsdkBluetooth"
        "AttachmentCommonTestLib"
        "UtilsCommonTestLib"
        "RegistrationManagerTestUtils"
        "SDKInterfacesTests"
        "ShutdownManagerTestLib"
        "ApplicationAudioPipelineFactoryTestLib")

discover_unit_tests("" "${LIBS}")

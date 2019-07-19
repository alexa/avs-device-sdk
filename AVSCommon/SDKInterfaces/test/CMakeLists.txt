add_library(SDKInterfacesTests
        src/EqualizerStorageInterfaceTest.cpp
        src/StubMiscStorage.cpp
        )

target_include_directories(SDKInterfacesTests PUBLIC
        "${AVSCommon_INCLUDE_DIRS}"
        )

target_link_libraries(SDKInterfacesTests AVSCommon gmock_main)


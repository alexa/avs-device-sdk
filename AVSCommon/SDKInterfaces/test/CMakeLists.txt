if (BUILD_TESTING)
    add_library(SDKInterfacesTests
            src/StubMiscStorage.cpp
            )

    target_include_directories(SDKInterfacesTests PUBLIC
            "${AVSCommon_INCLUDE_DIRS}"
            )

    target_link_libraries(SDKInterfacesTests AVSCommon gmock_main)

endif()

if (BUILD_MOCKS)
    install(DIRECTORY "${AVSCommon_SOURCE_DIR}/SDKInterfaces/test/AVSCommon"
            DESTINATION "${ASDK_INCLUDE_INSTALL_DIR}/include")
endif()

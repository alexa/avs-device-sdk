add_library(KeywordDetectorProvider
    KeywordDetectorProvider.cpp
    ${KWD_ADAPTER_REGISTRATION_FILE})

target_include_directories(KeywordDetectorProvider PUBLIC
    "${KeywordDetectorProvider_SOURCE_DIR}/include/acsdkKWDProvider")

target_link_libraries(KeywordDetectorProvider AVSCommon acsdkKWDImplementations)

if(TARGET_KWD_LIB)
    target_link_libraries(KeywordDetectorProvider ${TARGET_KWD_LIB})
else()
    message(FATAL_ERROR "No KWD Target set")
endif()

# install target
asdk_install()

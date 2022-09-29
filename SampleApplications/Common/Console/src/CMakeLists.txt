add_definitions("-DACSDK_LOG_MODULE=acsdkSampleConsole")

add_library(acsdkSampleConsole
    ConsolePrinter.cpp)

target_include_directories(acsdkSampleConsole PUBLIC
    "${acsdkSampleConsole_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleConsole
    AVSCommon
)

# install target only if sample libraries are installed
asdk_sample_install()

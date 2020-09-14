add_definitions("-DACSDK_LOG_MODULE=SoftwareComponentReporter")

add_library(SoftwareComponentReporter SHARED
    SoftwareComponentReporterCapabilityAgent.cpp)

target_include_directories(SoftwareComponentReporter PUBLIC "${SoftwareComponentReporter_SOURCE_DIR}/include")

target_link_libraries(SoftwareComponentReporter
    AVSCommon)

# install target
asdk_install()
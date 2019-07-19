add_definitions("-DACSDK_LOG_MODULE=mrm")

add_library(MRM SHARED
        MRMCapabilityAgent.cpp)

target_include_directories(MRM PUBLIC
        "${MRM_SOURCE_DIR}/include")

target_link_libraries(MRM AVSCommon)

if (MRM)
    target_link_libraries(MRM MRMHandler)
endif()

# install target
asdk_install()

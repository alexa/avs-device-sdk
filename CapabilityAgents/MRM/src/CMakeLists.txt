add_definitions("-DACSDK_LOG_MODULE=mrm")

add_library(MRM SHARED
        MRMCapabilityAgent.cpp)

set_target_properties(MRM PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(MRM PUBLIC
        "${MRM_SOURCE_DIR}/include")

target_link_libraries(MRM AVSCommon)

if (MRM)
    target_link_libraries(MRM MRMHandler)
endif()

# install target
asdk_install()

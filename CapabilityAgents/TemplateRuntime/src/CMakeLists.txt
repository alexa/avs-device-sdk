add_definitions("-DACSDK_LOG_MODULE=templateRuntime")

add_library(TemplateRuntime SHARED
    "${CMAKE_CURRENT_LIST_DIR}/TemplateRuntime.cpp")

set_target_properties(TemplateRuntime PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR} )

target_include_directories(TemplateRuntime
    PUBLIC "${TemplateRuntime_SOURCE_DIR}/include")

target_link_libraries(TemplateRuntime AVSCommon)

# install target
asdk_install()
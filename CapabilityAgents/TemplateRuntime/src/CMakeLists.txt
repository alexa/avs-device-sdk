add_definitions("-DACSDK_LOG_MODULE=templateRuntime")

add_library(TemplateRuntime SHARED
    "${CMAKE_CURRENT_LIST_DIR}/TemplateRuntime.cpp")

target_include_directories(TemplateRuntime
    PUBLIC "${TemplateRuntime_SOURCE_DIR}/include")

target_link_libraries(TemplateRuntime AVSCommon)

# install target
asdk_install()
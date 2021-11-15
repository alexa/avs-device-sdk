add_definitions("-DACSDK_LOG_MODULE=templateRuntime")

add_library(TemplateRuntime
    "${CMAKE_CURRENT_LIST_DIR}/TemplateRuntime.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/RenderPlayerInfoCardsProviderRegistrar.cpp")

target_include_directories(TemplateRuntime
    PUBLIC "${TemplateRuntime_SOURCE_DIR}/include")

target_link_libraries(TemplateRuntime AVSCommon)

# install target
asdk_install()
add_definitions("-DACSDK_LOG_MODULE=templateRuntime")

add_library(acsdkTemplateRuntime
        TemplateRuntime.cpp
        TemplateRuntimeFactory.cpp
        RenderPlayerInfoCardsProviderRegistrar.cpp
        RenderPlayerInfoCardsProviderRegistrarFactory.cpp)

target_include_directories(acsdkTemplateRuntime PRIVATE
        "${acsdkTemplateRuntime_SOURCE_DIR}/privateInclude")

target_include_directories(acsdkTemplateRuntime PUBLIC
        "${acsdkTemplateRuntime_SOURCE_DIR}/include")

target_link_libraries(acsdkTemplateRuntime acsdkTemplateRuntimeInterfaces AVSCommon acsdkNotifier)

# install target
asdk_install()

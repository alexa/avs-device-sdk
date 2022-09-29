add_definitions("-DACSDK_LOG_MODULE=acsdkSampleTemplateRuntimePresentationAdaptor")

add_library(acsdkSampleTemplateRuntimePresentationAdaptor
    TemplateRuntimePresentationAdapter.cpp)

target_include_directories(acsdkSampleTemplateRuntimePresentationAdaptor PUBLIC
    "${acsdkSampleTemplateRuntimePresentationAdaptor_SOURCE_DIR}/include")

target_link_libraries(acsdkSampleTemplateRuntimePresentationAdaptor
        acsdkAudioPlayer
        acsdkPresentationOrchestratorClient
        acsdkPresentationOrchestratorInterfaces
        acsdkSampleApplicationInterfaces
        acsdkTemplateRuntime
        acsdkTemplateRuntimeInterfaces)

# install target only if sample libraries are installed
asdk_sample_install()

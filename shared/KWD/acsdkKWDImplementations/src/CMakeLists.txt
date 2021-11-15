add_definitions("-DACSDK_LOG_MODULE=acsdkKWDImplementations")

add_library(acsdkKWDImplementations
    AbstractKeywordDetector.cpp
    KWDNotifierFactories.cpp
    KeywordDetectorStateNotifier.cpp
    KeywordNotifier.cpp)

target_include_directories(acsdkKWDImplementations PRIVATE
    "${acsdkKWDImplementations_SOURCE_DIR}/privateInclude"
)

target_include_directories(acsdkKWDImplementations PUBLIC
    "${acsdkKWDImplementations_SOURCE_DIR}/include"
)

target_link_libraries(acsdkKWDImplementations
    acsdkKWDInterfaces
    acsdkNotifier
    AVSCommon)

# install target
asdk_install()

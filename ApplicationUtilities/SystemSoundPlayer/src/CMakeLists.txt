add_definitions("-DACSDK_LOG_MODULE=systemSoundPlayer")

add_library(SystemSoundPlayer ${AVS_DEVICE_SDK_BUILD_LIBRARY_TYPE} SystemSoundPlayer.cpp)

target_include_directories(SystemSoundPlayer PUBLIC
    "${SystemSoundPlayer_SOURCE_DIR}/include")

target_link_libraries(SystemSoundPlayer AVSCommon)

# install target
asdk_install()
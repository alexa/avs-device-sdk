add_definitions("-DACSDK_LOG_MODULE=esp")

if (ESP_PROVIDER)
    add_library(ESP SHARED ESPDataProvider.cpp)
    target_link_libraries(ESP "${ESP_LIB_PATH}")
    target_include_directories(ESP PUBLIC "${ESP_INCLUDE_DIR}")
else()
    add_library(ESP SHARED DummyESPDataProvider.cpp)
endif()

target_include_directories(ESP PUBLIC
    "${ESP_SOURCE_DIR}/include"
    "${AIP_SOURCE_DIR}/include"
    "${AVSCommon_SOURCE_DIR}/include")

target_link_libraries(ESP AIP AVSCommon)

# install target
asdk_install()

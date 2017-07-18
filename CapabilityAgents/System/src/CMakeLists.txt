add_library(AVSSystem SHARED
    "${CMAKE_CURRENT_LIST_DIR}/EndpointHandler.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/StateSynchronizer.cpp")

target_include_directories(AVSSystem
    PUBLIC "${AVSSystem_SOURCE_DIR}/include")

target_link_libraries(AVSSystem AVSCommon)

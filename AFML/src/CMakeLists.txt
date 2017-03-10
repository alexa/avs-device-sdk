add_library(AFML SHARED
    Channel.cpp
    FocusManager.cpp)

include_directories(AFML "${AFML_SOURCE_DIR}/include")
target_link_libraries(AFML AVSUtils)

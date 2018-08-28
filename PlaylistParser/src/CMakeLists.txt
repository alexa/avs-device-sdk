add_definitions("-DACSDK_LOG_MODULE=PlaylistParser")

add_library(PlaylistParser SHARED
    IterativePlaylistParser.cpp
    PlaylistParser.cpp
    PlaylistUtils.cpp
    UrlContentToAttachmentConverter.cpp)

target_include_directories(PlaylistParser PUBLIC
    "${PlaylistParser_SOURCE_DIR}/include" 
    "${ACL_SOURCE_DIR}/include")

target_link_libraries(PlaylistParser AVSCommon)

# install target
asdk_install()

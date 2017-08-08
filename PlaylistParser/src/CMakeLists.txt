add_definitions("-DACSDK_LOG_MODULE=PlaylistParser")

if(TOTEM_PLPARSER)
    add_library(PlaylistParser SHARED PlaylistParser.cpp)

    target_include_directories(PlaylistParser PUBLIC
            "${PlaylistParser_SOURCE_DIR}/include"
            ${TOTEM_INCLUDE_DIRS})

    target_link_libraries(PlaylistParser ${TOTEM_LDFLAGS} AVSCommon)
else()
    add_library(PlaylistParser SHARED DummyPlaylistParser.cpp)

    target_include_directories(PlaylistParser PUBLIC
        "${PlaylistParser_SOURCE_DIR}/include")

    target_link_libraries(PlaylistParser AVSCommon)
endif()

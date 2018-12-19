find_package(PkgConfig)
pkg_check_modules(CRYPTO REQUIRED libcrypto)

add_definitions("-DACSDK_LOG_MODULE=PlaylistParser")

add_library(PlaylistParser SHARED
    ContentDecrypter.cpp
    FFMpegInputBuffer.cpp
    IterativePlaylistParser.cpp
    M3UParser.cpp
    PlaylistParser.cpp
    PlaylistUtils.cpp
    UrlContentToAttachmentConverter.cpp)

target_include_directories(PlaylistParser PUBLIC
    "${PlaylistParser_SOURCE_DIR}/include" 
    "${ACL_SOURCE_DIR}/include"
    "${CRYPTO_INCLUDE_DIRS}"
    "${FFMPEG_INCLUDE_DIR}")

target_link_libraries(PlaylistParser
    AVSCommon
    "${CRYPTO_LDFLAGS}")

if (ENABLE_SAMPLE_AES)
    if (NOT FFMPEG_INCLUDE_DIR OR NOT FFMPEG_LIB_PATH)
        message(FATAL_ERROR "Cannot enable SAMPLE-AES decryption without FFmpeg support.")
    endif()

    target_include_directories(PlaylistParser PUBLIC "${FFMPEG_INCLUDE_DIR}")
    target_link_libraries(PlaylistParser avcodec avutil avformat)
    add_definitions("-DENABLE_SAMPLE_AES")
endif()

# install target
asdk_install()

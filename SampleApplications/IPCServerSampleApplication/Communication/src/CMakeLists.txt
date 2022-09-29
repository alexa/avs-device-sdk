cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

add_definitions("-DACSDK_LOG_MODULE=communication")
add_library(Communication SHARED WebSocketServer.cpp WebSocketSDKLogger.cpp)


if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "WebSocketPP Path is required to enable websocket server")
endif()


if(NOT ASIO_INCLUDE_DIR)
    find_path(ASIO_INCLUDE_DIR NAMES "asio.hpp")

    if(ASIO_INCLUDE_DIR STREQUAL "ASIO_INCLUDE_DIR-NOTFOUND")
        message(FATAL_ERROR "WebSocket support requires asio.")
    endif()
endif()

target_include_directories(Communication PUBLIC
    "${WEBSOCKETPP_INCLUDE_DIR}"
    "${Communication_SOURCE_DIR}/include"
    "${ASDK_INCLUDE_DIRS}"
     "${ASIO_INCLUDE_DIR}")

target_link_libraries(Communication "${ASDK_LDFLAGS}" AVSCommon)
target_compile_definitions(Communication PUBLIC ASIO_STANDALONE)

# Currently only allow non SSL websocket with debug builds
if(DISABLE_WEBSOCKET_SSL)
    string(TOLOWER ${CMAKE_BUILD_TYPE} LOWERCASE_CMAKE_BUILD_TYPE)
    if(NOT LOWERCASE_CMAKE_BUILD_TYPE STREQUAL "debug")
        message(FATAL_ERROR "DISABLE_WEBSOCKET_SSL used in non-debug build.")
    endif()
else()
    find_package(OpenSSL REQUIRED)
    add_definitions(-DENABLE_WEBSOCKET_SSL)

    target_include_directories(Communication PUBLIC "${OPENSSL_INCLUDE_DIR}")
    target_link_libraries(Communication
            "${OPENSSL_SSL_LIBRARY}"
            "${OPENSSL_CRYPTO_LIBRARY}")
endif()

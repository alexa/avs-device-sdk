find_package(Doxygen ${DOXYGEN_PACKAGE_CONFIG})

if(NOT DOXYGEN_FOUND)
    message(WARNING "Doxygen is required to generate documentation.")
else()
    set(DOXYGEN_CFG_IN doxygen.cfg.in)

    set(DOXYGEN_CFG ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg)
    set(CMAKE_EXTRACT_PRIVATE "NO")
    set(CMAKE_EXCLUDE_PATTERNS "*/privateInclude/* */src/* */test/*" "*/internal/*")
    configure_file(${DOXYGEN_CFG_IN} ${DOXYGEN_CFG} @ONLY)
    add_custom_target(
        doc COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)

    set(DOXYGEN_CFG_ALL ${CMAKE_CURRENT_BINARY_DIR}/doxygen_all.cfg)
    set(CMAKE_EXCLUDE_PATTERNS "")
    configure_file(${DOXYGEN_CFG_IN} ${DOXYGEN_CFG_ALL} @ONLY)
    add_custom_target(
        doc-all COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG_ALL}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen including private headers content"
        VERBATIM)

    set(DOXYGEN_CFG_ALL_PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/doxygen_all_private.cfg)
    set(CMAKE_EXTRACT_PRIVATE "YES")
    configure_file(${DOXYGEN_CFG_IN} ${DOXYGEN_CFG_ALL_PRIVATE} @ONLY)
    add_custom_target(
        doc-all-private COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG_ALL_PRIVATE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen including private headers content and private elements"
        VERBATIM)
endif()

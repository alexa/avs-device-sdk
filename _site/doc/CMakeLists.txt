find_package(Doxygen ${DOXYGEN_PACKAGE_CONFIG})

if(NOT DOXYGEN_FOUND)
    message(FATAL "Doxygen is required to generate documentation.")
else()
    set(DOXYGEN_CFG_IN doxygen.cfg.in)
    set(DOXYGEN_CFG ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg)
    configure_file(${DOXYGEN_CFG_IN} ${DOXYGEN_CFG} @ONLY)
    add_custom_target(
        doc COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CFG}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()

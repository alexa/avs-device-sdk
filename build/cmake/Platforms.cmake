#
# Setup the platform dependent Cmake options.
#

# Build for Windows is supported
# with MSYS2-MinGW64 shell
# Run cmake with -G"MSYS Makefiles"
if (WIN32)
    add_definitions(
            # Wingdi.h inclusion causes build error
            -DNOGDI
            # Gtest has open issue with pthread on MinGW
            # See: https://github.com/google/googletest/issues/606
            -Dgtest_disable_pthreads=ON
            -DNO_SIGPIPE
    )

    # Windows doesn't have rpath so shared libraries should either be
    # in the same directory or add them to the path variable.
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

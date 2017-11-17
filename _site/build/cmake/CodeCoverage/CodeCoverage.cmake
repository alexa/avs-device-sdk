#
# Setup code coverage environment
# 
# To enable code coverage data collection and report creation, run the CMAKE command as follows:
#       cmake -DCOVERAGE=ON <path-to-source>
#
# In order to run the code coverage, the OS must be Linux or Mac, the compiler type GNU, and `lcov` must be installed.
# The following compiler flags enabled by the coverage option.
#       -fprofile-arcs: Add code so that program flow arcs are instrumented.
#       -ftest-coverage: Produce a notes file that the gcov code-coverage utility can use to show program coverage.
#       These flags combined enables code coverage data files to be created while execution. For detailed explanation
#       of these flags @see https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html .
# 
# Usage: After `make <target>` at the top of the build directory, launch Coverage/index.html to access the report.
#

# Enable code coverage command line option.
option(COVERAGE "Enable code coverage for unit and integration tests." ${COVERAGE})

# Find LCOV.
# TODO: Address the necessity of `lcov` per ACSDK-168 and ACSDK-149.
find_program(LCOVCHECK lcov)

# Check system requirements if we can enable coverage.
# TODO: Revisit this logic for llvm-clang per ACSDK-168.
# TODO: Revisit this logic for Windows per ACSDK-149.
set(COVERAGE_AVAILABLE (LCOVCHECK AND UNIX AND (NOT CYGWIN) AND (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")))

if (${COVERAGE_AVAILABLE} AND ${COVERAGE})
    # Enable compiler coverage flags.
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -fprofile-arcs -ftest-coverage")

    # Setup variables for coverage results.
    set(COVERAGE_DIR "${CMAKE_BINARY_DIR}/Coverage")
    set(COVERAGE_DATA_DIR "${COVERAGE_DIR}/Data")
    set(COVERAGE_FILENAME "${COVERAGE_DIR}/Data/${PROJECT_NAME}Coverage.dat")
    set(COVERAGE_REPORT_DIR "${COVERAGE_DIR}/Reports/${PROJECT_NAME}")

    # Keep the top level coverage filename for merging code coverage results.
    if(${PROJECT_NAME} STREQUAL ${CMAKE_PROJECT_NAME})
        file(MAKE_DIRECTORY ${COVERAGE_DATA_DIR})
        file(MAKE_DIRECTORY ${COVERAGE_REPORT_DIR})
        set(CMAKE_COVERAGE_FILENAME ${COVERAGE_FILENAME})
        set(CMAKE_COVERAGE_REPORT_DIR ${COVERAGE_REPORT_DIR})
        set(CMAKE_COVERAGE_FINAL_REPORT "${COVERAGE_DIR}/index.html")
        set(CMAKE_PRECTEST_SCRIPT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/preCTest.sh")
        set(CMAKE_POSTCTEST_SCRIPT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/postCTest.sh")
        set(CMAKE_COVERAGE_SCRIPT_DIR "${COVERAGE_DIR}/Scripts")
        # Copy the pre/post CTest scripts to build directory by changing their permissions to executable.
        file(COPY ${CMAKE_PRECTEST_SCRIPT_SOURCE} ${CMAKE_POSTCTEST_SCRIPT_SOURCE}
                DESTINATION ${CMAKE_COVERAGE_SCRIPT_DIR}
                FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
    endif()

    # Add the coverage output directory to `clean` target.
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES
            "${COVERAGE_REPORT_DIR};${COVERAGE_DATA_DIR};${CMAKE_COVERAGE_FINAL_REPORT}")

    # Setup CTest so that the scripts above run before and after all tests respectively.
    configure_file(${CMAKE_CURRENT_LIST_DIR}/CTestCustom.cmake.template ${CMAKE_CURRENT_BINARY_DIR}/CTestCustom.cmake)
endif()

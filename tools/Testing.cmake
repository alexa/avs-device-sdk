include(CheckCXXCompilerFlag)

if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()

include(CTest)
include(ThirdParty/googletest.cmake)

add_custom_target(unit COMMAND ${CMAKE_CTEST_COMMAND})

if (ANDROID_TEST_AVAILABLE)
    set(TESTING_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

macro(discover_unit_tests includes libraries)
    # This will result in some errors not finding GTest when running cmake, but allows us to better integrate with CTest
    if(BUILD_TESTING)
        find_package(GTest ${GTEST_PACKAGE_CONFIG})
        set (extra_macro_args ${ARGN})
        LIST(LENGTH extra_macro_args num_extra_args)
        if (${num_extra_args} GREATER 0)
            list(GET extra_macro_args 0 inputs)
        endif()
        file(GLOB_RECURSE tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*Test.cpp")
        foreach(testsourcefile IN LISTS tests)
            get_filename_component(testname ${testsourcefile} NAME_WE)
            add_executable(${testname} ${testsourcefile})
            add_dependencies(unit ${testname})
            CHECK_CXX_COMPILER_FLAG("-Wno-deprecated-declarations" HAS_NO_DEPRECATED_DECLARATIONS)
            if (HAS_NO_DEPRECATED_DECLARATIONS)
                target_compile_options(${testname} PRIVATE -Wno-deprecated-declarations)
            endif()
            CHECK_CXX_COMPILER_FLAG("-Winfinite-recursion" HAS_INFINITE_RECURSION)
            if (HAS_INFINITE_RECURSION)
                target_compile_options(${testname} PRIVATE -Wno-error=infinite-recursion)
            endif()
            target_include_directories(${testname} PRIVATE ${includes})
            # Do not include gtest_main due to double free issue
            # - https://github.com/google/googletest/issues/930
            target_link_libraries(${testname} ${libraries} gmock_main)
            add_rpath_to_target("${testname}")
            configure_test_command(${testname} "${inputs}" ${testsourcefile})
        endforeach()
    endif()
endmacro()

macro(configure_test_command testname inputs testsourcefile)
    if(NOT ANDROID)
        GTEST_ADD_TESTS(${testname} "${inputs}" ${testsourcefile})
    elseif(ANDROID_TEST_AVAILABLE)
        # Use generator expression to get the test path when available.
        set(target_path $<TARGET_FILE:${testname}>)
        add_test(NAME "${testname}_fast"
            COMMAND python ${TESTING_CMAKE_DIR}/Testing/android_test.py
                -n ${testname}
                -s ${target_path}
                -d ${ANDROID_DEVICE_INSTALL_PREFIX}
                -i "${inputs}" " --gtest_filter=*test_*")
        add_test(NAME "${testname}_slow"
            COMMAND python ${TESTING_CMAKE_DIR}/Testing/android_test.py
                -n ${testname}
                -s ${target_path}
                -d ${ANDROID_DEVICE_INSTALL_PREFIX}
                -i "${inputs}" " --gtest_filter=*testSlow_*")
        add_test(NAME "${testname}_timer"
            COMMAND python ${TESTING_CMAKE_DIR}/Testing/android_test.py
                -n ${testname}
                -s ${target_path}
                -d ${ANDROID_DEVICE_INSTALL_PREFIX}
                -i "${inputs}" " --gtest_filter=*testTimer_*")
    endif()
endmacro()

# Add Custom Targets
add_custom_target(fast-test COMMAND ${CMAKE_CTEST_COMMAND} -E "testSlow|testTimer" --output-on-failure VERBATIM)
add_custom_target(find-slow-test COMMAND ${CMAKE_CTEST_COMMAND} -E "testSlow|testTimer" --output-on-failure --timeout 1 VERBATIM)
add_custom_target(slow-test COMMAND ${CMAKE_CTEST_COMMAND} -R "testSlow" --output-on-failure)
add_custom_target(timer-test COMMAND ${CMAKE_CTEST_COMMAND} -R "testTimer" --output-on-failure)

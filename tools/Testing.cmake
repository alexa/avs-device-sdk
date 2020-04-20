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
            target_include_directories(${testname} PRIVATE ${includes})
            # Do not include gtest_main due to double free issue
            # - https://github.com/google/googletest/issues/930
            target_link_libraries(${testname} ${libraries} gmock_main)
            if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
                target_link_libraries(${testname} atomic)
            endif()
            configure_test_command(${testname} "${inputs}" ${testsourcefile})
        endforeach()
    endif()
endmacro()

macro(configure_test_command testname inputs testsourcefile)
    if(NOT ANDROID)
        if(${CMAKE_VERSION} VERSION_LESS "3.9.6")
            GTEST_ADD_TESTS(${testname} "${inputs}" ${testsourcefile})
        else()
            GTEST_ADD_TESTS(TARGET ${testname} SOURCES ${testsourcefile} EXTRA_ARGS "${inputs}" TEST_LIST testlist)
            foreach(test IN LISTS testlist)
                if(${test} MATCHES "testSlow_")
                    set_tests_properties(${test} PROPERTIES LABELS "slowtest")
                elseif(${test} MATCHES "testTimer_")
                    set_tests_properties(${test} PROPERTIES LABELS "timertest")
                endif()
            endforeach()
        endif()
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
        set_tests_properties("${testname}_slow" PROPERTIES LABELS "slowtest")
        set_tests_properties("${testname}_timer" PROPERTIES LABELS "timertest")
    endif()
endmacro()

# Add Custom Targets
add_custom_target(fast-test COMMAND ${CMAKE_CTEST_COMMAND} -LE "slowtest\\\|timertest" --output-on-failure)
add_custom_target(find-slow-test COMMAND ${CMAKE_CTEST_COMMAND} -LE "slowtest\\\|timertest" --output-on-failure --timeout 1)
add_custom_target(slow-test COMMAND ${CMAKE_CTEST_COMMAND} -L "slowtest" --output-on-failure)
add_custom_target(timer-test COMMAND ${CMAKE_CTEST_COMMAND} -L "timertest" --output-on-failure)

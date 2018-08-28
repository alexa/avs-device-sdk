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
    find_package(GTest ${GTEST_PACKAGE_CONFIG})
    if(BUILD_TESTING)
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
        add_test(NAME ${testname}
            COMMAND python ${TESTING_CMAKE_DIR}/Testing/android_test.py
                -n ${testname}
                -s ${target_path}
                -d ${ANDROID_DEVICE_INSTALL_PREFIX}
                -i "${inputs}")
    endif()
endmacro()

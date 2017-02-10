include(CTest)
include(ThirdParty/googletest.cmake)


macro(discover_unit_tests includes libraries)
    # This will result in some errors not finding GTest when running cmake, but allows us to better integrate with CTest
    find_package(GTest)
    if(BUILD_TESTING)
        file(GLOB_RECURSE tests RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*Test.cpp")
        foreach (testsourcefile IN LISTS tests)
            get_filename_component(testname ${testsourcefile} NAME_WE)
            add_executable(${testname} ${testsourcefile})
            target_include_directories(${testname} PRIVATE ${includes})
            target_link_libraries(${testname} ${libraries} gtest_main gmock_main)
            add_test(NAME ${testname}_test COMMAND ${testname})
            GTEST_ADD_TESTS(${testname} "" ${testsourcefile})
        endforeach ()
    endif()
endmacro()
cmake_minimum_required(VERSION 3.15)

cmake_minimum_required(VERSION 3.15)

include(GoogleTest)

file(GLOB_RECURSE LSMTREE_TEST_SOURCES "${PROJECT_SOURCE_DIR}/test/*/*test.cpp")

# #####################################################################################################################
# MAKE TARGETS
# #####################################################################################################################

# #########################################
# "make check-tests"
# #########################################
add_custom_target(build-tests)
add_custom_target(check-tests COMMAND ${CMAKE_CTEST_COMMAND} --verbose)

# #########################################
# "make XYZ_test"
# #########################################
foreach (lsmtree_test_source ${LSMTREE_TEST_SOURCES})
    # Create a human readable name.
    get_filename_component(lemtree_test_filename ${lsmtree_test_source} NAME)
    string(REPLACE ".cpp" "" lsmtree_test_name ${lemtree_test_filename})

    # Add the test target separately and as part of "make check-tests".
    add_executable(${lsmtree_test_name} EXCLUDE_FROM_ALL ${lsmtree_test_source})
    add_dependencies(build-tests ${lsmtree_test_name})
    add_dependencies(check-tests ${lsmtree_test_name})

    gtest_discover_tests(${lsmtree_test_name}
            EXTRA_ARGS
            --gtest_color=auto
            --gtest_output=xml:${CMAKE_BINARY_DIR}/test/${lsmtree_test_name}.xml
            --gtest_catch_exceptions=0
            DISCOVERY_TIMEOUT 120
            PROPERTIES
            TIMEOUT 120
            )

    target_link_libraries(${lsmtree_test_name} lsm_tree gmock_main gtest)

    # Set test target properties and dependencies.
    set_target_properties(${lsmtree_test_name}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test"
            COMMAND ${lsmtree_test_name}
            )

    # Add each test to the build-tests target to ensure they are all built with 'make build-tests'
    add_dependencies(build-tests ${lsmtree_test_name})
endforeach ()


# # #########################################
# # "make XYZ_test"
# # #########################################
# foreach (lsmtree_test_source ${BUSTUB_SLT_SOURCES})
#     # Create a human readable name.
#     get_filename_component(lemtree_test_filename ${lsmtree_test_source} NAME)
#     string(REPLACE ".slt" "" bustub_filename_wo_suffix "${lemtree_test_filename}")
#     string(REPLACE ".slt" "" lsmtree_test_name "SQLLogicTest.${bustub_filename_wo_suffix}")
#     add_test(NAME ${lsmtree_test_name} COMMAND "${CMAKE_BINARY_DIR}/bin/lsmtree-sqllogictest" ${lsmtree_test_source} --verbose -d --in-memory)
#     add_custom_target(${bustub_filename_wo_suffix}_test COMMAND "${CMAKE_BINARY_DIR}/bin/lsmtree-sqllogictest" "${lsmtree_test_source}" --verbose -d --in-memory)
#     add_dependencies(${bustub_filename_wo_suffix}_test sqllogictest)
# endforeach ()


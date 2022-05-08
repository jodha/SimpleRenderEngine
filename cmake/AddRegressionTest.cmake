# Add image comparison tests to ${test_name} ====================================
# This funcion assumes the following:
#   - the "gold results" to compare to are located in a directory 'gold_results'
#   - the image comparison software is ${PROJECT_BINARY_DIR}/bin/imgcmp
#   - PNG images are being compared
function(add_image_tests test_name tolerance percent_error save_diff_images)
    set(dir ${CMAKE_CURRENT_BINARY_DIR})
    file(GLOB test_cases "gold_results/*.png")
    foreach(case_file_with_path ${test_cases})
        get_filename_component(case_name ${case_file_with_path} NAME_WLE)
        get_filename_component(case_file ${case_file_with_path} NAME)
        if (save_diff_images)
            set(diff_file_string "-o" "diff_${case_file}")
        else ()
            set(diff_file_string "")
        endif ()
        add_test(NAME regression:${test_name}_${case_name}
                 COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v ${diff_file_string} -t ${tolerance} -e ${percent_error}% ${dir}/${case_file} ${case_file_with_path}
                )
    endforeach()
endfunction()

# Add an SRE test in the current directory ======================================
# This funcion assumes the following:
#   - the user interface events file is called 'test.ui_events'
#   - the test's executable name is located '../../' and prefixed with 'SRE-Test'
function(add_sre_test test_name tolerance percent_error save_diff_images)
    set(dir ${CMAKE_CURRENT_BINARY_DIR})
    file(COPY . DESTINATION ${dir})
    add_test(NAME regression:${test_name}
             COMMAND ../../SRE-Test-${test_name} -p test.ui_events -c
            )
        add_image_tests(${test_name} ${tolerance} ${percent_error} ${save_diff_images})
endfunction()

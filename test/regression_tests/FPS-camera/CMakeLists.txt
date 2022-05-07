# FPS-camera regression test

set(test_name FPS-camera)

set(dir ${CMAKE_CURRENT_BINARY_DIR})
file(COPY . DESTINATION ${dir})

add_test(NAME ${test_name} COMMAND ../../SRE-Test-${test_name} -p test.ui_events -c)

file(GLOB test_cases "gold_results/*.png")
foreach(case_file_with_path ${test_cases})
    get_filename_component(case_name ${case_file_with_path} NAME_WLE)
    get_filename_component(case_file ${case_file_with_path} NAME)
    add_test(NAME ${test_name}_${case_name}
             COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v -o diff_${case_file} -t 0.0 -e 0.0% ${dir}/${case_file} ${case_file_with_path}
             )
endforeach()

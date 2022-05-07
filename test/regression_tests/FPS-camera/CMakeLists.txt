# FPS-camera regression test

set(dir ${CMAKE_CURRENT_BINARY_DIR})
file(COPY . DESTINATION ${dir})

set(test_name FPS-camera)
add_test(NAME ${test_name} COMMAND ../../SRE-Test-FPS-camera -p test.ui_events -c)
#add_test(NAME FPS-camera-image0 COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v -o diff0.png -t 0.0 -e 0.0% gold_results/Test0.png Test0.png)

#file(GLOB test_cases "${dir}/*.png")
file(GLOB test_cases "gold_results/*.png")
foreach(case_file_with_path ${test_cases})
    get_filename_component(case_name ${case_file_with_path} NAME_WLE)
    get_filename_component(case_file ${case_file_with_path} NAME)
    set(diff_file diff_${case_file})
    set(sub_test_name ${test_name}_${case_name})
    add_test(NAME ${sub_test_name}
             COMMAND ${PROJECT_BINARY_DIR}/bin/imgcmp -v -o ${diff_file} -t 0.0 -e 0.0% ${dir}/${case_file} ${case_file_with_path}
             )
endforeach()

#find_program (BASH_PROGRAM bash)
#if (BASH_PROGRAM)
#    add_test(NAME FPS-camera 
#             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
#             COMMAND run-test.sh
#             )
#endif (BASH_PROGRAM)

# List of single-file tests

include_directories(SYSTEM
    ${gtest_SOURCE_DIR}
    ${gtest_SOURCE_DIR}/include)

file(GLOB_RECURSE test_cases *.cpp)
foreach(case_file ${test_cases})
    get_filename_component( case_name ${case_file} NAME_WE )
    set (case_name test_${case_name})
    add_executable(${case_name} ${case_file})
    target_link_libraries(${case_name} 
                          SRE ${EXTRA_LIBS} ${SDL2_LIBRARY} ${SDL2_IMAGE_LIBRARIES} ${OPENVR_LIB}
                          ${LINK_LIBS} gtest_main pthread  
                         )
    add_test(NAME ${case_name}
             COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${case_name}
             WORKING_DIRECTORY
             ${PROJECT_BINARY_DIR})
endforeach()

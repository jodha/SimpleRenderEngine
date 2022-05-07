# Add regression tests
#add_subdirectory(FPS_test)
#add_test(NAME my_test COMMAND my_test)

file(COPY . DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#  add_test (mytest ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/regression_tests/FPS-camera/run-test.sh)
find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
    add_test(NAME FPS-camera 
             WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
             COMMAND run-test.sh
             )
endif (BASH_PROGRAM)

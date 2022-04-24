# List of single-file tests

#  add_test (mytest ${BASH_PROGRAM} ${CMAKE_CURRENT_SOURCE_DIR}/FPS-camera/run-test.sh)

find_program (BASH_PROGRAM bash)
if (BASH_PROGRAM)
    add_test(NAME  mytest
             WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test_data/FPS-camera/
             COMMAND run-test.sh
             )
endif (BASH_PROGRAM)

file(COPY .  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})


set(test_name "particle-test")
set(pixel_error_tolerance "0.0")
set(percent_error_allowed "0.0")
set(save_diff_images FALSE)

build_sre_test(${test_name})
add_sre_test(${test_name} ${pixel_error_tolerance} ${percent_error_allowed} ${save_diff_images})

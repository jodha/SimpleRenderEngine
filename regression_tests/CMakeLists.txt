#Add regression testing directories

add_all_subdirectories()

# Comment line above and Uncomment line below to add only selected subdirectories
#add_subdirectory(FPS-camera)

# Uncomment the following to build on Windows outside the MSYS2 environment
# This creates a list of DLL files and copies them to where the execuables are
#IF (WIN32)
#    SET(SDL2_dll "" CACHE STRING "Where the SDL2.dll can be found")
#    SET(SDL2_image_lib_path "" CACHE PATH "Where the SDL2_image.dll can be       found")
#    SET(DLLFileList "${SDL2_image_lib_path}/SDL2_image.dll"                      "${SDL2_image_lib_path}/libjpeg-9.dll" "${SDL2_image_lib_path}/libpng16-16.dll"   "${SDL2_image_lib_path}/libtiff-5.dll" "${SDL2_image_lib_path}/libwebp-7.dll"     "${SDL2_image_lib_path}/zlib1.dll"  ${SDL2_dll})
#    IF (USE_OPENVR)
#        SET(DLLFileList "${DLLFileList}" "${OPENVR_ROOT_DIR}/bin/                 ${OPENVR_PLATFORM}/openvr_api.dll")
#    ENDIF (USE_OPENVR)
#    FIXME: copy the dll files into each test directory (run a 'foreach' loop)
##    file(COPY ${DLLFileList} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#ENDIF(WIN32)

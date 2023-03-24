# CMakeList.txt : CMake project for JPEGC, include source and define
# project specific logic here.
#
find_package(OpenCV CONFIG REQUIRED) 
# Add source to this project's executable.
add_executable (JPEGC "JPEGC.cpp" "JPEGC.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET JPEGC PROPERTY CXX_STANDARD 20)
endif()
target_link_libraries(JPEGC PRIVATE opencv_ml opencv_dnn opencv_core opencv_flann opencv_highgui opencv_imgproc)
# TODO: Add tests and install targets if needed.

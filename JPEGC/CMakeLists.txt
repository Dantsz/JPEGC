set(CMAKE_EXPERIMENTAL_CXX_MODULE_DYNDEP 1)
set(CMAKE_EXPERIMENTAL_CXX_MODULE_CMAKE_API "3c375311-a3c9-4396-a187-3227ef642046")

find_package(OpenCV CONFIG REQUIRED) 
find_package( Argumentum CONFIG REQUIRED )





# Add source to this project's executable.
add_executable (JPEGC "JPEGC.cpp")
target_sources(JPEGC PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES imageproc.ixx)

set_property(TARGET JPEGC PROPERTY CXX_STANDARD 20)
target_link_libraries(JPEGC PRIVATE opencv_ml opencv_dnn opencv_core opencv_flann opencv_highgui opencv_imgproc Argumentum::argumentum)


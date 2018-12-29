add_library(jfio file2.h jfile.h jfio.h jfio.cpp)

add_executable(jfio_test jfio_test.cpp)
target_link_libraries(jfio_test jfio)
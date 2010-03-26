set(CMAKE_MODULE_PATH Modules ${CMAKE_MODULE_PATH} cache path)
cmake_minimum_required(VERSION 2.6)

project(bu0836a)
find_package(USB1)
include_directories(${LIBUSB_INCLUDE_DIR})
add_executable(bu0836a bu0836a hid_parser logging options main)
target_link_libraries(bu0836a ${LIBUSB_LIBRARIES})

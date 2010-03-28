set(CMAKE_MODULE_PATH Modules ${CMAKE_MODULE_PATH} cache path)
cmake_minimum_required(VERSION 2.6)

project(bu0836)
find_package(USB1)
include_directories(${LIBUSB_INCLUDE_DIR})
add_executable(bu0836 bu0836 hid logging options main)
target_link_libraries(bu0836 ${LIBUSB_LIBRARIES})

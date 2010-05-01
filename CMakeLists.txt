set(CMAKE_MODULE_PATH "CMake" ${CMAKE_MODULE_PATH} cache path)
cmake_minimum_required(VERSION 2.6)

execute_process(COMMAND git log master --pretty=format:%h -1 OUTPUT_VARIABLE SHA)
execute_process(COMMAND git tag -l "[0-9].*" COMMAND tail -1 COMMAND tr -d '\n' OUTPUT_VARIABLE TAG)
execute_process(COMMAND git diff --shortstat COMMAND wc -l OUTPUT_VARIABLE MOD)
add_definitions(-DSHA=${SHA} -DTAG=${TAG} -DMOD=${MOD})

project(bu0836)
find_package(USB1)
include_directories(${LIBUSB_INCLUDE_DIR})
add_executable(bu0836 bu0836 hid logging options main)
target_link_libraries(bu0836 ${LIBUSB_LIBRARIES})

install(FILES bu0836.1 DESTINATION share/man/man1)
install(PROGRAMS bu0836 DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

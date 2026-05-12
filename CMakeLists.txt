# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXTRA_COMPONENT_DIRS
    core/src
    core/external
    core/plugin
)

add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-frtti>")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(iot-stride)

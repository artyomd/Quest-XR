cmake_minimum_required(VERSION 3.18.1)

project(quest-xr)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)
add_subdirectory(external/fmt)
add_subdirectory(external/glm)
add_subdirectory(external/spirv-reflect)
add_subdirectory(cpp)

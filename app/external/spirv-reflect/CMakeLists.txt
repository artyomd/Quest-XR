cmake_minimum_required(VERSION 3.18.1)

add_library(spirv-reflect STATIC SPIRV-Reflect/spirv_reflect.c)

target_include_directories(spirv-reflect PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(spirv-reflect)

cmake_minimum_required(VERSION 3.18.1)

set(SRC logger.cpp utils.cpp)

add_library(utils STATIC ${SRC})

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(utils android log fmt)

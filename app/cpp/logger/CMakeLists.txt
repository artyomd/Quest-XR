cmake_minimum_required(VERSION 3.18.1)

set(SRC
        logger.cpp)

add_library(logger STATIC ${SRC})

target_include_directories(logger PUBLIC ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(logger
        android
        log)

project(matrix_hal C CXX)
cmake_minimum_required(VERSION 2.8)

add_compile_options(-std=c++11)

include_directories(
  ../..
)

set(matrix_hal_src
  everloop.cpp
)

set (matrix_hal_headers
  creator_memory_map.h
  everloop.h        
  everloop_image.h  						 
)

add_library(matrix_hal SHARED ${matrix_hal_src})
  set_property(TARGET matrix_hal PROPERTY CXX_STANDARD 11)

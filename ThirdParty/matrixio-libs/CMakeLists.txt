project(matrix_creator_hal C CXX)
cmake_minimum_required(VERSION 2.8)

add_compile_options(-std=c++11)

include_directories(
  ../..
)

set(matrix_creator_hal_src
  everloop.cpp
)

set (matrix_creator_hal_headers
  creator_memory_map.h
  everloop.h        
  everloop_image.h  						 
)

add_library(matrix_creator_hal SHARED ${matrix_creator_hal_src})
  set_property(TARGET matrix_creator_hal PROPERTY CXX_STANDARD 11)

project(matrix_creator_hal C CXX)
cmake_minimum_required(VERSION 2.8)

add_compile_options(-std=c++11)

set(matrix_creator_hal_src
  everloop.cpp
)

add_library(matrix_creator_hal ${matrix_creator_hal_src})
  set_property(TARGET matrix_creator_hal PROPERTY CXX_STANDARD 11)

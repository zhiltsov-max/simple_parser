cmake_minimum_required(VERSION 3.5)

project(parser)

add_subdirectory(src)

enable_testing()
if (BUILD_TESTING)
  add_subdirectory(test)
endif()

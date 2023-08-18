cmake_minimum_required(VERSION 3.23)
project(avtool)


set(CMAKE_CXX_STANDARD 17)

option(AV_LIBRARY_STATIC "Compile av_library as static library" OFF)


add_subdirectory(av_library)
add_subdirectory(av_test)
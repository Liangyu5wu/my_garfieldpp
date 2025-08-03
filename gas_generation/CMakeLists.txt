cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(GenerateHeIC4H10Gas)

if(NOT TARGET Garfield::Garfield)
  find_package(Garfield REQUIRED)
endif()

add_executable(generate_he_ic4h10 generate_he_ic4h10.C)
target_link_libraries(generate_he_ic4h10 Garfield::Garfield)
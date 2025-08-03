# Wire Chamber Simulation Project
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(IDEA_DCH)

if(NOT TARGET Garfield::Garfield)
  find_package(Garfield REQUIRED)
endif()

#---Build executable------------------------------------------------------------
add_executable(idea_chamber idea_chamber.C)
target_link_libraries(idea_chamber Garfield::Garfield)

# Add OpenMP support for potential multi-threading
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(idea_chamber OpenMP::OpenMP_CXX)
endif()

# ---Copy all data files to build directory----------------------------------
foreach(_file 
    ar_93_co2_7_3bar.gas
    mdt_elx_delta.txt)
  configure_file(${_file} ${CMAKE_CURRENT_BINARY_DIR}/${_file} COPYONLY)
endforeach()

cmake_minimum_required(VERSION 3.10)
project(fd_convergence CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(HDF5 REQUIRED COMPONENTS CXX)

add_executable(fd_convergence finite_difference.cpp)

target_include_directories(fd_convergence PRIVATE ${HDF5_INCLUDE_DIRS})
target_link_libraries(fd_convergence PRIVATE ${HDF5_CXX_LIBRARIES} ${HDF5_LIBRARIES})
target_compile_definitions(fd_convergence PRIVATE ${HDF5_DEFINITIONS})

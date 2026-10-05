cmake_minimum_required(VERSION 3.15)
project(rabbit_protocol VERSION 3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find OpenMP library for multi-threading
find_package(OpenMP REQUIRED)

# Define project executable and source targets
add_executable(rabbit_simulator 
    main.cpp
    rabbit_codec.cpp
    rabbit_codec.hpp
)

# Link OpenMP to the simulator executable
target_link_libraries(rabbit_simulator PRIVATE OpenMP::OpenMP_CXX)

# Configure compiler optimizations for Release builds
if(MSVC)
    target_compile_options(rabbit_simulator PRIVATE /O2 /Oi)
else()
    target_compile_options(rabbit_simulator PRIVATE -O3 -march=native)
endif()

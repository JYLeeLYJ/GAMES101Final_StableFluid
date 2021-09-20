cmake_minimum_required(VERSION 3.18)

project(Stable_Fluid2D)

# set(CMAKE_CXX_STANDARD 20)

find_package(Eigen3 CONFIG REQUIRED )
find_package(imgui CONFIG REQUIRED )

set(SOURCES 
    main.cpp
    gui.cpp
    fluid_solver.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/include)
add_executable(fluid ${SOURCES})
target_compile_options(fluid PRIVATE /openmp /std:c++latest)
target_link_libraries(fluid PRIVATE imgui::imgui d3d11)


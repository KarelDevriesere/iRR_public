cmake_minimum_required(VERSION 3.28.1)
project(test CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Enable modules for MSVC
if (MSVC)
    add_compile_options(/std:c++20 /experimental:module)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

# Boost
find_package(Boost REQUIRED)
if (Boost_FOUND)
    message(STATUS "Boost found")
    message(STATUS "${BOOST_INCLUDEDIR}")
    message(STATUS "${BOOST_LIBRARYDIR}")
endif()

include_directories(${BOOST_INCLUDEDIR})

# Gurobi:
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_GENERATOR_PLATFORM x64)

find_package(GUROBI REQUIRED)
include_directories(${GUROBI_INCLUDE_DIRS})

add_executable(test main.cpp algo.h algo.cpp Input.h Input.cpp GurSolver.h GurSolver.cpp Operators.h Operators.cpp Solution.h Solution.cpp ILS.h ILS.cpp MiaoAlgo.h MiaoAlgo.cpp SA.h FO.h FO.cpp Analyze.h Analyze.cpp InstanceGenerator.h CM.h Heuristic_CM.h Heuristic_CM.cpp MiaoHockey.h)

message(STATUS "TRY CXX HERE")
target_link_libraries(${CMAKE_PROJECT_NAME} optimized ${GUROBI_CXX_LIBRARY}
                     debug ${GUROBI_CXX_DEBUG_LIBRARY})

target_link_libraries(${CMAKE_PROJECT_NAME} ${GUROBI_LIBRARY})
add_definitions(-DPATHSEP="\\\\")
set(PRINT 1 CACHE STRING "Enable printing (0/1)")
add_compile_definitions(PRINT=${PRINT})

message(STATUS "Gurobi Include Dir: ${GUROBI_INCLUDE_DIRS}")
message(STATUS "Gurobi Library: ${GUROBI_LIBRARY}")
message(STATUS "Gurobi CXX Library: ${GUROBI_CXX_LIBRARY}")

# RapidXML -> add after executable test is made
# header only so this is the only thing we need
# target_include_directories(test PRIVATE "C:/rapidxml_1_13")

set(RAPIDXML_DIR "${CMAKE_SOURCE_DIR}/rapidxml")
target_include_directories(test PRIVATE ${RAPIDXML_DIR})

# to compile in debug mode:
# cmake --build build --config Debug
# .\build\Debug\test.exe

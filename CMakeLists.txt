cmake_minimum_required(VERSION 3.1)

project(realtime)

set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}/include")
file(GLOB SRCS "src/*.cpp" "src/*.c")

set(CMAKE_CXX_STANDARD 17)

add_executable(realtime ${SRCS})

# find_package(PNG REQUIRED)
# include_directories(${PNG_INCLUDE_DIR})

find_package(ASSIMP REQUIRED)
include_directories(${ASSIMP_INCLUDE_DIR})

find_package(glfw3 3.2 REQUIRED)

target_link_libraries(realtime assimp glfw GL dl)

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif (CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

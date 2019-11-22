cmake_minimum_required(VERSION 3.1)

project(realtime)

set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

file(GLOB SRCS "src/*.cpp" "src/*.c")
set(CMAKE_CXX_STANDARD 17)
add_executable(realtime ${SRCS})
include_directories("${PROJECT_SOURCE_DIR}/include")

if (WIN32)
    include_directories("${PROJECT_SOURCE_DIR}/external/assimp/include;${PROJECT_SOURCE_DIR}/external/glfw/include")
    link_directories("${PROJECT_SOURCE_DIR}/external/assimp/lib;${PROJECT_SOURCE_DIR}/external/glfw/lib")
    target_link_libraries(realtime assimp-vc140-mt glfw3)
endif()
if (UNIX)
    find_package(ASSIMP REQUIRED)
    find_package(glfw3 3.3 REQUIRED)
    target_link_libraries(realtime assimp glfw dl)
endif()

if (CMAKE_COMPILER_IS_GNUCC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif (CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

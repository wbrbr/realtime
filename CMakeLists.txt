cmake_minimum_required(VERSION 3.1)

project(realtime)

set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

set(SRCS "src/camera.cpp" "src/main.cpp" "src/mesh.cpp" "src/renderer.cpp" "src/shader.cpp" "src/texture.cpp" "src/texture_loader.cpp" "src/transform.cpp")
set(IMGUI_SRC "${PROJECT_SOURCE_DIR}/external/imgui/src")
list(APPEND SRCS "${IMGUI_SRC}/imgui_demo.cpp"
                 "${IMGUI_SRC}/imgui_draw.cpp"
                 "${IMGUI_SRC}/imgui_impl_glfw.cpp"
                 "${IMGUI_SRC}/imgui_impl_opengl3.cpp"
                 "${IMGUI_SRC}/imgui_widgets.cpp"
                 "${IMGUI_SRC}/imgui.cpp"
                 "${PROJECT_SOURCE_DIR}/external/tracy/TracyClient.cpp")
list(APPEND SRCS "${PROJECT_SOURCE_DIR}/external/gl3w/src/gl3w.c")
set(CMAKE_CXX_STANDARD 17)
add_executable(realtime ${SRCS})
target_include_directories(realtime PRIVATE "${PROJECT_SOURCE_DIR}/include" 
                    "${PROJECT_SOURCE_DIR}/external/gl3w/include"
                    "${PROJECT_SOURCE_DIR}/external/glm/include"
                    "${PROJECT_SOURCE_DIR}/external/imgui/include"
                    "${PROJECT_SOURCE_DIR}/external/tracy"
                    "${PROJECT_SOURCE_DIR}/external/stb_include")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory("${PROJECT_SOURCE_DIR}/external/glfw")
add_subdirectory("${PROJECT_SOURCE_DIR}/external/assimp")

# if (WIN32)
#     include_directories("${PROJECT_SOURCE_DIR}/external/assimp/include;${PROJECT_SOURCE_DIR}/external/glfw/include")
#     link_directories("${PROJECT_SOURCE_DIR}/external/assimp/lib;${PROJECT_SOURCE_DIR}/external/glfw/lib")
#     target_link_libraries(realtime assimp-vc140-mt glfw3)
# endif()
# if (UNIX)
#     find_package(ASSIMP REQUIRED)
#     find_package(glfw3 3.3 REQUIRED)
#     target_link_libraries(realtime assimp glfw dl pthread)
# endif()

target_link_libraries(realtime assimp glfw)

find_package(OpenGL REQUIRED)
target_link_libraries(realtime OpenGL::GL)

add_compile_definitions(TRACY_ENABLE)
add_compile_definitions(NOMINMAX)

# if (CMAKE_COMPILER_IS_GNUCC)
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -DTRACY_ENABLE")
#     # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
# endif (CMAKE_COMPILER_IS_GNUCC)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)

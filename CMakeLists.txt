cmake_minimum_required(VERSION 3.20)
project(amd_gl_spirv_array_repro CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Point GLFW_LOCAL_DIR at an existing GLFW checkout to skip the download.
set(GLFW_LOCAL_DIR "" CACHE PATH "Existing GLFW source directory")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
if(GLFW_LOCAL_DIR)
	add_subdirectory(${GLFW_LOCAL_DIR} glfw)
else()
	include(FetchContent)
	FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
	FetchContent_MakeAvailable(glfw)
endif()

add_executable(repro main.cpp glad/src/glad.c)
target_include_directories(repro PRIVATE glad/include)
target_link_libraries(repro PRIVATE glfw)
target_compile_definitions(repro PRIVATE SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders")

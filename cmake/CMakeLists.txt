# CMake Version
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Project name and version
project(VulkanRenderer)

set(VulkanRenderer_VERSION_MAJOR 0)
set(VulkanRenderer_VERSION_MINOR 1)
set(VulkanRenderer_PATCH 0)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories("${CMAKE_SOURCE_DIR}/../include")
include_directories("${CMAKE_SOURCE_DIR}/../include/glfw")
include_directories("${CMAKE_SOURCE_DIR}/../include/glm")

# Gather C++ and C from src directory
file(GLOB_RECURSE SOURCES_CPP "${CMAKE_SOURCE_DIR}/../src/*.cpp")
file(GLOB_RECURSE SOURCES_C "${CMAKE_SOURCE_DIR}/../src/*.c")

# Combine source files into list
set(SOURCES ${SOURCES_CPP} ${SOURCES_C})

# Executable target
add_executable(VulkanRenderer ${SOURCES})

# Binary output directory
set_target_properties(VulkanRenderer PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/../build/$<CONFIG>>)

# Libraries directory
set(LIBRARIES_DIR "${CMAKE_SOURCE_DIR}/../libraries")

# Set path to glfw
set(GLFW_LIB_DEBUG "${LIBRARIES_DIR}/glfw/glfw3.dll")
set(GLFW_LIB_RELEASE "${LIBRARIES_DIR}/glfw/glfw3.dll")

# Link glfw
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Link GLFW debug version
    target_link_libraries(VulkanRenderer ${GLFW_LIB_DEBUG})
else()
    # Link GLFW release version
    target_link_libraries(VulkanRenderer ${GLFW_LIB_RELEASE})
endif()

# Find Vulkan
find_package(Vulkan REQUIRED)

# Link Vulkan
target_link_libraries(VulkanRenderer Vulkan::Vulkan)

# find shaders directory
set(SHADERS_DIR "${CMAKE_SOURCE_DIR}/../shaders")

# Copy DLL files after building
add_custom_command(TARGET VulkanRenderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:${GLFW_LIB_DEBUG}>
        $<$<CONFIG:Release>:${GLFW_LIB_RELEASE}>
        $<TARGET_FILE_DIR:VulkanRenderer>

        # copy shaders folder
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADERS_DIR}
        $<TARGET_FILE_DIR:VulkanRenderer>/shaders
)
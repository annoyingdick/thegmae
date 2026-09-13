cmake_minimum_required(VERSION 3.10)

project(game)

set(GCC_COVERAGE_COMPILE_FLAGS "-g -Wall -Wextra -Werror -O0") # -Werror
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_definitions(${GCC_COVERAGE_COMPILE_FLAGS})

#add_library(nuklear "src/include/nuklear/nuklear.h" "src/include/nuklear/nuklear_sdl_vulkan.h" "src/nuklear_usage.c")

#set_target_properties(vma PROPERTIES CXX_STANDARD 17)
#target_link_libraries(vma)

include_directories(src/include src E:/MSYS2/ucrt64/include)
link_directories(src/lib E:/MSYS2/ucrt64/lib)
link_libraries(SDL3 SDL3_image opengl32 glew32)

file(GLOB SOURCES "src/*/*.c")
add_executable(game ${SOURCES})

#set(CMAKE_MAKE_PROGRAM E:\MSYS2\ucrt64\bin\make.exe)

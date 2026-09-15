cmake_minimum_required(VERSION 3.25)

project(thegmae)

include(FetchContent)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

set(GENERATE_DEBUG_SYMBOLS ON)
set(ENABLE_ALL_WARNINGS ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SDL_TEST_LIBRARY OFF)
set(glew-cmake_BUILD_SHARED OFF)

FetchContent_Declare(
    SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.4.16
)
FetchContent_MakeAvailable(SDL)
FetchContent_Declare(
    GLEW
    GIT_REPOSITORY https://github.com/Perlmint/glew-cmake
)
FetchContent_MakeAvailable(GLEW)
FetchContent_Declare(
    CGLM
    GIT_REPOSITORY https://github.com/recp/cglm
    GIT_TAG v0.9.6
)
FetchContent_MakeAvailable(CGLM)
FetchContent_Declare(
    nuklear 
    URL https://raw.githubusercontent.com/Immediate-Mode-UI/Nuklear/v4.13.3/nuklear.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(nuklear)
FetchContent_Declare(
    stbimage 
    URL https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(stbimage)
FetchContent_Declare(
    cgltf 
    URL https://raw.githubusercontent.com/jkuhlmann/cgltf/v1.15/cgltf.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(cgltf)
FetchContent_Declare(
    khashl 
    URL https://raw.githubusercontent.com/attractivechaos/khashl/refs/heads/main/khashl.h
    DOWNLOAD_NO_EXTRACT TRUE
)
FetchContent_MakeAvailable(khashl)

set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_C_FLAGS_RELEASE "-O3")

file(GLOB SOURCES "src/*/*.c")
add_executable(thegmae ${SOURCES})

target_include_directories(thegmae PRIVATE src)
target_link_libraries(thegmae SDL3-static)
target_link_libraries(thegmae libglew_static)
target_link_libraries(thegmae cglm)

if (ENABLE_ALL_WARNINGS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")
endif()
if (GENERATE_DEBUG_SYMBOLS)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
endif()

#target_include_directories(thegmae PRIVATE ${SDL_SOURCE_DIR}/include)

target_include_directories(thegmae PUBLIC ${nuklear_SOURCE_DIR})
target_include_directories(thegmae PUBLIC ${stbimage_SOURCE_DIR})
target_include_directories(thegmae PUBLIC ${cgltf_SOURCE_DIR})
target_include_directories(thegmae PUBLIC ${khashl_SOURCE_DIR})

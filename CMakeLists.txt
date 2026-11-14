cmake_minimum_required(VERSION 3.16)
project(WatermelonVulkan CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)

find_package(glm QUIET)
if(NOT glm_FOUND)
    include(FetchContent)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(watermelon ${SOURCES})

target_include_directories(watermelon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(watermelon PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    gdi32
    opengl32
)

if(MINGW)
    target_link_options(watermelon PRIVATE -static -static-libgcc -static-libstdc++)
endif()

find_program(GLSLC glslc HINTS $ENV{VULKAN_SDK}/bin $ENV{VULKAN_SDK}/Bin)
if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found. Install Vulkan SDK.")
endif()

set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_BIN})

add_custom_command(TARGET watermelon POST_BUILD
    COMMAND ${GLSLC} ${SHADER_SRC}/watermelon.vert -o ${SHADER_BIN}/watermelon.vert.spv
    COMMAND ${GLSLC} ${SHADER_SRC}/watermelon.frag -o ${SHADER_BIN}/watermelon.frag.spv
)

file(TO_CMAKE_PATH "${SHADER_BIN}" SHADER_BIN_STR)
target_compile_definitions(watermelon PRIVATE SHADER_DIR="${SHADER_BIN_STR}")
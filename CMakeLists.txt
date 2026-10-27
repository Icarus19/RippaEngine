cmake_minimum_required(VERSION 3.20)

project(RippaEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(ZLIB REQUIRED)

add_library(glad STATIC
    third_party/glad/src/glad.c
)
target_include_directories(glad PUBLIC
    third_party/glad/include
)

add_library(assimp STATIC IMPORTED)
set_target_properties(assimp PROPERTIES
    IMPORTED_LOCATION
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/lib/libassimp.a
    INTERFACE_INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/include
)

add_executable(RippaEngine
        src/main.cpp
        src/camera/Camera.cpp
        src/input/Input.cpp
        src/renderer/Texture.cpp
        src/renderer/Mesh.cpp
        src/renderer/Shader.cpp
        src/assets/ImageLoader.cpp
        src/renderer/Shader.h
)

target_include_directories(RippaEngine PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        third_party/glad
        third_party/stb_image
)

target_link_libraries(RippaEngine PRIVATE
        glad
        OpenGL::GL
        glfw
        assimp
        ZLIB::ZLIB
)
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

add_library(glad STATIC
    third_party/glad/src/glad.c
)

target_include_directories(glad PUBLIC
    third_party/glad/include
)

add_executable(RippaEngine
        src/main.cpp
        src/assets/ImageLoader.cpp
)

target_include_directories(RippaEngine PRIVATE
        third_party/stb_image
)

target_link_libraries(RippaEngine PRIVATE
        glad
        OpenGL::GL
        glfw
)
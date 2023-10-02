cmake_minimum_required(VERSION 3.5)
project(test)

find_package(GLEW)
find_package(GLFW3)

find_package(OpenGL REQUIRED)

include_directories(external/glad/include)

include_directories(./include)
file(GLOB SOURCES src/*.cpp external/glad/src/glad.c)

file(GLOB ASSETS "assets/*")
file(COPY ${ASSETS} DESTINATION ${CMAKE_BINARY_DIR})

add_executable(test ${SOURCES})

message(STATUS ${LIBS})

target_link_libraries(
    test
    GLEW::GLEW
    glfw3
    opengl32
)
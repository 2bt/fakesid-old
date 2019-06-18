cmake_minimum_required(VERSION 3.7)

project(fakesid)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

find_package(PkgConfig REQUIRED)

pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image)

link_libraries(
    ${GLEW_LIBRARIES}
    ${SDL2IMAGE_LIBRARIES}
    -lsndfile
    )

include_directories(
    ${GLEW_INCLUDE_DIRS}
    ${SDL2IMAGE_INCLUDE_DIRS}
    /usr/include/glm
    )

file(GLOB SRC "src/*.hpp" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRC})

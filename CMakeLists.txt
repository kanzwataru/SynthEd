cmake_minimum_required(VERSION 3.1)
project(SynthEd)

set(CMAKE_CXX_STANDARD 11)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB SRCS src/*.cpp src/*.c)
file(GLOB HDRS src/*.hpp src/*.h)

file(GLOB LIBSRC extern/opl_dosbox/*.cpp extern/opl_dosbox/*.h)

add_executable(${PROJECT_NAME} ${SRCS} ${HDRS})
target_include_directories(${PROJECT_NAME} PRIVATE $(CMAKE_SOURCE_DIR)/)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
target_sources(${PROJECT_NAME} PRIVATE ${SRCS} ${HDRS} ${LIBSRC})

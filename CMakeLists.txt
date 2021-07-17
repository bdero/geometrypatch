cmake_minimum_required(VERSION 3.19)
project(geometrypatch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE on)

message(STATUS "Using C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} (${CMAKE_CXX_COMPILER})")

# Add sources
file(GLOB_RECURSE SOURCE_FILES "src/*.cc" "src/*.c" "src/*.h")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

add_subdirectory(deps/fltk)

target_link_libraries(${PROJECT_NAME} PRIVATE fltk gdiplus)
target_include_directories(${PROJECT_NAME} PRIVATE src deps/fltk ${PROJECT_BINARY_DIR}/deps/fltk)

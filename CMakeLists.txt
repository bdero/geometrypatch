cmake_minimum_required(VERSION 3.19)
project(geometrypatch)

set(CMAKE_CXX_STANDARD 20)

# Add sources
file(GLOB_RECURSE SOURCE_FILES "src/*.cc" "src/*.h")
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

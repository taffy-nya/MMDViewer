# cmake version
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# project
project(MMDViewer)

# target
aux_source_directory("./" PROJECT_SOURCES)
add_executable(MMDViewer ${PROJECT_SOURCES})
target_include_directories(MMDViewer PRIVATE include)

# dependency
# find_package(glad CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
# link
# target_link_libraries(MMDViewer PRIVATE glad::glad)
target_link_libraries(MMDViewer PRIVATE glfw)
target_link_libraries(MMDViewer PRIVATE glm::glm)

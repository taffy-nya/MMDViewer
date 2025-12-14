# cmake version
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# project
project(MMDViewer)

# dependencies
include(FetchContent)

# glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG        1.0.2
)
FetchContent_MakeAvailable(glm)

# glfw
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

# stb
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        v1.92.5
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Embedded Shaders Generation
find_package(Python3 REQUIRED)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedShaders.h
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/embed_shaders.py
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/embed_shaders.py
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating embedded shaders header"
)

# target
aux_source_directory("./" PROJECT_SOURCES)
# Ensure the header is generated before building the executable
add_executable(MMDViewer ${PROJECT_SOURCES} ${IMGUI_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/include/EmbeddedShaders.h)
target_include_directories(MMDViewer PRIVATE include ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends ${stb_SOURCE_DIR})

# dependency
target_link_libraries(MMDViewer PRIVATE glfw)
target_link_libraries(MMDViewer PRIVATE glm::glm)

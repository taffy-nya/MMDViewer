cmake_minimum_required(VERSION 3.20)

project(MMDViewer LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG master
    SOURCE_SUBDIR ""
)
FetchContent_MakeAvailable(imguizmo)

if(TARGET imguizmo)
    target_include_directories(imguizmo PUBLIC "${imgui_SOURCE_DIR}")
endif()

# ImGuizmo master 的 AddPolyline 参数顺序适配了旧 ImGui API (closed, thickness)，
# 但当前 ImGui v1.92.5 用的是 (flags, thickness)。交换最后两个参数。
file(READ "${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp" ig_src)
string(REPLACE "gContext.mStyle.RotationLineThickness, 0);"    "0, gContext.mStyle.RotationLineThickness);"    ig_src "${ig_src}")
string(REPLACE "gContext.mStyle.RotationLineThickness, ImDrawFlags_Closed);" "ImDrawFlags_Closed, gContext.mStyle.RotationLineThickness);" ig_src "${ig_src}")
string(REPLACE "1.0f, ImDrawFlags_Closed);"                    "ImDrawFlags_Closed, 1.0f);"                    ig_src "${ig_src}")
file(WRITE "${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp" "${ig_src}")

FetchContent_Declare(stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb)


file(GLOB SHADER_FILES CONFIGURE_DEPENDS "src/shaders/*.glsl")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gen/shader_sources.h"
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src/shaders"
        -DTEMPLATE="${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/shader_sources.h.in"
        -DOUTPUT="${CMAKE_CURRENT_BINARY_DIR}/gen/shader_sources.h"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/embed.cmake"
    DEPENDS ${SHADER_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/embed.cmake"
    COMMENT "Embedding shader sources"
)

add_custom_target(embed_shaders DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gen/shader_sources.h")


file(GLOB PLATFORM_SOURCES CONFIGURE_DEPENDS "src/platform/*.cpp")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "src/core/*.cpp")
file(GLOB RENDER_SOURCES CONFIGURE_DEPENDS "src/render/*.cpp")
file(GLOB ANIM_SOURCES CONFIGURE_DEPENDS "src/animation/*.cpp")
file(GLOB SCENE_SOURCES CONFIGURE_DEPENDS "src/scene/*.cpp")
file(GLOB APP_SOURCES CONFIGURE_DEPENDS "src/app/*.cpp")
file(GLOB UI_SOURCES CONFIGURE_DEPENDS "src/ui/*.cpp" "src/ui/panels/*.cpp")
file(GLOB MAIN_SOURCE CONFIGURE_DEPENDS "src/main.cpp")

set(IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

set(IMGUIZMO_SOURCES
    "${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp"
)


add_library(third_party STATIC
    ${IMGUI_SOURCES}
    ${IMGUIZMO_SOURCES}
    third_party/glad/glad.c
)

target_compile_definitions(third_party PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLAD
)

target_link_libraries(third_party PUBLIC
    glfw
)

target_include_directories(third_party
    PUBLIC
        third_party
        "${imgui_SOURCE_DIR}"
        "${imgui_SOURCE_DIR}/backends"
        "${imguizmo_SOURCE_DIR}/src"
    SYSTEM PUBLIC
        "${glm_SOURCE_DIR}"
        "${stb_SOURCE_DIR}"
)

if(MSVC)
    target_compile_options(third_party PRIVATE /w)
else()
    target_compile_options(third_party PRIVATE -w)
endif()

add_executable(MMDViewer
    ${MAIN_SOURCE}
    ${PLATFORM_SOURCES}
    ${CORE_SOURCES}
    ${RENDER_SOURCES}
    ${ANIM_SOURCES}
    ${SCENE_SOURCES}
    ${APP_SOURCES}
    ${UI_SOURCES}
)

add_dependencies(MMDViewer embed_shaders)

target_link_libraries(MMDViewer PRIVATE third_party)

target_include_directories(MMDViewer PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/gen"
)

if(MSVC)
    target_compile_options(MMDViewer PRIVATE /W4)
else()
    target_compile_options(MMDViewer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    target_link_libraries(MMDViewer PRIVATE winmm)
endif()

if(MINGW)
    target_link_libraries(MMDViewer PRIVATE stdc++exp)
endif()

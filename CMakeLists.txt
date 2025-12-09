# cmake version
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
      CACHE STRING "")
endif()
if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
  set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "")
endif()

# project
project(MMDViewer)

# target
aux_source_directory("./" PROJECT_SOURCES)
add_executable(MMDViewer ${PROJECT_SOURCES})
target_include_directories(MMDViewer PRIVATE include)


if(APPLE)

	find_package(glfw3 CONFIG REQUIRED)
	find_package(glm CONFIG REQUIRED)
	# link

   	target_link_libraries(MMDViewer PRIVATE glfw)
	target_link_libraries(MMDViewer PRIVATE glm::glm)
else()
	# dependency
	# find_package(glad CONFIG REQUIRED)
	find_package(glfw3 CONFIG REQUIRED)
	find_package(glm CONFIG REQUIRED)
	# link
	# target_link_libraries(MMDViewer PRIVATE glad::glad)
   	target_link_libraries(MMDViewer PRIVATE glfw)
	target_link_libraries(MMDViewer PRIVATE glm::glm)
endif(APPLE)

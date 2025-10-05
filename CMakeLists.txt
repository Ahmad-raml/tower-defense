cmake_minimum_required(VERSION 3.16)
project(TowerDefenseStarter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(USE_SFML "Build with SFML graphics" OFF)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(tower_defense ${CORE_SOURCES})
target_include_directories(tower_defense PRIVATE src)

if(USE_SFML)
  # Try SFML 3 first (vcpkg default), fall back to SFML 2 if someone has it.
  find_package(SFML 3 CONFIG QUIET COMPONENTS Graphics Window System)
  if(SFML_FOUND)
    add_compile_definitions(HAVE_SFML=1)
    target_link_libraries(tower_defense PRIVATE SFML::Graphics SFML::Window SFML::System)
  else()
    find_package(SFML 2 CONFIG REQUIRED COMPONENTS graphics window system)
    add_compile_definitions(HAVE_SFML=1)
    target_link_libraries(tower_defense PRIVATE sfml-graphics sfml-window sfml-system)
  endif()
endif() 

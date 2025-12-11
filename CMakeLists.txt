cmake_minimum_required(VERSION 3.28)
set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
project(CasinoRoyale LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ----------------------------
# SFML via FetchContent (works fine)
# ----------------------------
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

# ----------------------------
# Packages from VCPKG (manifest mode)
# ----------------------------
find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)

# ----------------------------
# Executable
# ----------------------------
add_executable(CasinoRoyale 
    src/main.cpp
    src/conductor.cpp
    src/i_component_array.cpp
    src/component_manager.cpp
    src/entity_manager.cpp
    src/system_manager.cpp
    src/network_manager.cpp
    src/component_serialization.cpp
    src/systems/player_input_system.cpp
    src/systems/basic_render_system.cpp
    src/systems/collision_detection_system.cpp
    src/systems/physics_system.cpp
    src/systems/jump_system.cpp
    src/systems/inventory_system.cpp
    src/systems/item_system.cpp
    src/systems/network_system.cpp
    src/help_functions.cpp
)

target_include_directories(CasinoRoyale PRIVATE include src)

# ----------------------------
# C++ Standard
# ----------------------------
target_compile_features(CasinoRoyale PRIVATE cxx_std_17)

# ----------------------------
# Linking
# ----------------------------
target_link_libraries(CasinoRoyale
    PRIVATE
        SFML::Graphics
        GameNetworkingSockets::GameNetworkingSockets
        OpenSSL::SSL
        OpenSSL::Crypto
)

target_include_directories(CasinoRoyale
    PRIVATE
        ${SFML_INCLUDE_DIRS}
        ${GameNetworkingSockets_INCLUDE_DIRS}
        ${OpenSSL_INCLUDE_DIRS}
)

# ----------------------------
# Run target
# ----------------------------
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CasinoRoyale${CMAKE_EXECUTABLE_SUFFIX}
    DEPENDS CasinoRoyale
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running CasinoRoyale..."
)
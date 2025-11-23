cmake_minimum_required(VERSION 3.28)
project(CasinoRoyale LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.2
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(CasinoRoyale 
    src/main.cpp
    src/conductor.cpp
    src/i_component_array.cpp
    src/component_manager.cpp
    src/entity_manager.cpp
    src/system_manager.cpp
    src/systems/player_input_system.cpp
    src/systems/basic_render_system.cpp
    src/systems/collision_detection_system.cpp
    src/systems/physics_system.cpp
    src/systems/jump_system.cpp
)

target_include_directories(CasinoRoyale PRIVATE include src)

target_compile_features(CasinoRoyale PRIVATE cxx_std_17)
set_target_properties(CasinoRoyale PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

target_link_libraries(CasinoRoyale PRIVATE SFML::Graphics)

# Add a custom target to run the executable
add_custom_target(run
    COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CasinoRoyale${CMAKE_EXECUTABLE_SUFFIX}
    DEPENDS CasinoRoyale
    COMMENT "Running CasinoRoyale..."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
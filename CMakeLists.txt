cmake_minimum_required(VERSION 3.14)
project(OOP_Game_Engine_ECS_Project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(GameEngineECS src/main.cpp)

set_target_properties(GameEngineECS PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

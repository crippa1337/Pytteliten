cmake_minimum_required(VERSION 3.24)
project(Pytteliten)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    # enable exceptions
    add_compile_options(/EHsc)

    # statically link to stdlib in release mode
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:DebugDLL>")
endif()

add_executable(Pytteliten
        main.cpp)

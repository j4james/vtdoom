cmake_minimum_required(VERSION 3.15)
project(vtdoom)

set(
    MAIN_FILES
    "src/main.cpp"
    "src/PureDOOM.c"
    "src/input.cpp"
    "src/os.cpp"
    "src/renderer.cpp"
)

set(
    DOC_FILES
    "README.md"
    "LICENSE.txt"
)

if(WIN32)
    add_compile_options(/EHsc-)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif()

add_executable(vtdoom ${MAIN_FILES})

if(UNIX)
    target_link_libraries(vtdoom -lpthread)
endif()

set_target_properties(vtdoom PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED On)
source_group("Doc Files" FILES ${DOC_FILES})

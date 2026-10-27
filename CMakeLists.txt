cmake_minimum_required(VERSION 3.20)
project(GcsCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(
    asio
    URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-38-2.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(asio)

add_library(asio_interface INTERFACE)
target_include_directories(asio_interface INTERFACE "${asio_SOURCE_DIR}/include")

FetchContent_Declare(
    glfw
    URL https://github.com/glfw/glfw/archive/refs/tags/3.5.1.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.93.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(imgui)

file(GLOB IMGUI_SOURCES
    "${imgui_SOURCE_DIR}/*.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)

FetchContent_Declare(
    nlohmann_json
    URL https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(GcsCore
    main.cpp
    MainWindow.h
    MainWindow.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(GcsCore PRIVATE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    "${asio_SOURCE_DIR}/include"
)

target_link_libraries(GcsCore PRIVATE
    asio_interface
    glfw
    opengl32
    nlohmann_json::nlohmann_json
)

if(MSVC)
    target_compile_options(GcsCore PRIVATE /Zc:__cplusplus)
    target_compile_definitions(GcsCore PRIVATE _WIN32_WINNT=0x0A00 ASIO_STANDALONE)
endif()
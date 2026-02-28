cmake_minimum_required(VERSION 3.20)
project(joycon2_connector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options("/Zc:char8_t-")
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include_directories(
  src
  src/imgui
  ${CMAKE_SOURCE_DIR}/include
)

# Core application sources (exclude old testapp.cpp)
set(APP_SOURCES
  src/App.cpp
  src/JoyConDecoder.cpp
)

# Dear ImGui sources
set(IMGUI_SOURCES
  src/imgui/imgui.cpp
  src/imgui/imgui_draw.cpp
  src/imgui/imgui_tables.cpp
  src/imgui/imgui_widgets.cpp
  src/imgui/imgui_impl_win32.cpp
  src/imgui/imgui_impl_dx11.cpp
)

add_executable(joycon2_connector WIN32 ${APP_SOURCES} ${IMGUI_SOURCES})

target_link_directories(joycon2_connector PRIVATE ${CMAKE_SOURCE_DIR}/lib)
target_link_libraries(joycon2_connector
    PRIVATE
        setupapi
        hid
        ViGEmClient
        windowsapp
        d3d11
        dxgi
        d3dcompiler
)

if(MSVC)
  target_compile_options(joycon2_connector PRIVATE /W3 /permissive- /utf-8)
else()
  target_compile_options(joycon2_connector PRIVATE -Wall -Wextra -pedantic)
endif()

# Copy resources to output directory
add_custom_command(TARGET joycon2_connector POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:joycon2_connector>/resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/NotoSansSC-Regular.otf"
        "$<TARGET_FILE_DIR:joycon2_connector>/resources/NotoSansSC-Regular.otf"
)

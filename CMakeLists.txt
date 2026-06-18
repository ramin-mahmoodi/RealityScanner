cmake_minimum_required(VERSION 3.16)
project(RealityScanner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# 1. Fetch ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)
FetchContent_MakeAvailable(imgui)

# 2. Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
# Disable building GLFW tests and examples
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 3. Fetch nlohmann_json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(RealityScanner WIN32
    src/main.cpp
    src/app.cpp
    src/xui_api.cpp
    src/scanner.cpp
    src/utils.cpp
    resource.rc
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(RealityScanner PRIVATE 
    src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Fully static linking so no DLLs are required on other PCs
if (MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_options(RealityScanner PRIVATE "-static" "-static-libgcc" "-static-libstdc++")
endif()

target_link_libraries(RealityScanner PRIVATE
    glfw
    nlohmann_json::nlohmann_json
)

# Windows specific libraries for WinHttp, UI and UUID
if (WIN32)
    target_link_libraries(RealityScanner PRIVATE winhttp ws2_32 crypt32 wsock32 user32 shell32 opengl32 rpcrt4)
endif()

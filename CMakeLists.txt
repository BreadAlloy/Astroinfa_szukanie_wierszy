cmake_minimum_required(VERSION 3.5.0) # Bo to mam

project(AlgorytmyHeurystyczne LANGUAGES CXX C)
# C dla SOIL

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings  -Wextra -Wpedantic
    add_compile_options(-Wall)
endif()

set(VERBOSE true)

# set(CMAKE_C_COMPILER "C:\\msys64\\ucrt64\\bin\\gcc.exe ")
# set(CMAKE_CXX_COMPILER "C:\\msys64\\ucrt64\\bin\\g++.exe ")

# cmake_policy(SET CMP0141 NEW)
# set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

# set(CMAKE_CUDA_FLAGS ${CMAKE_CUDA_FLAGS} ${CMAKE_CUDA_DEBUG_FLAGS})  # enable cuda-gdb

# ---=== INSTALL DEPENDENCIES ===---

include(FetchContent)

# Setup OpenGL
cmake_policy(SET CMP0072 NEW) # Pefer GLVND over legacy GL libraries
find_package(OpenGL REQUIRED)


# Setup GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw"
    GIT_TAG "3.3.8"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

# Setup ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui"
    GIT_TAG "v1.92.7"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui)
set(IMGUI_SOURCE
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCE})
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR};${imgui_SOURCE_DIR}/backends/")
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Setup ImPlot
FetchContent_Declare(
    implot
    GIT_REPOSITORY "https://github.com/epezent/implot"
    GIT_TAG "v0.17"
    GIT_PROGRESS TRUE
)

FetchContent_Declare(
        jsoncpp
        GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
        GIT_TAG        master
)

FetchContent_MakeAvailable(jsoncpp)

# To by wypadalo wykonac tylko raz przy pobraniu
message("Adding to imconfig.h")
file(APPEND ${imgui_SOURCE_DIR}/imconfig.h
"
#undef ImDrawIdx
// opcja 1 fixa z readme implot
#define ImDrawIdx unsigned int"
)

FetchContent_MakeAvailable(implot)
set(IMPLOT_SOURCE
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_demo.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)


add_library(implot STATIC ${IMPLOT_SOURCE})
target_include_directories(implot PUBLIC ${implot_SOURCE_DIR})
target_link_libraries(implot PUBLIC imgui)

# Setup ImPlot3D
FetchContent_Declare(
    implot3d
    GIT_REPOSITORY "https://github.com/brenocq/implot3d"
    GIT_TAG "v0.4"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(implot3d)

set(IMPLOT3D_SOURCE
    ${implot3d_SOURCE_DIR}/implot3d.cpp
    ${implot3d_SOURCE_DIR}/implot3d_demo.cpp
    ${implot3d_SOURCE_DIR}/implot3d_items.cpp
    ${implot3d_SOURCE_DIR}/implot3d_meshes.cpp
)

add_library(implot3d STATIC ${IMPLOT3D_SOURCE})
target_include_directories(implot3d PUBLIC ${implot3d_SOURCE_DIR})
target_link_libraries(implot3d PUBLIC imgui)
if(IMPLOT3D_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(implot3d PRIVATE /W4 /WX)
    else()
        target_compile_options(implot3d PRIVATE -Wall -Wextra -Werror)
    endif()
endif()


# ---=== PROJECT COMPILE ===---

# # find_package( CUDA  REQUIRED )

# set(GLOBAL_INCLUDE_DIRS
#     ${CUDA_INCLUDE_DIRS}
#     # ${CMAKE_CURRENT_SOURCE_DIR}/CUDA/Common # wszystko ma cude
# )

set(imgui_INCLUDE_DIRS
    ${glfw_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
    ${implot3d_SOURCE_DIR}
)

# if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
#   set(CMAKE_CUDA_ARCHITECTURES 86)
# endif()

add_library(SOIL STATIC
    SOIL/image_DXT.c
    SOIL/image_helper.c
    SOIL/SOIL.c
    SOIL/stb_image_aug.c
)

add_executable(
    "zad1_exec" "lab1/lab1.cpp"
)
target_include_directories("zad1_exec" PUBLIC ${jsoncpp_INCLUDE_DIR})
target_link_libraries(zad1_exec PUBLIC jsoncpp_static aio)

# set_target_properties( exec PROPERTIES CUDA_SEPARABLE_COMPILATION ON )

# target_link_libraries(
#   exec PRIVATE
#   implot implot3d
# )

# target_link_libraries(
#   exec PUBLIC
#   SOIL
# )

# Silence OpenGL deprecation warnings on macOS
if(APPLE)
    target_compile_definitions(AlgorytmyHeurystyczne PRIVATE GL_SILENCE_DEPRECATION)
endif()

# So that clangd sees things installed by cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
file(COPY ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/)
# endif()
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)

set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/thirdparty)
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(INCLUDE_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)
set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/examples)

# RF frontend driver
find_package(SoapySDR REQUIRED)

# SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# OpenGL/GLEW
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# FFTW3
# find_package(FFTW3 REQUIRED)

include_directories(${INCLUDE_DIR})
include_directories(${INCLUDE_DIR}/ui)
include_directories(${INCLUDE_LIB_DIR})


# ImGui
add_library(imgui STATIC
    ${THIRD_PARTY_DIR}/imgui/imgui.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_draw.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_demo.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_tables.cpp
    ${THIRD_PARTY_DIR}/imgui/imgui_widgets.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_sdl2.cpp
    ${THIRD_PARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC ${THIRD_PARTY_DIR}/imgui)
target_compile_options(imgui PRIVATE -fPIC)

# ImPlot
add_library(implot STATIC
    ${THIRD_PARTY_DIR}/implot/implot.cpp
    ${THIRD_PARTY_DIR}/implot/implot_demo.cpp
    ${THIRD_PARTY_DIR}/implot/implot_items.cpp
)
target_include_directories(implot PUBLIC ${THIRD_PARTY_DIR}/implot)
target_link_libraries(implot PRIVATE imgui)
target_compile_options(implot PRIVATE -fPIC)

# Добавляем директорию lib с нашими функциями обработки сигнала
add_subdirectory(./lib)

set(SOURCES ${SRC_DIR}/main.cpp
            ${SRC_DIR}/pluto_sdr_lib.cpp
            ${SRC_DIR}/ui/pluto_ui.cpp
            ${INCLUDE_LIB_DIR}/time_sync.cpp
            ${INCLUDE_LIB_DIR}/freq_sync.cpp
            ${INCLUDE_LIB_DIR}/bit_generation.cpp
            ${INCLUDE_LIB_DIR}/math_operations.cpp
            ${INCLUDE_LIB_DIR}/modulation.cpp
            ${INCLUDE_LIB_DIR}/pulse_shaping.cpp)

add_executable(main ${SOURCES})
target_link_libraries(main PRIVATE imgui implot ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${SoapySDR_LIBRARIES} mylib fftw3)

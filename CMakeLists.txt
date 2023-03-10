# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
# Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
# SPDX-License-Identifier: ISC

cmake_minimum_required(VERSION 3.7)

set(CMAKE_CXX_STANDARD 17)

set(NAME synth303maker)
project(${NAME})

set(CMAKE_VERBOSE_MAKEFILE on)

add_subdirectory(dpf)

dpf_add_plugin(${NAME}
  TARGETS jack vst2 clap
  FILES_DSP
      src/PluginDSP.cpp
      src/synth303common.cpp
  FILES_UI
      src/PluginUI.cpp
      src/synth303common.cpp
      implot/implot.cpp
      implot/implot_demo.cpp
      implot/implot_items.cpp
      dpf-widgets/opengl/DearImGui.cpp)

target_compile_definitions(${NAME} PUBLIC IMGUI_USER_CONFIG="synth303_imconfig.h")

add_subdirectory(mingw-std-threads)
target_link_libraries(${NAME} PRIVATE mingw-std-threads)
target_include_directories(${NAME} PUBLIC mingw-std-threads)
target_include_directories(${NAME} PUBLIC mingw-compat)

target_include_directories(${NAME} PUBLIC src)
target_include_directories(${NAME} PUBLIC dpf-widgets/generic)
target_include_directories(${NAME} PUBLIC dpf-widgets/opengl)
target_include_directories(${NAME} PUBLIC dpf-widgets/opengl/DearImGui)
target_include_directories(${NAME} PUBLIC implot)
target_include_directories(${NAME} PUBLIC chowdsp_utils/modules/dsp)
target_include_directories(${NAME} PUBLIC chowdsp_wdf/include)
target_include_directories(${NAME} PUBLIC sst-filters/include)
target_include_directories(${NAME} PUBLIC chowdsp_utils/modules/common)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_subdirectory(chowdsp_utils)

# create static library based on these modules
setup_chowdsp_lib(chowdsp_lib               # Name of the static library
    MODULES chowdsp_sources chowdsp_dsp_utils chowdsp_filters # DSP modules that should be included in the library...
)

# link the static library to your project
target_link_libraries(${NAME} PRIVATE chowdsp_lib)

add_subdirectory(sst-filters)
target_link_libraries(${NAME} PUBLIC sst-filters)


cmake_minimum_required(VERSION 3.19)
project(morphism3d)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(is_multi_config)
    set(bindir ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
    set(is_debug $<CONFIG:Debug>)
    set(is_release $<CONFIG:Release>)
    message(STATUS "Current build mode: Multi Config")
else()
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE Debug)
    endif()

    set(bindir ${CMAKE_CURRENT_BINARY_DIR})
    set(is_debug $<STREQUAL:${CMAKE_BUILD_TYPE},Debug>)
    set(is_release $<STREQUAL:${CMAKE_BUILD_TYPE},Release>)
    message(STATUS "Current build mode: Single Config, ${CMAKE_BUILD_TYPE}")
endif()

# Compiler options
add_library(compiler-flags INTERFACE)
target_compile_features(compiler-flags INTERFACE cxx_std_23)
set(is_msvc "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
set(is_gcc "$<COMPILE_LANG_AND_ID:CXX,GNU>")
target_compile_options(
    compiler-flags
    INTERFACE
    "$<${is_msvc}:-W3>"
    # Note: CMake uses -O3 by default when build type is Release
    "$<${is_gcc}:$<${is_debug}:-Og>>"
)
target_compile_definitions(compiler-flags INTERFACE "$<${is_debug}:DEBUG>")

# Set up libraries
# If Vulkan can't be found, set the env variable VULKAN_SDK to the root
# directory of the Vulkan SDK.
find_package(Vulkan REQUIRED) # Developed with 1.4.321.1
add_subdirectory("lib/glfw")

# Application source code
add_executable(main "src/main.cpp" icon.rc)
target_sources(main PRIVATE "src/camera_control.cpp")
target_sources(main PRIVATE "src/container.cpp")
target_sources(main PRIVATE "src/entity.cpp")
target_sources(main PRIVATE "src/event.cpp")
target_sources(main PRIVATE "src/expression.cpp")
target_sources(main PRIVATE "src/graph.cpp")
target_sources(main PRIVATE "src/primitive.cpp")
target_sources(main PRIVATE "src/statistics.cpp")
target_sources(main PRIVATE "src/stb_impl.cpp")
target_sources(main PRIVATE "src/temporal.cpp") # Would prefer "time.h" but that's taken.
target_sources(main PRIVATE "src/type.cpp")
target_sources(main PRIVATE "src/ui/camera_window.cpp")
target_sources(main PRIVATE "src/ui/common.cpp")
target_sources(main PRIVATE "src/ui/debug_window.cpp")
target_sources(main PRIVATE "src/ui/graph_window.cpp")
target_sources(main PRIVATE "src/ui/imgui.cpp")
target_sources(main PRIVATE "src/ui/render_window.cpp")
target_sources(main PRIVATE "src/ui/stats_window.cpp")
target_sources(main PRIVATE "src/ui/ui.cpp")
target_sources(main PRIVATE "src/vk/camera.cpp")
target_sources(main PRIVATE "src/vk/datatypes.cpp")
target_sources(main PRIVATE "src/vk/device.cpp")
target_sources(main PRIVATE "src/vk/helper.cpp")
target_sources(main PRIVATE "src/vk/renderer.cpp")
target_sources(main PRIVATE "src/window.cpp")
set_target_properties(main PROPERTIES OUTPUT_NAME "morphism3d")

# ImGui sources
target_sources(main PRIVATE "lib/imgui/imgui.cpp")
target_sources(main PRIVATE "lib/imgui/imgui_demo.cpp")
target_sources(main PRIVATE "lib/imgui/imgui_draw.cpp")
target_sources(main PRIVATE "lib/imgui/imgui_tables.cpp")
target_sources(main PRIVATE "lib/imgui/imgui_widgets.cpp")

# ImGui backends
target_sources(main PRIVATE "lib/imgui/backends/imgui_impl_glfw.cpp")
target_sources(main PRIVATE "lib/imgui/backends/imgui_impl_vulkan.cpp")

# Application and lib includes
target_include_directories(main PRIVATE "src")
target_include_directories(main PRIVATE ${Vulkan_INCLUDE_DIRS})
target_include_directories(main PRIVATE "lib/glm")
target_include_directories(main PRIVATE "lib/imgui")
target_include_directories(main PRIVATE "lib/imgui/backends")
target_include_directories(main PRIVATE "lib/stb")

target_link_libraries(main PRIVATE compiler-flags glfw Vulkan::Vulkan)

# Compile shaders
function(add_shader SOURCE OUTNAME)
    set(INFILE ${CMAKE_CURRENT_SOURCE_DIR}/shader/${SOURCE})
    set(OUTFILE ${bindir}/${OUTNAME}.spv)

    add_custom_command(
        OUTPUT ${OUTFILE}
        COMMAND Vulkan::glslc ${INFILE} -o ${OUTFILE}
        DEPENDS ${INFILE}
        VERBATIM)

    target_sources(main PRIVATE ${OUTFILE})
endfunction()

add_shader(unlit.vert unlit_vert)
add_shader(unlit.frag unlit_frag)
add_shader(lit.vert lit_vert)
add_shader(lit.frag lit_frag)

function(add_icon SIZE)
    set(FNAME icon${SIZE}.png)

    add_custom_command(
        TARGET main POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/${FNAME}
        ${bindir}/${FNAME})
endfunction()

add_icon(16)
add_icon(32)

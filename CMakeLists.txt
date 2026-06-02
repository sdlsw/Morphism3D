cmake_minimum_required(VERSION 3.19)
project(graph3d)

# Compiler options
add_library(compiler-flags INTERFACE)
target_compile_features(compiler-flags INTERFACE cxx_std_23)
set(msvc "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
target_compile_options(compiler-flags INTERFACE "$<${msvc}:-W3>")
target_compile_definitions(compiler-flags INTERFACE "$<$<CONFIG:Debug>:DEBUG>")

# Set up libraries
# If Vulkan can't be found, set the env variable VULKAN_SDK to the root
# directory of the Vulkan SDK.
find_package(Vulkan REQUIRED) # Developed with 1.4.321.1
add_subdirectory("lib/glfw")

# Application source code
add_executable(main "src/main.cpp" icon.rc)
target_sources(main PUBLIC "src/stb_impl.cpp")
target_sources(main PUBLIC "src/camera_control.cpp")
target_sources(main PUBLIC "src/container.cpp")
target_sources(main PUBLIC "src/entity.cpp")
target_sources(main PUBLIC "src/event.cpp")
target_sources(main PUBLIC "src/expression.cpp")
target_sources(main PUBLIC "src/graph.cpp")
target_sources(main PUBLIC "src/primitive.cpp")
target_sources(main PUBLIC "src/type.cpp")
target_sources(main PUBLIC "src/ui.cpp")
target_sources(main PUBLIC "src/vk/camera.cpp")
target_sources(main PUBLIC "src/vk/datatypes.cpp")
target_sources(main PUBLIC "src/vk/device.cpp")
target_sources(main PUBLIC "src/vk/helper.cpp")
target_sources(main PUBLIC "src/vk/renderer.cpp")
target_sources(main PUBLIC "src/window.cpp")
set_target_properties(main PROPERTIES OUTPUT_NAME "graph3d")

# ImGui sources
target_sources(main PUBLIC "lib/imgui/imgui.cpp")
target_sources(main PUBLIC "lib/imgui/imgui_demo.cpp")
target_sources(main PUBLIC "lib/imgui/imgui_draw.cpp")
target_sources(main PUBLIC "lib/imgui/imgui_tables.cpp")
target_sources(main PUBLIC "lib/imgui/imgui_widgets.cpp")

# ImGui backends
target_sources(main PUBLIC "lib/imgui/backends/imgui_impl_glfw.cpp")
target_sources(main PUBLIC "lib/imgui/backends/imgui_impl_vulkan.cpp")

# Application and lib includes
target_include_directories(main PUBLIC "src")
target_include_directories(main PUBLIC "src/vk")
target_include_directories(main PUBLIC ${Vulkan_INCLUDE_DIRS})
target_include_directories(main PUBLIC "lib/glm")
target_include_directories(main PUBLIC "lib/imgui")
target_include_directories(main PUBLIC "lib/imgui/backends")
target_include_directories(main PUBLIC "lib/stb")

target_link_libraries(main PUBLIC compiler-flags glfw Vulkan::Vulkan)

# Compile shaders
function(add_shader SOURCE OUTNAME)
    set(OUTFILE ${CMAKE_CURRENT_BINARY_DIR}/${OUTNAME}.spv)
    set(INFILE ${CMAKE_CURRENT_SOURCE_DIR}/shader/${SOURCE})

    add_custom_command(
        OUTPUT ${OUTFILE}
        COMMAND Vulkan::glslc ${INFILE} -o ${OUTFILE}
        DEPENDS ${INFILE}
        VERBATIM)

    target_sources(main PUBLIC ${OUTFILE})
endfunction()

add_shader(unlit.vert unlit_vert)
add_shader(unlit.frag unlit_frag)
add_shader(lit.vert lit_vert)
add_shader(lit.frag lit_frag)

# Copy icon
add_custom_command(
    TARGET main POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/logosmall.png
        ${CMAKE_CURRENT_BINARY_DIR}/icon.png)

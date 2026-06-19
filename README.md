# Overview

Morphism3D is a real time, interactive visualization program that can graph arbitrary functions of two variables (more graph types planned).

# Tour

Here's a brief demo of some of the current features of Morphism3D.

### Arbitrary Expressions

Morphism3D understands a miniature expression language that can be used to express a wide variety of functions. The graph is updated as soon as a valid expression is detected.

https://github.com/user-attachments/assets/a00e2b8a-78ee-4aa7-a7cb-24d47dc88092

### Variable Sliders

Variables may be inserted into expressions, which can be manipulated interactively with UI elements.

https://github.com/user-attachments/assets/b206ae87-a43d-4f0d-abe7-9b6d514329ed

### Real-time Animation

The variable `t` is always set to the number of seconds since the application started. This may be used in periodic functions to animate the graph.

https://github.com/user-attachments/assets/36815dff-9304-4fb7-a5e7-7ebc919a65eb

### Orthographic Projection

Morphism3D supports both orthographic and perspective projections, and can snap the view to the axes.

https://github.com/user-attachments/assets/f59a9495-8405-4e2c-961a-4cee13968e3f

### Freecam

In "free" mode, the camera is not fixed looking at the origin.

https://github.com/user-attachments/assets/f3e6716f-7867-47a9-b2a4-591e0ea2bff6

### Render Modes

A variety of different render modes and options are supported.

https://github.com/user-attachments/assets/d1717643-5797-4116-bbbc-ea617571c2d7

# Dependencies

Morphism3D depends on the following components:

- [GLFW](https://www.glfw.org/)
- [GLM](http://glm.g-truc.net/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [stb_image.h](https://github.com/nothings/stb/blob/master/stb_image.h)
- [Vulkan SDK](https://www.lunarg.com/products/vulkan-sdk/)

All dependencies except the Vulkan SDK are included as submodules at [`lib`](lib).

# Building

> [!NOTE]
> If CMake complains that Vulkan can't be found, the `VULKAN_SDK` environment variable must be set to the root directory of the installed SDK. Some platforms will set this variable automatically.

Morphism3D uses the [CMake](https://cmake.org/) build system. See the first line of [CMakeLists.txt](CMakeLists.txt) for the minimum required version of CMake.
The only officially supported compiler is GCC, used on Windows through the [MSYS2](https://www.msys2.org/) environment.

To set up the build:
```sh
git checkout https://github.com/sdlsw/Morphism3D.git
git submodule init
git submodule update
mkdir build
cd build
```

The next step depends on your choice of [CMake generator](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html). For single config generators like Ninja or Make:

```sh
# (in build directory)
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

For multi config generators like Visual Studio, Xcode or Ninja Multi-Config:

```sh
# (in build directory)
cmake ..
cmake --build . --config Release
```

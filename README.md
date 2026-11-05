# Overview

Morphism3D is a real time, interactive visualization program that can graph arbitrary functions of the following types:

- Two input variables (`x`, `y`), one output variable (`z`)
- One input variable (`x`), two output variables (`y`, `z`)

# Download

See [Releases](https://github.com/sdlsw/Morphism3D/releases). Pick the appropriate ZIP file for your platform, extract it, and run. No install necessary.

# Tour

Here's a brief demo of some of the current features of Morphism3D.

### Arbitrary Expressions

Morphism3D understands a miniature expression language that can be used to express a wide variety of functions. The graph is updated as soon as a valid expression is detected.

https://github.com/user-attachments/assets/472fa3ab-2a36-4ace-8098-cfa9bb0d16b5

### Variable Sliders

Variables may be inserted into expressions, which can be manipulated interactively with UI elements.

https://github.com/user-attachments/assets/d68c3e93-6018-4b80-a799-2b976ea83689

### Real-time Animation

The "Animator" figure may be used to animate arbitrary variables.

https://github.com/user-attachments/assets/993c0e61-746a-4861-a809-8625edb04172

### Orthographic Projection

Morphism3D supports both orthographic and perspective projections, and can snap the view to the axes.

https://github.com/user-attachments/assets/f59a9495-8405-4e2c-961a-4cee13968e3f

### Freecam

In "free" mode, the camera is not fixed looking at the origin.

https://github.com/user-attachments/assets/f3e6716f-7867-47a9-b2a4-591e0ea2bff6

### Render Modes

A variety of different render modes and options are supported.

https://github.com/user-attachments/assets/9cf87d63-66d7-4db7-bd01-12e943e9e825

### Alternate Graphing Modes

The "Line Graph" figure allows plotting functions of one variable:

https://github.com/user-attachments/assets/f2dd211a-e76b-47a9-8cbf-3a1d82cb898d

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
The only officially supported compiler is GCC. On Windows, the build is accomplished through the [MSYS2](https://www.msys2.org/) environment.

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

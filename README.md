[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/mortennobel/SimpleRenderEngine/master/LICENSE)
[![Build Status](https://travis-ci.org/mortennobel/SimpleRenderEngine.svg?branch=master)](https://travis-ci.org/mortennobel/SimpleRenderEngine)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/86403818b8b54161a6fef03248c0b828)](https://www.codacy.com/app/mortennobel/SimpleRenderEngine?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=mortennobel/SimpleRenderEngine&amp;utm_campaign=Badge_Grade)

# SimpleRenderEngine

This is an update to Morten Nobel-Jørgensen's SimpleRenderEngine. The goals of the updates are to make the interface easier to use (e.g. not as dependent on advanced C++11/14 features like Lambdas) and with less exposure to OpenGL internals (e.g. by abstracting away direct reliance on transformation matrices). Several build issues have also been fixed.

Two new cameras (derived from the base camera class) have been added:
1) A basic "First-Person Surveyor" (e.g. a Minecraft-like) camera (demonstrated in 'examples/18_FPS-camera.cpp')
2) A basic "Flight" camera (demonstrated in 'examples/19_Flight-camera.cpp')

The goal of SimpleRenderEngine (sre) is to provide easy way to get started with graphics programming in 2D or 3D without a deep knowledge about the low-level graphics APIs like OpenGL, DirectX and Vulkan.
 
SimpleRenderEngine currently depends on Simple Direct Layer 2.x (SDL2), SDL2-image, OpenGL Mathematics (GLM), and OpenGL 3.3 (or higher), Dear ImGui and runs on Windows, macOS, and Linux.
 
sre provides:
 * First-Person Surveyor (e.g. Minecraft-like) camera
 * Flight camera
 * Virtual camera (perspective and orthographic)
 * Texture support (JPEG, PNG)
 * Cube map support
 * Mesh support (with custom vertex attributes)
 * Shaders (PBR, Blinn-Phong, unlit, alpha blending, and custom shaders)
 * Enforces efficient use of OpenGL
 * Forward rendering
 * Full C++14 support
 * Support for 2D or 3D rendering
 * GUI rendering (using Dear ImGui)
 * Emscripten support (allows cross compiling to HTML 5 + WebGL)
 * VR support (OpenVR)
 * Bump mapping
 * Shadowmap

To keep sre as simple and flexible as possible the following features are not a part of sre:
 * Scenegraphs
 * Deferred rendering
 * Dynamic particle systems

## Build Instructions

### Ubuntu Linux:

#### First get your Ubuntu installation fully up-to-date:

Use the Ubuntu user-interface to do an upgrade to the latest version of Ubuntu (this is not necessary, but is recommended) and perform updates until the upgrade is fully up-to-date. Then follow these steps::

 * sudo apt full-upgrade
 * sudo apt update
 * sudo apt autoremove

#### Install compilers and build tools (if not installed already):

 * sudo apt install g++
 * sudo apt install git
 * sudo apt install make
 * sudo apt install cmake
 * sudo apt install cmake-curses-gui

#### Install libraries (if not installed already):

 * sudo apt install libsdl2-dev
 * sudo apt install libsdl2-image-dev
 * sudo apt install libglew-dev

#### Install SRE:
 
Clone the library and note that all the updates are currently only available on the 'develop2' branch (they are in a 'Beta' state). Take the following steps:

 * cd directory-to-install-into 
 * git clone --recurse-submodules https://github.com/estrac/SimpleRenderEngine.git sre
 * git checkout develop2
 * mkdir -p ../sre_build
 * cmake full-path-to/directory-to-install-into/sre -DOpenGL_GL_PREFERENCE:TYPE=GLVND
 * cd ../sre_build
 * make
 * cd examples
 * ./SRE-Example-00_hello-engine
 * try other examples

## Examples
 
Example usage can be found in the examples folder.

[![Matcap](https://mortennobel.github.io/SimpleRenderEngine/examples/07_matcap.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/07_matcap.html)[![Picking](https://mortennobel.github.io/SimpleRenderEngine/examples/09_picking.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/09_picking.html)[![Skybox](https://mortennobel.github.io/SimpleRenderEngine/examples/10_skybox-example.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/10_skybox-example.html)[![Render to texture](https://mortennobel.github.io/SimpleRenderEngine/examples/12_render-to-texture.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/12_render-to-texture.html)[![Cloth_Simulation](https://mortennobel.github.io/SimpleRenderEngine/examples/15_cloth_simulation.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/15_cloth_simulation.html)[![Shadows](https://mortennobel.github.io/SimpleRenderEngine/examples/16_shadows.png)](https://mortennobel.github.io/SimpleRenderEngine/examples/16_shadows.html)[![GLSL Editor](https://mortennobel.github.io/SimpleRenderEngine/examples/glsl_editor.png)](https://github.com/mortennobel/sre_glsl_editor)[![Platformer](https://mortennobel.github.io/SimpleRenderEngine/examples/platformer.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/platformer)[![Particle system](https://mortennobel.github.io/SimpleRenderEngine/examples/particle-system.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/particle_system)[![ImGUI integration](https://mortennobel.github.io/SimpleRenderEngine/examples/gui.png)](https://github.com/mortennobel/SimpleRenderEngineProject/tree/master/project/gui)

## Documentation

API documentation is defined in header files, usage is shown in the example files.
 
## Other resources
 
 * https://www.libsdl.org Simple Direct Layer 2.x 
 * https://www.libsdl.org/projects/SDL_image/ Simple Direct Layer Image 2.x
 * http://glm.g-truc.net/ OpenGL Mathematics (bundled as submodule)
 * https://www.opengl.org/registry/ OpenGL Registry
 * https://github.com/ocornut/imgui ImGui 1.60 (submodule)
 * https://github.com/BalazsJako/ImGuiColorTextEdit ImGuiColorTextEdit (bundled as submodule)
 * https://github.com/kazuho/picojson PicoJSON (bundled as submodule)

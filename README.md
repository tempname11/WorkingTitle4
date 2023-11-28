# Working Title

## Notice

Please see https://tempname11.github.io/2021 for a more detailed description of the project.

## Build

Make sure you are on Windows and have a recent version of the Vulkan SDK installed (last tested: `1.3.268.0`). After cloning, do:

```bash
git submodule update --init
```

Then, build with CMake. Preferred compiler is MSVC, but Clang should also work.

## Usage

You will need a GPU with ray tracing capabilities (grep for `device_extensions` to see the exact Vulkan requirements), because RT is extensively used in the GI implementation. Launch `engine.exe` for a blank engine startup, or `script_default.exe` for a simple predefined scene.

Press `~` (tilde) to toggle the debug UI. You can convert GLTF meshes (for example, the classic Sponza) into an internal format and then load it into the engine. Same applies to MagicaVoxel meshes.

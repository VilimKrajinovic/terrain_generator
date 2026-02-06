# Terrain Simulator

A Vulkan-based terrain simulation application written in C.

## Requirements

- CMake 3.20+
- C11 compiler (Clang or GCC)
- Vulkan SDK
- SDL3

### macOS

```bash
brew install sdl3
```

Download and install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).

### Linux

```bash
# Ubuntu/Debian
sudo apt install libsdl3-dev libvulkan-dev vulkan-tools

# Fedora
sudo dnf install SDL3-devel vulkan-loader-devel vulkan-tools
```

## Building

```bash
./build.sh
```

Or manually:

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

## Running

```bash
./build.sh run
```

Or manually:

```bash
./build/terrain_sim
```

Press `ESC` to exit the application.

## Project Structure

```
src/
├── core/           # Application core (app, logging)
├── foundation/     # Shared foundation types (result codes)
├── geometry/       # Mesh and geometry (quad)
├── memory/         # Arena allocator and memory lifetimes
├── platform/       # Platform abstraction (window, input)
├── renderer/       # Vulkan renderer
│   ├── renderer_init.c     # Renderer setup and teardown
│   ├── renderer_frame.c    # Per-frame rendering path
│   ├── renderer_swapchain.c# Swapchain/framebuffer lifecycle
│   ├── vk_instance.c   # Vulkan instance
│   ├── vk_device.c     # Device selection
│   ├── vk_swapchain.c  # Swapchain management
│   ├── vk_pipeline.c   # Graphics pipeline
│   ├── vk_shader.c     # Shader loading
│   └── ...
├── simulation/     # Simulation lifecycle (init/update/shutdown)
├── utils/          # Utilities (types, macros, file I/O)
└── main.c          # Entry point
shaders/
├── basic.vert      # Vertex shader
└── basic.frag      # Fragment shader
```

## Memory Management

The project uses arena-based allocation following Ryan Fleury's pattern:

- **Permanent Arena** : All application-lifetime allocations
- **Scratch Arena** : Temporary allocations within function scope

This eliminates individual malloc/free calls and enables bulk deallocation.

More about it can be found [here on his blog post](https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator).

## License

MIT

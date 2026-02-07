# Terrain Simulator

A Vulkan-based terrain simulation application written in C++20 with a C-style procedural architecture.

## Requirements

- CMake 3.20+
- C++20 compiler (Clang or GCC)
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

## C++ Runtime Policy

The codebase stays C-style in C++:

- Procedural APIs and plain structs (no OOP redesign required)
- `Result`/error-code flow for failures
- Exceptions and RTTI disabled in build flags

## Project Structure

```
src/
├── core/           # Application core (app, logging)
├── foundation/     # Shared foundation types (result codes)
├── geometry/       # Mesh and geometry (quad)
├── memory/         # Arena allocator and memory lifetimes
├── platform/       # Platform abstraction (window, input)
├── renderer/       # Vulkan renderer
│   ├── renderer_init.cpp     # Renderer setup and teardown
│   ├── renderer_frame.cpp    # Per-frame rendering path
│   ├── renderer_swapchain.cpp# Swapchain/framebuffer lifecycle
│   ├── vk_instance.cpp   # Vulkan instance
│   ├── vk_device.cpp     # Device selection
│   ├── vk_swapchain.cpp  # Swapchain management
│   ├── vk_pipeline.cpp   # Graphics pipeline
│   ├── vk_shader.cpp     # Shader loading
│   └── ...
├── simulation/     # Simulation lifecycle (init/update/shutdown)
├── utils/          # Utilities (types, macros, file I/O)
└── main.cpp        # Entry point
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

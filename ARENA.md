# Arena Allocator Documentation

This document explains the arena allocator system used in the Terrain Simulator project.

## What is an Arena Allocator?

An **arena allocator** (also called a bump allocator or linear allocator) is a simple memory management strategy that allocates from a single contiguous block of memory.

### How It Works

Instead of tracking individual allocations like `malloc`/`free`, an arena just:
1. Maintains a pointer to the next free byte
2. Bumps that pointer forward on each allocation
3. Frees everything at once when done

```
┌─────────────────────────────────────────────────────────────┐
│                        Arena Memory                         │
├─────────────────────────────────────────────────────────────┤
│ [Alloc 1] [Alloc 2] [  Alloc 3  ] [    Free Space...    ]  │
└─────────────────────────────────────────────────────────────┘
     ▲                               ▲                      ▲
     │                               │                      │
   base                             pos                   size
```

- **base**: Start of the memory block
- **pos**: Current allocation position (bump pointer)
- **size**: Total arena capacity

### Why Use Arenas?

| Benefit | Explanation |
|---------|-------------|
| **Fast allocation** | Just increment a pointer - O(1) always |
| **No fragmentation** | Memory is always contiguous |
| **Bulk deallocation** | Reset `pos = 0` to free everything instantly |
| **Cache friendly** | Sequential allocations are adjacent in memory |
| **Simple tracking** | No per-allocation metadata overhead |

### The Trade-off

You **cannot free individual allocations**. Everything in an arena is freed together. This makes arenas ideal for:
- Frame-based allocations (allocate during frame, free at end)
- Loading operations (load file, process, free all)
- Groups of related objects with the same lifetime

## Arena Types in This Project

### 1. Permanent Arena

Located in `AppContext.permanent_arena`. Used for allocations that live for the entire application lifetime.

```c
// In app.c
app->permanent_arena = arena_create(2 * 1024 * 1024);  // 2MB

// Usage: meshes, renderer contexts, etc.
Mesh *mesh = ARENA_PUSH_STRUCT(&app->permanent_arena, Mesh);
```

### 2. Scratch Arena

A global arena for temporary allocations. Automatically rolls back when the scope ends.

```c
// Initialized in main.c
arena_scratch_init(256 * 1024);  // 256KB

// Usage: temporary buffers, file loading, etc.
ArenaTemp temp = arena_scratch_begin();
u8 *buffer = arena_alloc(temp.arena, 1024);
// ... use buffer ...
arena_temp_end(temp);  // Memory automatically "freed"
```

## API Reference

### Structs

```c
typedef struct Arena {
    u8 *base;       // Base pointer to memory block
    size_t size;    // Total capacity in bytes
    size_t pos;     // Current allocation position
} Arena;

typedef struct ArenaTemp {
    Arena *arena;   // Pointer to the arena
    size_t pos;     // Saved position for rollback
} ArenaTemp;
```

### Core Functions

#### `arena_create(size_t size)`
Creates a new arena with the specified capacity.

```c
Arena arena = arena_create(1024 * 1024);  // 1MB arena
if (arena.base == NULL) {
    // Handle allocation failure
}
```

#### `arena_destroy(Arena *arena)`
Frees the arena's backing memory and zeroes the struct.

```c
arena_destroy(&arena);
// arena.base is now NULL, arena.size and arena.pos are 0
```

#### `arena_clear(Arena *arena)`
Resets the arena position to 0 without freeing memory. All previous allocations become invalid.

```c
arena_clear(&arena);  // arena.pos = 0
// Arena can be reused, memory is still allocated
```

#### `arena_alloc(Arena *arena, size_t size)`
Allocates `size` bytes with default alignment (pointer-sized).

```c
void *data = arena_alloc(&arena, 256);
if (data == NULL) {
    // Arena is out of memory
}
```

#### `arena_alloc_aligned(Arena *arena, size_t size, size_t alignment)`
Allocates `size` bytes with specified alignment.

```c
// Allocate 64 bytes with 16-byte alignment (for SIMD)
float *simd_data = arena_alloc_aligned(&arena, 64, 16);
```

**Alignment formula:**
```c
aligned_pos = (pos + alignment - 1) & ~(alignment - 1);
```

### Temporary Scope Functions

#### `arena_temp_begin(Arena *arena)`
Saves the current position for later restoration.

```c
ArenaTemp temp = arena_temp_begin(&arena);
// temp.pos now holds arena's current position
```

#### `arena_temp_end(ArenaTemp temp)`
Restores the arena to the saved position, effectively freeing all allocations made since `arena_temp_begin`.

```c
arena_temp_end(temp);
// Arena position restored - all allocations in scope are "freed"
```

### Global Scratch Arena Functions

#### `arena_scratch_init(size_t size)`
Initializes the global scratch arena. Call once at program startup.

```c
arena_scratch_init(256 * 1024);  // 256KB scratch space
```

#### `arena_scratch_shutdown(void)`
Destroys the global scratch arena. Call at program shutdown.

```c
arena_scratch_shutdown();
```

#### `arena_scratch_begin(void)`
Begins a temporary scope on the global scratch arena.

```c
ArenaTemp temp = arena_scratch_begin();
// ... temporary allocations ...
arena_temp_end(temp);
```

### Helper Macros

#### `ARENA_PUSH_STRUCT(arena, type)`
Allocates a single struct with proper alignment.

```c
Mesh *mesh = ARENA_PUSH_STRUCT(&arena, Mesh);
VkDeviceContext *ctx = ARENA_PUSH_STRUCT(&arena, VkDeviceContext);
```

**Expands to:**
```c
((type *)arena_alloc_aligned((arena), sizeof(type), _Alignof(type)))
```

#### `ARENA_PUSH_ARRAY(arena, type, count)`
Allocates an array of structs with proper alignment.

```c
Vertex *vertices = ARENA_PUSH_ARRAY(&arena, Vertex, 1000);
u32 *indices = ARENA_PUSH_ARRAY(&arena, u32, 3000);
```

**Expands to:**
```c
((type *)arena_alloc_aligned((arena), sizeof(type) * (count), _Alignof(type)))
```

## Usage Patterns

### Pattern 1: Permanent Allocations

For data that lives for the entire application:

```c
Result renderer_init(RendererContext *ctx, AppContext *app) {
    // Allocate mesh that persists for app lifetime
    ctx->terrain_mesh = ARENA_PUSH_STRUCT(&app->permanent_arena, Mesh);

    ctx->vertices = ARENA_PUSH_ARRAY(&app->permanent_arena, Vertex, MAX_VERTICES);
    ctx->indices = ARENA_PUSH_ARRAY(&app->permanent_arena, u32, MAX_INDICES);

    return RESULT_SUCCESS;
}
```

### Pattern 2: Temporary Allocations with Auto-Cleanup

For short-lived data like file loading:

```c
Result load_shader(const char *path, u32 **code, size_t *size) {
    ArenaTemp temp = arena_scratch_begin();

    // Read file into temporary buffer
    u8 *data = file_read_binary_arena(path, size, temp.arena);
    if (!data) {
        arena_temp_end(temp);
        return RESULT_ERROR_FILE_NOT_FOUND;
    }

    // Process data...
    *code = process_shader_code(data, *size);

    arena_temp_end(temp);  // Temporary memory freed
    return RESULT_SUCCESS;
}
```

### Pattern 3: Nested Temporary Scopes

Temporary scopes can nest:

```c
void process_frame(void) {
    ArenaTemp outer = arena_scratch_begin();

    u8 *frame_data = arena_alloc(outer.arena, 1024);

    for (int i = 0; i < 10; i++) {
        ArenaTemp inner = arena_scratch_begin();

        // Per-iteration temporary data
        u8 *iter_data = arena_alloc(inner.arena, 256);
        process_iteration(iter_data);

        arena_temp_end(inner);  // Per-iteration cleanup
    }

    arena_temp_end(outer);  // Full frame cleanup
}
```

### Pattern 4: File Loading

Common pattern for loading and processing files:

```c
Mesh *load_mesh_from_file(const char *path, Arena *dest_arena) {
    ArenaTemp temp = arena_scratch_begin();

    size_t size;
    u8 *raw_data = file_read_binary_arena(path, &size, temp.arena);
    if (!raw_data) {
        arena_temp_end(temp);
        return NULL;
    }

    // Parse into permanent storage
    Mesh *mesh = ARENA_PUSH_STRUCT(dest_arena, Mesh);
    parse_mesh_data(raw_data, size, mesh, dest_arena);

    arena_temp_end(temp);  // Raw data freed, parsed mesh persists
    return mesh;
}
```

## Common Pitfalls

### 1. Returning Pointers to Scratch Memory

**WRONG:**
```c
char *get_formatted_string(int value) {
    ArenaTemp temp = arena_scratch_begin();
    char *str = arena_alloc(temp.arena, 64);
    snprintf(str, 64, "Value: %d", value);
    arena_temp_end(temp);  // BUG: str is now invalid!
    return str;            // Dangling pointer!
}
```

**CORRECT:**
```c
char *get_formatted_string(int value, Arena *dest) {
    char *str = arena_alloc(dest, 64);
    snprintf(str, 64, "Value: %d", value);
    return str;  // Caller owns the memory
}
```

### 2. Forgetting `arena_temp_end()`

**WRONG:**
```c
void process_data(void) {
    ArenaTemp temp = arena_scratch_begin();
    u8 *data = arena_alloc(temp.arena, 1024);

    if (error_condition) {
        return;  // BUG: temp never ended, scratch arena keeps growing!
    }

    arena_temp_end(temp);
}
```

**CORRECT:**
```c
void process_data(void) {
    ArenaTemp temp = arena_scratch_begin();
    u8 *data = arena_alloc(temp.arena, 1024);

    if (error_condition) {
        arena_temp_end(temp);  // Always clean up
        return;
    }

    arena_temp_end(temp);
}
```

### 3. Not Checking for NULL Returns

```c
void *ptr = arena_alloc(&arena, huge_size);
// If arena is out of space, ptr is NULL
memcpy(ptr, src, huge_size);  // Crash if ptr is NULL!
```

**CORRECT:**
```c
void *ptr = arena_alloc(&arena, huge_size);
if (!ptr) {
    LOG_ERROR("Arena out of memory");
    return RESULT_ERROR_OUT_OF_MEMORY;
}
memcpy(ptr, src, huge_size);
```

### 4. Using After `arena_destroy()`

```c
Arena arena = arena_create(1024);
Mesh *mesh = ARENA_PUSH_STRUCT(&arena, Mesh);
arena_destroy(&arena);

mesh->vertex_count = 100;  // BUG: Use after free!
```

### 5. Mismatched Temp Scopes

```c
ArenaTemp temp1 = arena_scratch_begin();
ArenaTemp temp2 = arena_scratch_begin();
arena_temp_end(temp1);  // BUG: Ended outer scope, temp2's allocations lost!
arena_temp_end(temp2);  // This does nothing useful now
```

**Always end in reverse order (LIFO):**
```c
ArenaTemp temp1 = arena_scratch_begin();
ArenaTemp temp2 = arena_scratch_begin();
arena_temp_end(temp2);  // Inner first
arena_temp_end(temp1);  // Outer last
```

## Sizing Guidelines

### Estimating Requirements

| Data Type | Example Size |
|-----------|--------------|
| `Vertex` (pos + color) | 24 bytes |
| `u32` index | 4 bytes |
| 1000x1000 heightmap (f32) | 4 MB |
| 1M vertices | 24 MB |
| 3M indices | 12 MB |

### Recommended Sizes for Terrain Simulation

| Arena | Size | Purpose |
|-------|------|---------|
| **Permanent** | 64+ MB | Terrain mesh, simulation state |
| **Scratch** | 4+ MB | File loading, per-frame buffers, simulation temp data |

### Monitoring Usage

Add a helper to check arena usage:

```c
void arena_print_usage(Arena *arena, const char *name) {
    f32 used_mb = arena->pos / (1024.0f * 1024.0f);
    f32 total_mb = arena->size / (1024.0f * 1024.0f);
    f32 percent = (arena->pos * 100.0f) / arena->size;
    LOG_INFO("%s: %.2f / %.2f MB (%.1f%%)", name, used_mb, total_mb, percent);
}
```

### Sizing Strategy

1. Start with generous estimates
2. Log peak usage during development
3. Add 50% headroom for safety
4. Consider worst-case scenarios (maximum terrain size, largest files)

## When NOT to Use Arenas

Arenas are not suitable for every situation:

### 1. Varied Lifetimes

When objects need to be freed at different times:

```c
// BAD: Can't free individual enemies when they die
Enemy *enemies[100];
for (int i = 0; i < 100; i++) {
    enemies[i] = ARENA_PUSH_STRUCT(&arena, Enemy);
}
// Can't free enemies[5] when it dies without freeing all
```

**Solution:** Use a pool allocator or traditional malloc for objects with varied lifetimes.

### 2. Dynamic Data Structures with Deletions

Linked lists, trees, or hash tables that remove elements:

```c
// BAD: Removed nodes waste arena space forever
typedef struct Node {
    struct Node *next;
    int value;
} Node;

void remove_node(Node *prev, Node *node) {
    prev->next = node->next;
    // Can't reclaim node's memory in arena
}
```

**Solution:** Use malloc/free, or use arenas only for append-only structures.

### 3. Long-Running Servers

Applications that run indefinitely and accumulate allocations:

```c
// BAD: Arena grows forever
while (server_running) {
    Request *req = ARENA_PUSH_STRUCT(&arena, Request);
    handle_request(req);
    // Memory never freed
}
```

**Solution:** Use per-request arenas that get cleared after each request, or use traditional allocation.

### 4. Interfacing with External Libraries

Libraries that expect to free memory themselves:

```c
// BAD: Library will call free() on arena memory
char *str = arena_alloc(&arena, 256);
some_library_function(str);  // Library might free(str) - crash!
```

**Solution:** Use malloc for data passed to external libraries that take ownership.

## Quick Reference

```c
// Create/destroy
Arena arena = arena_create(size);
arena_destroy(&arena);
arena_clear(&arena);

// Allocate
void *ptr = arena_alloc(&arena, bytes);
void *aligned = arena_alloc_aligned(&arena, bytes, alignment);
Type *t = ARENA_PUSH_STRUCT(&arena, Type);
Type *arr = ARENA_PUSH_ARRAY(&arena, Type, count);

// Temporary scope
ArenaTemp temp = arena_temp_begin(&arena);
// ... allocations ...
arena_temp_end(temp);

// Global scratch
arena_scratch_init(size);
ArenaTemp temp = arena_scratch_begin();
// ... temporary allocations on temp.arena ...
arena_temp_end(temp);
arena_scratch_shutdown();
```

#ifndef RENDERER_H
#define RENDERER_H

#include "foundation/result.h"
#include "memory/arena.h"
#include "platform/window.h"

typedef struct Renderer Renderer;
typedef u32 MeshHandle;
struct Camera;
struct Entity;
struct Mesh;

typedef struct RendererConfig {
  const char *app_name;
  bool        enable_validation;
} RendererConfig;

Result renderer_create(Renderer **out_renderer, WindowContext *window, const RendererConfig *config, Arena *arena);
void   renderer_destroy(Renderer *renderer);

Result renderer_upload_mesh(Renderer *renderer, const struct Mesh *mesh, MeshHandle *out_handle);
Result renderer_draw(Renderer *renderer, const Camera *camera, const struct Entity *entities, u32 entity_count);
Result renderer_resize(Renderer *renderer);
void   renderer_wait_idle(Renderer *renderer);

#endif // RENDERER_H

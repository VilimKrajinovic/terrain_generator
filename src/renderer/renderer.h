#ifndef RENDERER_H
#define RENDERER_H

#include "foundation/result.h"
#include "memory/arena.h"
#include "platform/window.h"

typedef struct Renderer Renderer;

typedef struct RendererConfig {
  const char *app_name;
  bool enable_validation;
} RendererConfig;

Result renderer_create(Renderer **out_renderer, WindowContext *window,
                       const RendererConfig *config, Arena *arena);
void renderer_destroy(Renderer *renderer);

Result renderer_draw(Renderer *renderer);
Result renderer_resize(Renderer *renderer);
void renderer_wait_idle(Renderer *renderer);

#endif // RENDERER_H

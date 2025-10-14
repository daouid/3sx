#include "port/sdl/sdl_game_renderer.h"
#include "common.h"
#include "sf33rd/AcrSDK/ps2/flps2etc.h"
#include "sf33rd/AcrSDK/ps2/flps2render.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"

#include <libgraph.h>

#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>

#define RENDER_TASK_MAX 1024
#define TEXTURES_TO_DESTROY_MAX 1024

typedef struct RenderTask {
    SDL_Texture* texture;
    SDL_Vertex vertices[4];
    float z;
    int original_index;  // Preserves submission order for stable sorting
} RenderTask;

SDL_Texture* cps3_canvas = NULL;

static const int cps3_width = 384;
static const int cps3_height = 224;

static SDL_Renderer* _renderer = NULL;
static SDL_Surface* surfaces[FL_TEXTURE_MAX] = { NULL };
static SDL_Palette* palettes[FL_PALETTE_MAX] = { NULL };
static SDL_Texture* textures[FL_PALETTE_MAX] = { NULL };
static int texture_count = 0;
static SDL_Texture* texture_cache[FL_TEXTURE_MAX][FL_PALETTE_MAX + 1] = { { NULL } };
static SDL_Texture* textures_to_destroy[TEXTURES_TO_DESTROY_MAX] = { NULL };
static int textures_to_destroy_count = 0;
static RenderTask render_tasks[RENDER_TASK_MAX] = { 0 };
static int render_task_count = 0;

// Pre-allocated batch buffers for optimized rendering
static SDL_Vertex batch_vertices[RENDER_TASK_MAX * 4];
static int batch_indices[RENDER_TASK_MAX * 6];
static bool batch_buffers_initialized = false;

// Debugging and statistics
static bool draw_rect_borders = false;
static bool dump_textures = false;
static int texture_index = 0;

// --- PlayStation 2 Graphics Synthesizer CLUT index shuffle ---
// The PS2 GS stores 256-color CLUTs (Color Look-Up Tables) in a non-linear memory order
// to optimize texture cache performance. This macro converts from linear index to PS2 GS order.
// Specifically, it swaps bits 3 and 4 of the index to match the hardware's block-swizzled layout.
#define clut_shuf(x) (((x) & ~0x18) | ((((x) & 0x08) << 1) | (((x) & 0x10) >> 1)))

// --- Color Reading Functions ---

static void read_rgba32_color(Uint32 pixel, SDL_Color* color) {
    color->b = pixel & 0xFF;
    color->g = (pixel >> 8) & 0xFF;
    color->r = (pixel >> 16) & 0xFF;
    color->a = (pixel >> 24) & 0xFF;
}

static void read_rgba32_fcolor(Uint32 pixel, SDL_FColor* fcolor) {
    SDL_Color color;
    read_rgba32_color(pixel, &color);
    fcolor->r = (float)color.r / 255.0f;
    fcolor->g = (float)color.g / 255.0f;
    fcolor->b = (float)color.b / 255.0f;
    fcolor->a = (float)color.a / 255.0f;
}

static void read_rgba16_color(Uint16 pixel, SDL_Color* color) {
    color->r = (pixel & 0x1F) * 255 / 31;
    color->g = ((pixel >> 5) & 0x1F) * 255 / 31;
    color->b = ((pixel >> 10) & 0x1F) * 255 / 31;
    color->a = (pixel & 0x8000) ? 255 : 0;
}

static void read_color(const void* pixels, int index, size_t color_size, SDL_Color* color) {
    switch (color_size) {
    case 2: {
        const Uint16* rgba16_colors = (const Uint16*)pixels;
        read_rgba16_color(rgba16_colors[index], color);
        break;
    }
    case 4: {
        const Uint32* rgba32_colors = (const Uint32*)pixels;
        read_rgba32_color(rgba32_colors[index], color);
        break;
    }
    }
}

#define LERP_FLOAT(a, b, x) ((a) * (1.0f - (x)) + (b) * (x))

static void lerp_fcolors(SDL_FColor* dest, const SDL_FColor* a, const SDL_FColor* b, float x) {
    dest->r = LERP_FLOAT(a->r, b->r, x);
    dest->g = LERP_FLOAT(a->g, b->g, x);
    dest->b = LERP_FLOAT(a->b, b->b, x);
    dest->a = LERP_FLOAT(a->a, b->a, x);
}

// --- Texture Debugging ---

static void save_texture(const SDL_Surface* surface, const SDL_Palette* palette) {
    if (surface == NULL || palette == NULL) {
        SDL_Log("Cannot save texture: NULL surface or palette");
        return;
    }

    char filename[128];
    snprintf(filename, sizeof(filename), "textures/%d.tga", texture_index);

    const Uint8* pixels = (const Uint8*)surface->pixels;
    const int width = surface->w;
    const int height = surface->h;

    FILE* f = fopen(filename, "wb");
    if (!f) {
        SDL_Log("Failed to open file for writing: %s", filename);
        return;
    }

    uint8_t header[18] = { 0 };
    header[2] = 2; // uncompressed RGB
    header[12] = width & 0xFF;
    header[13] = (width >> 8) & 0xFF;
    header[14] = height & 0xFF;
    header[15] = (height >> 8) & 0xFF;
    header[16] = 32;   // bits per pixel
    header[17] = 0x20; // top-left origin

    fwrite(header, 1, 18, f);

    // Write pixels in BGRA format
    const int pixel_count = width * height;
    for (int i = 0; i < pixel_count; ++i) {
        Uint8 index;

        // Extract 4-bit or 8-bit index from packed pixel data
        if (palette->ncolors == 16) {
            const Uint8 byte = pixels[i / 2];
            if (i & 1) {
                index = byte >> 4;      // High nibble (second pixel in byte)
            } else {
                index = byte & 0x0F;    // Low nibble (first pixel in byte)
            }
        } else { // 256 colors
            index = pixels[i];
        }

        const SDL_Color* color = &palette->colors[index];
        const Uint8 bgr[] = { color->b, color->g, color->r, color->a };
        fwrite(bgr, 1, 4, f);
    }

    fclose(f);
    texture_index = (texture_index + 1) % 10000;
}

// --- Texture Stack Management ---

static void push_texture(SDL_Texture* texture) {
    if (texture_count >= FL_PALETTE_MAX) {
        fatal_error("Texture stack overflow in push_texture");
    }
    textures[texture_count] = texture;
    texture_count += 1;
}

static SDL_Texture* get_texture(void) {
    if (texture_count == 0) {
        fatal_error("No textures to get");
    }
    return textures[texture_count - 1];
}

// --- Deferred Texture Destruction ---

static void push_texture_to_destroy(SDL_Texture* texture) {
    if (textures_to_destroy_count >= TEXTURES_TO_DESTROY_MAX) {
        SDL_Log("Warning: textures_to_destroy buffer full, destroying texture immediately");
        SDL_DestroyTexture(texture);
        return;
    }
    textures_to_destroy[textures_to_destroy_count] = texture;
    textures_to_destroy_count += 1;
}

static void destroy_textures(void) {
    for (int i = 0; i < texture_count; i++) {
        textures[i] = NULL;
    }
    texture_count = 0;

    for (int i = 0; i < textures_to_destroy_count; i++) {
        SDL_DestroyTexture(textures_to_destroy[i]);
        textures_to_destroy[i] = NULL;
    }
    textures_to_destroy_count = 0;
}

// --- Render Task Management ---

static void push_render_task(const RenderTask* task) {
    if (render_task_count >= RENDER_TASK_MAX) {
        SDL_Log("Warning: render task buffer full, skipping task");
        return;
    }
    render_tasks[render_task_count] = *task;
    render_tasks[render_task_count].original_index = render_task_count;
    render_task_count += 1;
}

static void clear_render_tasks(void) {
    SDL_zeroa(render_tasks);
    render_task_count = 0;
}

// --- Render Task Sorting ---
// CRITICAL: Sort by z-depth first, then original submission order.
// This maintains correct layering and prevents z-fighting while allowing
// the batching system to optimize consecutive same-texture draws.
static int compare_render_tasks(const void* a, const void* b) {
    const RenderTask* task_a = (const RenderTask*)a;
    const RenderTask* task_b = (const RenderTask*)b;

    // Primary sort: z-depth (rendering order)
    if (task_a->z < task_b->z) {
        return -1;
    } else if (task_a->z > task_b->z) {
        return 1;
    }

    // Secondary sort: original submission order (prevents z-fighting)
    // Reverse order preserves FIFO submission behavior
    if (task_a->original_index > task_b->original_index) {
        return -1;
    } else if (task_a->original_index < task_b->original_index) {
        return 1;
    }

    return 0;
}

// --- Lifecycle ---

void SDLGameRenderer_Init(SDL_Renderer* renderer) {
    _renderer = renderer;
    cps3_canvas =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, cps3_width, cps3_height);

    if (!cps3_canvas) {
        fatal_error("Failed to create cps3_canvas texture: %s", SDL_GetError());
    }
    SDL_SetTextureScaleMode(cps3_canvas, SDL_SCALEMODE_NEAREST);

    // Pre-initialize index buffer (constant for all quads: two triangles per quad)
    if (!batch_buffers_initialized) {
        for (int i = 0; i < RENDER_TASK_MAX; i++) {
            const int idx_offset = i * 6;
            const int vert_offset = i * 4;
            batch_indices[idx_offset + 0] = vert_offset + 0;
            batch_indices[idx_offset + 1] = vert_offset + 1;
            batch_indices[idx_offset + 2] = vert_offset + 2;
            batch_indices[idx_offset + 3] = vert_offset + 1;
            batch_indices[idx_offset + 4] = vert_offset + 2;
            batch_indices[idx_offset + 5] = vert_offset + 3;
        }
        batch_buffers_initialized = true;
    }
}

void SDLGameRenderer_BeginFrame(void) {
    const Uint8 r = (flPs2State.FrameClearColor >> 16) & 0xFF;
    const Uint8 g = (flPs2State.FrameClearColor >> 8) & 0xFF;
    const Uint8 b = flPs2State.FrameClearColor & 0xFF;
    const Uint8 a = flPs2State.FrameClearColor >> 24;

    if (a != SDL_ALPHA_TRANSPARENT) {
        SDL_SetRenderDrawColor(_renderer, r, g, b, a);
    } else {
        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    }

    SDL_SetRenderTarget(_renderer, cps3_canvas);
    SDL_RenderClear(_renderer);
}

void SDLGameRenderer_RenderFrame(void) {
    SDL_SetRenderTarget(_renderer, cps3_canvas);

    if (render_task_count == 0) {
        return;
    }

    // Sort tasks by z-depth, then original submission order
    qsort(render_tasks, render_task_count, sizeof(RenderTask), compare_render_tasks);

    // Batch rendering: group consecutive tasks with same texture
    // Note: Sorting is stable by design, so batching only merges naturally adjacent same-texture draws
    int batch_start = 0;
    SDL_Texture* current_texture = render_tasks[0].texture;

    for (int i = 0; i <= render_task_count; i++) {
        const bool should_flush = (i == render_task_count) ||
                                 (render_tasks[i].texture != current_texture);

        if (should_flush) {
            const int batch_size = i - batch_start;
            if (batch_size > 0) {
                SDL_assert(batch_size <= RENDER_TASK_MAX);

                // Copy vertices to batch buffer
                for (int j = 0; j < batch_size; j++) {
                    const int task_idx = batch_start + j;
                    const int vert_offset = j * 4;
                    memcpy(&batch_vertices[vert_offset],
                           render_tasks[task_idx].vertices,
                           4 * sizeof(SDL_Vertex));
                }

                // Single draw call for entire batch
                SDL_RenderGeometry(_renderer, current_texture,
                                  batch_vertices, batch_size * 4,
                                  batch_indices, batch_size * 6);
            }

            // Update batch parameters for next group
            if (i < render_task_count) {
                current_texture = render_tasks[i].texture;
                batch_start = i;
            }
        }
    }

    // Debug visualization: draw colored borders around quads
    if (draw_rect_borders) {
        const SDL_FColor red = { .r = 1.0f, .g = 0.0f, .b = 0.0f, .a = SDL_ALPHA_OPAQUE_FLOAT };
        const SDL_FColor green = { .r = 0.0f, .g = 1.0f, .b = 0.0f, .a = SDL_ALPHA_OPAQUE_FLOAT };
        SDL_FColor border_color;

        for (int i = 0; i < render_task_count; i++) {
            const RenderTask* task = &render_tasks[i];
            const float x0 = task->vertices[0].position.x;
            const float y0 = task->vertices[0].position.y;
            const float x1 = task->vertices[3].position.x;
            const float y1 = task->vertices[3].position.y;
            const SDL_FRect border_rect = { .x = x0, .y = y0, .w = (x1 - x0), .h = (y1 - y0) };

            // Interpolate color from red (first) to green (last) based on render order
            const float lerp_factor = (render_task_count > 1)
                ? (float)i / (float)(render_task_count - 1)
                : 0.5f;
            lerp_fcolors(&border_color, &red, &green, lerp_factor);

            SDL_SetRenderDrawColorFloat(_renderer, border_color.r, border_color.g, border_color.b, border_color.a);
            SDL_RenderRect(_renderer, &border_rect);
        }
    }
}

void SDLGameRenderer_EndFrame(void) {
    destroy_textures();
    clear_render_tasks();
}

// --- Texture and Palette Unlocking ---

void SDLGameRenderer_UnlockPalette(unsigned int ph) {
    const int palette_handle = ph;
    if ((palette_handle > 0) && (palette_handle < FL_PALETTE_MAX)) {
        // Always recreate to match original behavior
        SDLGameRenderer_DestroyPalette(palette_handle);
        SDLGameRenderer_CreatePalette(ph << 16);
    }
}

void SDLGameRenderer_UnlockTexture(unsigned int th) {
    const int texture_handle = th;
    if ((texture_handle > 0) && (texture_handle < FL_TEXTURE_MAX)) {
        // Always recreate to match original behavior
        SDLGameRenderer_DestroyTexture(texture_handle);
        SDLGameRenderer_CreateTexture(th);
    }
}

// --- Texture Creation and Destruction ---

void SDLGameRenderer_CreateTexture(unsigned int th) {
    const int texture_index = LO_16_BITS(th) - 1;
    if (texture_index < 0 || texture_index >= FL_TEXTURE_MAX) {
        fatal_error("Texture index out of bounds in CreateTexture: %d", texture_index + 1);
    }

    const FLTexture* fl_texture = &flTexture[texture_index];
    const void* pixels = flPS2GetSystemBuffAdrs(fl_texture->mem_handle);
    SDL_PixelFormat pixel_format = SDL_PIXELFORMAT_UNKNOWN;
    int pitch = 0;

    if (surfaces[texture_index] != NULL) {
        fatal_error("Overwriting an existing texture at index %d", texture_index);
    }

    switch (fl_texture->format) {
    case SCE_GS_PSMT8:
        pixel_format = SDL_PIXELFORMAT_INDEX8;
        pitch = fl_texture->width;
        break;
    case SCE_GS_PSMT4:
        pixel_format = SDL_PIXELFORMAT_INDEX4LSB;
        pitch = (fl_texture->width + 1) / 2;  // Round up for odd widths
        break;
    case SCE_GS_PSMCT16:
        pixel_format = SDL_PIXELFORMAT_ABGR1555;
        pitch = fl_texture->width * 2;
        break;
    default:
        fatal_error("Unhandled pixel format: %d", fl_texture->format);
        break;
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(fl_texture->width, fl_texture->height, pixel_format, (void*)pixels, pitch);
    if (!surface) {
        fatal_error("Failed to create surface from memory: %s", SDL_GetError());
    }
    surfaces[texture_index] = surface;
}

void SDLGameRenderer_DestroyTexture(unsigned int texture_handle) {
    const int texture_index = texture_handle - 1;
    if (texture_index < 0 || texture_index >= FL_TEXTURE_MAX) {
        SDL_Log("Warning: Attempted to destroy invalid texture handle: %u", texture_handle);
        return;
    }

    // Destroy all cached texture+palette combinations
    for (int i = 0; i < FL_PALETTE_MAX + 1; i++) {
        SDL_Texture** texture_p = &texture_cache[texture_index][i];
        if (*texture_p != NULL) {
            push_texture_to_destroy(*texture_p);
            *texture_p = NULL;
        }
    }

    if (surfaces[texture_index] != NULL) {
        SDL_DestroySurface(surfaces[texture_index]);
        surfaces[texture_index] = NULL;
    }
}

// --- Palette Creation and Destruction ---

void SDLGameRenderer_CreatePalette(unsigned int ph) {
    const int palette_index = HI_16_BITS(ph) - 1;
    if (palette_index < 0 || palette_index >= FL_PALETTE_MAX) {
        fatal_error("Palette index out of bounds in CreatePalette: %d", palette_index + 1);
    }

    const FLTexture* fl_palette = &flPalette[palette_index];
    const void* pixels = flPS2GetSystemBuffAdrs(fl_palette->mem_handle);
    const int color_count = fl_palette->width * fl_palette->height;
    SDL_Color colors[256];
    size_t color_size = 0;

    if (palettes[palette_index] != NULL) {
        fatal_error("Overwriting an existing palette at index %d", palette_index);
    }

    switch (fl_palette->format) {
    case SCE_GS_PSMCT32:
        color_size = 4;
        break;
    case SCE_GS_PSMCT16:
        color_size = 2;
        break;
    default:
        fatal_error("Unhandled palette pixel format: %d", fl_palette->format);
        break;
    }

    switch (color_count) {
    case 16:
        for (int i = 0; i < 16; i++) {
            read_color(pixels, i, color_size, &colors[i]);
        }
        break;
    case 256:
        // Apply PS2 GS CLUT shuffle for 256-color palettes
        for (int i = 0; i < 256; i++) {
            const int color_index = clut_shuf(i);
            read_color(pixels, color_index, color_size, &colors[i]);
        }
        break;
    default:
        fatal_error("Unhandled palette dimensions: %dx%d", fl_palette->width, fl_palette->height);
        break;
    }

    SDL_Palette* palette = SDL_CreatePalette(color_count);
    if (!palette) {
        fatal_error("Failed to create SDL palette: %s", SDL_GetError());
    }
    SDL_SetPaletteColors(palette, colors, 0, color_count);
    palettes[palette_index] = palette;
}

void SDLGameRenderer_DestroyPalette(unsigned int palette_handle) {
    const int palette_index = palette_handle - 1;
    if (palette_index < 0 || palette_index >= FL_PALETTE_MAX) {
        SDL_Log("Warning: Attempted to destroy invalid palette handle: %u", palette_handle);
        return;
    }

    // Destroy all cached texture+palette combinations using this palette
    for (int i = 0; i < FL_TEXTURE_MAX; i++) {
        SDL_Texture** texture_p = &texture_cache[i][palette_handle];
        if (*texture_p != NULL) {
            push_texture_to_destroy(*texture_p);
            *texture_p = NULL;
        }
    }

    if (palettes[palette_index] != NULL) {
        SDL_DestroyPalette(palettes[palette_index]);
        palettes[palette_index] = NULL;
    }
}

// --- Texture Binding ---

void SDLGameRenderer_SetTexture(unsigned int th) {
    const int texture_handle = LO_16_BITS(th);
    const int palette_handle = HI_16_BITS(th);

    if (texture_handle < 1 || texture_handle > FL_TEXTURE_MAX) {
        fatal_error("Invalid texture handle in SetTexture: %d", texture_handle);
    }

    if (palette_handle > FL_PALETTE_MAX) {
        fatal_error("Invalid palette handle in SetTexture: %d", palette_handle);
    }

    SDL_Surface* surface = surfaces[texture_handle - 1];
    if (!surface) {
        fatal_error("Surface is NULL for texture handle: %d", texture_handle);
    }

    const SDL_Palette* palette = (palette_handle != 0) ? palettes[palette_handle - 1] : NULL;

    if (dump_textures && palette != NULL) {
        save_texture(surface, palette);
    }

    // Apply palette to surface if present
    // Note: SDL_SetSurfacePalette modifies the surface's palette pointer, not pixel data
    if (palette != NULL) {
        SDL_SetSurfacePalette(surface, palette);
    }

    // Check cache for existing texture+palette combination
    SDL_Texture* texture = texture_cache[texture_handle - 1][palette_handle];

    if (texture == NULL) {
        // Create new texture from surface
        texture = SDL_CreateTextureFromSurface(_renderer, surface);
        if (!texture) {
            fatal_error("Failed to create texture from surface: %s", SDL_GetError());
        }
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
        texture_cache[texture_handle - 1][palette_handle] = texture;
    }

    push_texture(texture);
}

// --- Geometry Rendering ---

static void draw_quad(const SDLGameRenderer_Vertex* vertices, bool textured) {
    RenderTask task;
    task.texture = textured ? get_texture() : NULL;
    task.z = flPS2ConvScreenFZ(vertices[0].coord.z);
    task.original_index = 0;  // Set by push_render_task

    SDL_zeroa(task.vertices);

    for (int i = 0; i < 4; i++) {
        task.vertices[i].position.x = vertices[i].coord.x;
        task.vertices[i].position.y = vertices[i].coord.y;

        if (textured) {
            task.vertices[i].tex_coord.x = vertices[i].tex_coord.s;
            task.vertices[i].tex_coord.y = vertices[i].tex_coord.t;
        }

        read_rgba32_fcolor(vertices[i].color, &task.vertices[i].color);
    }

    push_render_task(&task);
}

void SDLGameRenderer_DrawTexturedQuad(const SDLGameRenderer_Vertex* vertices) {
    draw_quad(vertices, true);
}

void SDLGameRenderer_DrawSolidQuad(const SDLGameRenderer_Vertex* vertices) {
    draw_quad(vertices, false);
}

void SDLGameRenderer_DrawSprite(const SDLGameRenderer_Sprite* sprite, unsigned int color) {
    SDLGameRenderer_Vertex vertices[4];
    SDL_zeroa(vertices);

    for (int i = 0; i < 4; i++) {
        vertices[i].coord.z = sprite->v[0].z;
        vertices[i].color = color;
    }

    vertices[0].coord.x = sprite->v[0].x;
    vertices[0].coord.y = sprite->v[0].y;
    vertices[3].coord.x = sprite->v[3].x;
    vertices[3].coord.y = sprite->v[3].y;
    vertices[1].coord.x = vertices[3].coord.x;
    vertices[1].coord.y = vertices[0].coord.y;
    vertices[2].coord.x = vertices[0].coord.x;
    vertices[2].coord.y = vertices[3].coord.y;

    vertices[0].tex_coord = sprite->t[0];
    vertices[3].tex_coord = sprite->t[3];
    vertices[1].tex_coord.s = vertices[3].tex_coord.s;
    vertices[1].tex_coord.t = vertices[0].tex_coord.t;
    vertices[2].tex_coord.s = vertices[0].tex_coord.s;
    vertices[2].tex_coord.t = vertices[3].tex_coord.t;

    draw_quad(vertices, true);
}

void SDLGameRenderer_DrawSprite2(const SDLGameRenderer_Sprite2* sprite2) {
    SDLGameRenderer_Sprite sprite;
    SDL_zero(sprite);

    sprite.v[0] = sprite2->v[0];
    sprite.v[1].x = sprite2->v[1].x;
    sprite.v[1].y = sprite2->v[0].y;
    sprite.v[2].x = sprite2->v[0].x;
    sprite.v[2].y = sprite2->v[1].y;
    sprite.v[3] = sprite2->v[1];

    sprite.t[0] = sprite2->t[0];
    sprite.t[1].s = sprite2->t[1].s;
    sprite.t[1].t = sprite2->t[0].t;
    sprite.t[2].s = sprite2->t[0].s;
    sprite.t[2].t = sprite2->t[1].t;
    sprite.t[3] = sprite2->t[1];

    for (int i = 0; i < 4; i++) {
        sprite.v[i].z = sprite2->v[0].z;
    }

    SDLGameRenderer_DrawSprite(&sprite, sprite2->vertex_color);
}
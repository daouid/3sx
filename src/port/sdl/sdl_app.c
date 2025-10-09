#include "port/sdl/sdl_app.h"
#include "common.h"
#include "port/float_clamp.h"
#include "port/sdk_threads.h"
#include "port/sdl/sdl_adx_sound.h"
#include "port/sdl/sdl_game_renderer.h"
#include "port/sdl/sdl_message_renderer.h"
#include "port/sdl/sdl_pad.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"
#include "sf33rd/Source/Game/main.h"

#include <SDL3/SDL.h>

#define FRAME_END_TIMES_MAX 30

int ADXPS2_ExecVint(int mode);

static const char* app_name = "Street Fighter III: 3rd Strike";
static const float display_target_ratio = 4.0 / 3.0;
static const int window_default_width = 640;
static const int window_default_height = (int)(window_default_width / display_target_ratio);
static const double target_fps = 59.59949;
static const Uint64 target_frame_time_ns = 1000000000.0 / target_fps;

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* screen_texture = NULL;

static Uint64 frame_deadline = 0;
static Uint64 frame_end_times[FRAME_END_TIMES_MAX];
static int frame_end_times_index = 0;
static bool frame_end_times_filled = false;
static double fps = 0;
static Uint64 frame_counter = 0;

static bool should_save_screenshot = false;
static Uint64 last_mouse_motion_time = 0;
static const int mouse_hide_delay_ms = 2000;

// Cached letterbox rectangle
static SDL_FRect cached_letterbox_rect;
static bool letterbox_rect_valid = false;

// Asynchronous Screenshot System
typedef struct {
    SDL_Surface* surface;
    char filename[64];
} ScreenshotJob;

static SDL_Thread* screenshot_thread = NULL;
static SDL_Mutex* screenshot_mutex = NULL;
static SDL_Condition* screenshot_cond = NULL;
static ScreenshotJob* pending_screenshot_job = NULL;
static volatile bool screenshot_thread_running = true;

static int ScreenshotSaverThread(void* data) {
    (void)data;
    while (screenshot_thread_running) {
        SDL_LockMutex(screenshot_mutex);

        while (!pending_screenshot_job && screenshot_thread_running) {
            SDL_WaitCondition(screenshot_cond, screenshot_mutex);
        }

        if (!screenshot_thread_running) {
            SDL_UnlockMutex(screenshot_mutex);
            break;
        }

        ScreenshotJob* job = pending_screenshot_job;
        pending_screenshot_job = NULL;

        SDL_UnlockMutex(screenshot_mutex);

        if (job && job->surface) {
            if (!SDL_SaveBMP(job->surface, job->filename)) {
                SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to save screenshot: %s", SDL_GetError());
            } else {
                SDL_Log("Saved screenshot: %s", job->filename);
            }
            SDL_DestroySurface(job->surface);
            SDL_free(job);
        }
    }
    return 0;
}

static void QueueScreenshotJob(SDL_Surface* surface) {
    if (!surface || !surface->pixels) {
        return;
    }

    ScreenshotJob* job = SDL_malloc(sizeof(ScreenshotJob));
    if (!job) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to allocate screenshot job");
        return;
    }

    job->surface = surface;
    SDL_snprintf(job->filename, sizeof(job->filename), "screenshot_%llu.bmp", (unsigned long long)SDL_GetTicks());

    SDL_LockMutex(screenshot_mutex);
    if (pending_screenshot_job) {
        SDL_DestroySurface(pending_screenshot_job->surface);
        SDL_free(pending_screenshot_job);
    }
    pending_screenshot_job = job;
    SDL_SignalCondition(screenshot_cond);
    SDL_UnlockMutex(screenshot_mutex);
}

static SDL_FRect calculate_letterbox_rect(int win_w, int win_h) {
    float out_w = (float)win_w;
    float out_h = (float)win_w / display_target_ratio;

    if (out_h > (float)win_h) {
        out_h = (float)win_h;
        out_w = (float)win_h * display_target_ratio;
    }

    SDL_FRect rect;
    rect.w = out_w;
    rect.h = out_h;
    rect.x = ((float)win_w - out_w) / 2.0f;
    rect.y = ((float)win_h - out_h) / 2.0f;

    return rect;
}

static void invalidate_letterbox_cache() {
    letterbox_rect_valid = false;
}

static const SDL_FRect* get_letterbox_rect() {
    if (!letterbox_rect_valid) {
        cached_letterbox_rect = calculate_letterbox_rect(screen_texture->w, screen_texture->h);
        letterbox_rect_valid = true;
    }
    return &cached_letterbox_rect;
}

static void create_screen_texture() {
    if (screen_texture != NULL) {
        SDL_DestroyTexture(screen_texture);
    }

    int target_width, target_height;
    SDL_GetRenderOutputSize(renderer, &target_width, &target_height);
    
    screen_texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_TARGET, 
        target_width, target_height);
    SDL_SetTextureScaleMode(screen_texture, SDL_SCALEMODE_LINEAR);
    
    invalidate_letterbox_cache();
}

int SDLApp_Init() {
    SDL_SetAppMetadata(app_name, "0.1", NULL);
    SDL_SetHint(SDL_HINT_VIDEO_WAYLAND_PREFER_LIBDECOR, "1");
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer(app_name,
                                     window_default_width,
                                     window_default_height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                     &window,
                                     &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return 1;
    }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    SDLMessageRenderer_Initialize(renderer);
    SDLGameRenderer_Init(renderer);
    create_screen_texture();
    SDLPad_Init();

    // Initialize screenshot thread
    screenshot_mutex = SDL_CreateMutex();
    screenshot_cond = SDL_CreateCondition();
    if (!screenshot_mutex || !screenshot_cond) {
        SDL_Log("Couldn't create screenshot synchronization primitives: %s", SDL_GetError());
        return 1;
    }
    screenshot_thread = SDL_CreateThread(ScreenshotSaverThread, "ScreenshotSaver", NULL);
    if (!screenshot_thread) {
        SDL_Log("Couldn't create screenshot thread: %s", SDL_GetError());
        return 1;
    }

    return 0;
}

void SDLApp_Quit() {
    // Shutdown screenshot thread
    if (screenshot_thread) {
        SDL_LockMutex(screenshot_mutex);
        screenshot_thread_running = false;
        SDL_SignalCondition(screenshot_cond);
        SDL_UnlockMutex(screenshot_mutex);
        SDL_WaitThread(screenshot_thread, NULL);
    }

    if (pending_screenshot_job) {
        SDL_DestroySurface(pending_screenshot_job->surface);
        SDL_free(pending_screenshot_job);
    }

    if (screenshot_cond) SDL_DestroyCondition(screenshot_cond);
    if (screenshot_mutex) SDL_DestroyMutex(screenshot_mutex);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

static void set_screenshot_flag_if_needed(SDL_KeyboardEvent* event) {
    if ((event->key == SDLK_GRAVE) && event->down && !event->repeat) {
        should_save_screenshot = true;
    }
}

static void handle_fullscreen_toggle(SDL_KeyboardEvent* event) {
    if ((event->key == SDLK_F11) && event->down && !event->repeat) {
        const SDL_WindowFlags flags = SDL_GetWindowFlags(window);
        SDL_SetWindowFullscreen(window, (flags & SDL_WINDOW_FULLSCREEN) ? 0 : SDL_WINDOW_FULLSCREEN);
    }
}

static void handle_mouse_motion() {
    last_mouse_motion_time = SDL_GetTicks();
    SDL_ShowCursor();
}

static void hide_cursor_if_needed() {
    const Uint64 now = SDL_GetTicks();
    if ((last_mouse_motion_time > 0) && ((now - last_mouse_motion_time) > mouse_hide_delay_ms)) {
        SDL_HideCursor();
    }
}

int SDLApp_PollEvents() {
    SDL_Event event;
    int continue_running = 1;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
            SDLPad_HandleGamepadDeviceEvent(&event.gdevice);
            break;

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            SDLPad_HandleGamepadButtonEvent(&event.gbutton);
            break;

        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
            SDLPad_HandleGamepadAxisMotionEvent(&event.gaxis);
            break;

        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            set_screenshot_flag_if_needed(&event.key);
            handle_fullscreen_toggle(&event.key);
            SDLPad_HandleKeyboardEvent(&event.key);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            handle_mouse_motion();
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            create_screen_texture();
            break;

        case SDL_EVENT_QUIT:
            continue_running = 0;
            break;
        }
    }

    return continue_running;
}

void SDLApp_BeginFrame() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderClear(renderer);

    SDLMessageRenderer_BeginFrame();
    SDLGameRenderer_BeginFrame();
}

static void note_frame_end_time() {
    frame_end_times[frame_end_times_index] = SDL_GetTicksNS();
    frame_end_times_index = (frame_end_times_index + 1) % FRAME_END_TIMES_MAX;
    if (frame_end_times_index == 0) {
        frame_end_times_filled = true;
    }
}

static void update_fps() {
    if (!frame_end_times_filled) {
        return;
    }

    const int oldest_idx = frame_end_times_index;
    const int newest_idx = (frame_end_times_index - 1 + FRAME_END_TIMES_MAX) % FRAME_END_TIMES_MAX;
    const Uint64 time_span_ns = frame_end_times[newest_idx] - frame_end_times[oldest_idx];
    
    if (time_span_ns > 0) {
        fps = (FRAME_END_TIMES_MAX - 1) * 1e9 / (double)time_span_ns;
    }
}

void SDLApp_EndFrame() {
    SDLADXSound_ProcessTracks();

    begin_interrupt();
    ADXPS2_ExecVint(0);
    end_interrupt();

    SDLGameRenderer_RenderFrame();

    // Render to screen texture
    SDL_SetRenderTarget(renderer, screen_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    const SDL_FRect* dst_rect = get_letterbox_rect();
    SDL_RenderTexture(renderer, cps3_canvas, NULL, dst_rect);
    SDL_RenderTexture(renderer, message_canvas, NULL, dst_rect);

    // Async screenshot - pixel readback happens here, file I/O in background thread
    if (should_save_screenshot) {
        SDL_Surface* screenshot = SDL_RenderReadPixels(renderer, NULL);
        if (screenshot) {
            QueueScreenshotJob(screenshot);
        }
    }

    // Render to window
    SDL_SetRenderTarget(renderer, NULL);
    SDL_RenderTexture(renderer, screen_texture, NULL, NULL);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_SetRenderScale(renderer, 2, 2);
    SDL_RenderDebugTextFormat(renderer, 8, 8, "FPS: %.3f", fps);
    SDL_SetRenderScale(renderer, 1, 1);

    SDL_RenderPresent(renderer);

    SDLGameRenderer_EndFrame();
    should_save_screenshot = false;

    hide_cursor_if_needed();

    // Frame pacing
    const Uint64 now = SDL_GetTicksNS();

    if (frame_deadline == 0) {
        frame_deadline = now + target_frame_time_ns;
    }

    if (now < frame_deadline) {
        SDL_DelayNS(frame_deadline - now);
    }

    frame_deadline += target_frame_time_ns;

    const Uint64 now_after_sleep = SDL_GetTicksNS();
    if (now_after_sleep > frame_deadline + target_frame_time_ns) {
        frame_deadline = now_after_sleep + target_frame_time_ns;
    }

    frame_counter += 1;
    note_frame_end_time();
    update_fps();
}

void SDLApp_Exit() {
    SDL_Event quit_event;
    quit_event.type = SDL_EVENT_QUIT;
    SDL_PushEvent(&quit_event);
}
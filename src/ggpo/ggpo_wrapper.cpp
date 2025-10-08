#include "ggponet.h"
#include "ggpo_wrapper.h"
#include "platform_linux.h"

static bool on_event_callback(void* context, GGPOEvent *info) {
    return true;
}

static bool advance_frame_callback(void* context, int flags) {
    return true;
}

static bool log_game_state_callback(void* context, char *filename, unsigned char *buffer, int len) {
    return true;
}

static bool load_game_state_callback(void* context, unsigned char *buffer, int len, int frame) {
    return true;
}

static bool save_game_state_callback(void* context, unsigned char **buffer, int *len, int *checksum, int frame) {
    return true;
}

static void free_buffer_callback(void* context, void *buffer) {
}

extern "C" {

GGPOSession* ggpo_start_synctest_wrapper(
    int num_players,
    int input_size,
    int frames
) {
    GGPOSession* ggpo = NULL;
    GGPOSessionCallbacks cb = {};
    cb.on_event = on_event_callback;
    cb.advance_frame = advance_frame_callback;
    cb.load_game_state = load_game_state_callback;
    cb.save_game_state = save_game_state_callback;
    cb.log_game_state = log_game_state_callback;
    cb.free_buffer = free_buffer_callback;
    ggpo_start_synctest(&ggpo, &cb, "3sx", num_players, input_size, frames);
    return ggpo;
}

void ggpo_close_session_wrapper(GGPOSession* ggpo) {
    if (ggpo) {
        ggpo_close_session(ggpo);
    }
}

void ggpo_idle_wrapper(GGPOSession* ggpo) {
    if (ggpo) {
        ggpo_idle(ggpo);
    }
}

int ggpo_add_local_input_wrapper(GGPOSession* ggpo, GGPOPlayerHandle player, void* values, int size) {
    if (ggpo) {
        return ggpo_add_local_input(ggpo, player, values, size);
    }
    return 0;
}

int ggpo_synchronize_input_wrapper(GGPOSession* ggpo, void* values, int size, int* disconnect_flags) {
    if (ggpo) {
        return ggpo_synchronize_input(ggpo, values, size, disconnect_flags);
    }
    return 0;
}

}
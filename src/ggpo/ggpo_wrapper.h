#ifndef GGPO_WRAPPER_H
#define GGPO_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
typedef class GGPOSession GGPOSession;
#else
typedef struct GGPOSession GGPOSession;
#endif

typedef int GGPOPlayerHandle;

GGPOSession* ggpo_start_synctest_wrapper(
    int num_players,
    int input_size,
    int frames
);

void ggpo_close_session_wrapper(GGPOSession* ggpo);

void ggpo_idle_wrapper(GGPOSession* ggpo);

int ggpo_add_local_input_wrapper(GGPOSession* ggpo, GGPOPlayerHandle player, void* values, int size);
int ggpo_synchronize_input_wrapper(GGPOSession* ggpo, void* values, int size, int* disconnect_flags);

#ifdef __cplusplus
}
#endif

#endif // GGPO_WRAPPER_H
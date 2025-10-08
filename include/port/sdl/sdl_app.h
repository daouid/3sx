#ifndef SDL_APP_H
#define SDL_APP_H

int SDLApp_Init();
void SDLApp_Quit();
int SDLApp_PollEvents();
#include "ggpo_wrapper.h"

void SDLApp_BeginFrame();
void SDLApp_EndFrame();
void SDLApp_Exit();
GGPOSession* SDLApp_GetGgpoSession();
Uint64 SDLApp_GetFrameCounter();

#endif

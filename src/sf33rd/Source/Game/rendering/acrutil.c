/**
 * @file acrutil.c
 */

#include "sf33rd/Source/Game/rendering/acrutil.h"
#include "common.h"
#include "sf33rd/AcrSDK/ps2/flps2render.h"
#include "structs.h"

void setBackGroundColor(u32 color) {
    flSetRenderState(FLRENDER_BACKCOLOR, color);
}

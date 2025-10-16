/**
 * @file eff22.c
 * TODO: identify what this effect does
 */

#include "sf33rd/Source/Game/effect/eff22.h"
#include "bin2obj/char_table.h"
#include "common.h"
#include "sf33rd/Source/Game/aboutspr.h"
#include "sf33rd/Source/Game/effect/effect.h"
#include "sf33rd/Source/Game/engine/charset.h"
#include "sf33rd/Source/Game/engine/slowf.h"
#include "sf33rd/Source/Game/engine/workuser.h"
#include "sf33rd/Source/Game/stage/ta_sub.h"
#include "sf33rd/Source/Game/texcash.h"

const s16 snow_pos_tbl[12][2] = { { 112, 416 }, { 176, 256 }, { 216, 384 }, { 288, 448 }, { 368, 288 }, { 448, 384 },
                                  { 512, 240 }, { 576, 432 }, { 640, 328 }, { 704, 488 }, { 816, 256 }, { 880, 384 } };

const s32 snow_sp[4][12][4] = { { { 0x200, 0, -0x2000, 0 },
                                  { 0x400, 0, -0x1800, 0 },
                                  { 0x800, 0, -0x2000, 0 },
                                  { -0x200, 0, -0x2200, 0 },
                                  { 0x1200, 0, -0x2000, 0 },
                                  { 0, 0, -0x1C00, 0 },
                                  { -0x800, 0, -0x2100, 0 },
                                  { 0x400, 0, -0x1800, 0 },
                                  { 0x800, 0, -0x1E00, 0 },
                                  { -0x100, 0, -0x2000, 0 },
                                  { -0x400, 0, -0x2400, 0 },
                                  { -0x80, 0, -0x2800, 0 } },
                                {
                                    { 0x100, 0, -0x2100, 0 },
                                    { 0x200, 0, -0x1900, 0 },
                                    { -0x100, 0, -0x2500, 0 },
                                    { 0xC00, 0, -0x2300, 0 },
                                    { 0x1200, 0, -0x2000, 0 },
                                    { 0x1000, 0, -0x1F00, 0 },
                                    { -0x800, 0, -0x2100, 0 },
                                    { 0x1200, 0, -0x2700, 0 },
                                    { 0x200, 0, -0x2000, 0 },
                                    { -0x80, 0, -0x2300, 0 },
                                    { 0x80, 0, -0x1600, 0 },
                                    { -0x200, 0, -0x2900, 0 },
                                },
                                {
                                    { 0x100, 0, -0x1A00, 0 },
                                    { 0x200, 0, -0x2200, 0 },
                                    { 0x400, 0, -0x2600, 0 },
                                    { -0x1000, 0, -0x1200, 0 },
                                    { 0x1200, 0, -0x2000, 0 },
                                    { 0x2000, 0, -0x2000, 0 },
                                    { 0x800, 0, -0x1E00, 0 },
                                    { -0x400, 0, -0x2F00, 0 },
                                    { 0x180, 0, -0x2600, 0 },
                                    { -0x80, 0, -0x2200, 0 },
                                    { 0x100, 0, -0x1A00, 0 },
                                    { 0, 0, -0x2800, 0 },
                                },
                                { { 0x180, 0, -0x1A00, 0 },
                                  { 0, 0, -0x1400, 0 },
                                  { 0x1200, 0, -0x2400, 0 },
                                  { -0x800, 0, -0x2600, 0 },
                                  { 0x1200, 0, -0x2000, 0 },
                                  { -0x1000, 0, -0x1200, 0 },
                                  { 0x800, 0, -0x1E00, 0 },
                                  { 0x100, 0, -0x2200, 0 },
                                  { -0x100, 0, -0x1C00, 0 },
                                  { 0x100, 0, -0x1400, 0 },
                                  { -0x80, 0, -0x1800, 0 },
                                  { 0, 0, -0x2000, 0 } } };

void effect_22_move(WORK_Other* ewk) {
    const s32* ptr;

    if (obr_no_disp_check()) {
        return;
    }

    if (compel_dead_check(ewk) != 0) {
        ewk->wu.routine_no[0] = 99;
        ewk->wu.disp_flag = 0;
        return;
    }

    switch (ewk->wu.routine_no[0]) {
    case 0:
        ewk->wu.routine_no[0]++;
        ewk->wu.dead_f = 1;
        set_char_move_init(&ewk->wu, 0, 11);
        ewk->wu.disp_flag = 1;
        ewk->wu.old_rno[0] = 0;
        /* fallthrough */

    case 1:
        ewk->wu.routine_no[0]++;
        ptr = &snow_sp[ewk->wu.old_rno[0]][ewk->wu.type][0];
        ewk->wu.mvxy.a[0].sp = *ptr++;
        ewk->wu.mvxy.d[0].sp = *ptr++;
        ewk->wu.mvxy.a[1].sp = *ptr++;
        ewk->wu.mvxy.d[1].sp = *ptr++;
        ewk->wu.xyz[0].disp.pos = snow_pos_tbl[ewk->wu.type][0];
        ewk->wu.xyz[1].disp.pos = snow_pos_tbl[ewk->wu.type][1];
        ewk->wu.old_rno[0]++;
        ewk->wu.old_rno[0] &= 3;
        break;

    case 2:
        if (!EXE_flag && !Game_pause) {
            add_x_sub(&ewk->wu);
            add_y_sub(&ewk->wu);

            if (ewk->wu.xyz[1].disp.pos <= 23) {
                ewk->wu.routine_no[0] = 1;
            }
        }

        disp_pos_trans_entry_r(ewk);
        break;

    default:
        all_cgps_put_back(&ewk->wu);
        push_effect_work(&ewk->wu);
        break;
    }
}

s32 effect_22_init() {
    s16 ix;
    s16 i;
    WORK_Other* ewk;

    for (i = 0; i < 12; i++) {
        if ((ix = pull_effect_work(4)) == -1) {
            return -1;
        }

        ewk = (WORK_Other*)frw[ix];
        ewk->wu.be_flag = 1;
        ewk->wu.id = 22;
        ewk->wu.work_id = 16;
        ewk->wu.cgromtype = 1;
        ewk->wu.type = i;
        ewk->wu.my_family = 2;
        ewk->wu.my_col_mode = 0x4200;
        ewk->wu.my_col_code = 300;
        ewk->wu.my_mts = 7;
        ewk->wu.my_trans_mode = get_my_trans_mode(ewk->wu.my_mts);
        ewk->wu.char_table[0] = _rca_char_table;
        ewk->wu.my_priority = ewk->wu.position_z = 10;
    }

    return 0;
}

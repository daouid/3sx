#include "sf33rd/Source/Game/effl6.h"
#include "bin2obj/char_table.h"
#include "common.h"
#include "sf33rd/Source/Game/CALDIR.h"
#include "sf33rd/Source/Game/CHARSET.h"
#include "sf33rd/Source/Game/EFFECT.h"
#include "sf33rd/Source/Game/SLOWF.h"
#include "sf33rd/Source/Game/aboutspr.h"
#include "sf33rd/Source/Game/bg.h"
#include "sf33rd/Source/Game/bg_sub.h"
#include "sf33rd/Source/Game/ta_sub.h"
#include "sf33rd/Source/Game/texcash.h"
#include "sf33rd/Source/Game/workuser.h"

void effl6_flont(WORK_Other* ewk);
void effl6_back(WORK_Other* ewk);

void effect_L6_move(WORK_Other* ewk) {
    WORK* oya_ptr = (WORK*)ewk->my_master;

    switch (ewk->wu.routine_no[0]) {
    case 0:
        if (!EXE_flag && !Game_pause) {
            if (ewk->wu.type) {
                effl6_flont(ewk);
            } else {
                effl6_back(ewk);
            }
        }

        suzi_sync_pos_set(ewk);
        sort_push_request(&ewk->wu);
        break;

    default:
        push_effect_work(&ewk->wu);
        break;
    }
}

void effl6_flont(WORK_Other* ewk) {
#if defined(TARGET_PS2)
    void set_char_move_init(WORK * wk, s16 koc, s32 index);
#endif

    switch (ewk->wu.routine_no[1]) {
    case 0:
        ewk->wu.routine_no[1]++;
        ewk->wu.disp_flag = 1;
        ewk->wu.kage_flag = 1;
        ewk->wu.kage_hx = 0;
        ewk->wu.kage_hy = -10;
        ewk->wu.kage_prio = 71;
        ewk->wu.kage_char = 16;
        set_char_move_init(&ewk->wu, 0, ewk->wu.char_index);
        break;

    case 1:
        char_move(&ewk->wu);
        add_x_sub(ewk);
        add_y_sub(ewk);
        ewk->wu.old_rno[0]--;

        if (ewk->wu.old_rno[0] <= 0) {
            ewk->wu.routine_no[1]++;
            set_char_move_init(&ewk->wu, 0, 1);
        }

        break;

    case 2:
        char_move(&ewk->wu);

        if (ewk->wu.cg_type == 0xFF) {
            ewk->wu.routine_no[1]++;
            set_char_move_init(&ewk->wu, 1, 59);
        }

        break;

    case 3:
        char_move(&ewk->wu);
        break;
    }
}

void effl6_back(WORK_Other* ewk) {
#if defined(TARGET_PS2)
    void set_char_move_init(WORK * wk, s16 koc, s32 index);
#endif

    switch (ewk->wu.routine_no[1]) {
    case 0:
        ewk->wu.routine_no[1]++;
        ewk->wu.disp_flag = 1;
        ewk->wu.kage_flag = 1;
        ewk->wu.kage_hx = 0;
        ewk->wu.kage_hy = -10;
        ewk->wu.kage_prio = 71;
        ewk->wu.kage_char = 16;
        set_char_move_init(&ewk->wu, 0, ewk->wu.char_index);
        break;

    case 1:
        char_move(&ewk->wu);
        add_x_sub(ewk);
        add_y_sub(ewk);
        ewk->wu.old_rno[0]--;

        if (ewk->wu.old_rno[0] <= 0) {
            ewk->wu.routine_no[1]++;
            set_char_move_init(&ewk->wu, 0, 1);
        }

        break;

    case 2:
        char_move(&ewk->wu);

        if (ewk->wu.cg_type == 0xFF) {
            ewk->wu.routine_no[1]++;
            set_char_move_init(&ewk->wu, 1, 52);
        }

        break;

    case 3:
        char_move(&ewk->wu);
        break;
    }
}

s32 effect_L6_init(WORK* wk, u8 typel6) {
#if defined(TARGET_PS2)
    s16 get_my_trans_mode(s32 curr);
#endif

    s16 ix;
    WORK_Other* ewk;

    if ((ix = pull_effect_work(4)) == -1) {
        return -1;
    }

    ewk = (WORK_Other*)frw[ix];
    ewk->wu.be_flag = 1;
    ewk->wu.id = 216;
    ewk->wu.work_id = 16;
    ewk->master_id = wk->id;
    ewk->wu.type = typel6;
    ewk->wu.cgromtype = 1;
    ewk->wu.my_col_mode = wk->my_col_mode;
    ewk->wu.my_col_code = wk->my_col_code + 1;
    ewk->wu.my_family = wk->my_family;
    ewk->my_master = (u32*)wk;
    ewk->wu.xyz[1].disp.pos = wk->xyz[1].disp.pos - 12;
    ewk->wu.my_priority = 28;
    ewk->wu.position_z = 28;
    ewk->wu.char_table[0] = _etc3_char_table;
    ewk->wu.char_table[1] = _etc_char_table;
    ewk->wu.char_index = 0;
    ewk->wu.sync_suzi = 0;

    if (typel6) {
        ewk->wu.rl_flag = wk->rl_flag ^ 1;

        if (wk->rl_flag) {
            ewk->wu.xyz[0].disp.pos = bg_w.bgw[1].wxy[0].disp.pos + bg_w.pos_offset;
            ewk->wu.xyz[0].disp.pos += 32;
            ewk->wu.old_rno[1] = wk->xyz[0].disp.pos + 96;
        } else {
            ewk->wu.xyz[0].disp.pos = bg_w.bgw[1].wxy[0].disp.pos - bg_w.pos_offset;
            ewk->wu.xyz[0].disp.pos -= 32;
            ewk->wu.old_rno[1] = wk->xyz[0].disp.pos - 96;
        }

        ewk->wu.old_rno[0] = 120;
        cal_initial_speed(&ewk->wu, ewk->wu.old_rno[0], ewk->wu.old_rno[1], ewk->wu.xyz[1].disp.pos);
    } else {
        ewk->wu.rl_flag = wk->rl_flag;

        if (wk->rl_flag) {
            if (wk->xyz[0].disp.pos < bg_w.bgw[1].wxy[0].disp.pos) {
                ewk->wu.xyz[0].disp.pos = wk->xyz[0].disp.pos - 256;
            } else {
                ewk->wu.xyz[0].disp.pos = bg_w.bgw[1].wxy[0].disp.pos - (bg_w.pos_offset + 32);
            }

            ewk->wu.old_rno[1] = wk->xyz[0].disp.pos - 32;
        } else {
            if (wk->xyz[0].disp.pos > bg_w.bgw[1].wxy[0].disp.pos) {
                ewk->wu.xyz[0].disp.pos = wk->xyz[0].disp.pos + 256;
            } else {
                ewk->wu.xyz[0].disp.pos = bg_w.bgw[1].wxy[0].disp.pos + (bg_w.pos_offset + 32);
            }

            ewk->wu.old_rno[1] = wk->xyz[0].disp.pos + 32;
        }

        ewk->wu.old_rno[0] = 80;
        cal_initial_speed(&ewk->wu, ewk->wu.old_rno[0], ewk->wu.old_rno[1], ewk->wu.xyz[1].disp.pos);
    }

    suzi_offset_set(ewk);
    ewk->wu.my_mts = 14;
    ewk->wu.my_trans_mode = get_my_trans_mode(ewk->wu.my_mts);
    return 0;
}

#include "sf33rd/Source/Game/EFFD1.h"
#include "bin2obj/char_table.h"
#include "common.h"
#include "sf33rd/Source/Game/CALDIR.h"
#include "sf33rd/Source/Game/CHARSET.h"
#include "sf33rd/Source/Game/EFFECT.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/aboutspr.h"
#include "sf33rd/Source/Game/ta_sub.h"
#include "sf33rd/Source/Game/texcash.h"
#include "sf33rd/Source/Game/workuser.h"

void fall_data_set(WORK_Other* ewk);

void effect_D1_move(WORK_Other* ewk) {
#if defined(TARGET_PS2)
    void set_char_move_init(WORK * wk, s16 koc, s32 index);
#endif

    if (Exec_Wipe) {
        ewk->wu.no_death_attack = 1;
    }

    switch (ewk->wu.routine_no[0]) {
    case 0:
        ewk->wu.routine_no[0]++;
        ewk->wu.disp_flag = 1;
        set_char_move_init(&ewk->wu, 0, (s16)(ewk->wu.char_index));
        fall_data_set(ewk);
        pl_eff_trans_entry(ewk);
        break;

    case 1:
        if (ewk->wu.no_death_attack && !Exec_Wipe) {
            ewk->wu.routine_no[0] = 99;
        } else {
            if (!ewk->wu.cg_type) {
                char_move(&ewk->wu);
            }

            ewk->wu.old_rno[0]--;

            if (ewk->wu.old_rno[0] < 36) {
                ewk->wu.routine_no[0]++;
                ewk->wu.my_priority = ewk->wu.position_z = 20;
            }

            add_x_sub(ewk);
            add_y_sub(ewk);
        }

        pl_eff_trans_entry(ewk);
        break;

    case 2:
        if (ewk->wu.no_death_attack && !Exec_Wipe) {
            ewk->wu.routine_no[0] = 99;
        } else {
            if (!ewk->wu.cg_type) {
                char_move(&ewk->wu);
            }

            ewk->wu.old_rno[0]--;

            if (ewk->wu.old_rno[0] != 0) {
                add_x_sub(ewk);
                add_y_sub(ewk);
            } else {
                ewk->wu.routine_no[0]++;
            }
        }

        pl_eff_trans_entry(ewk);
        break;

    case 3:
        if (ewk->wu.no_death_attack && !Exec_Wipe) {
            ewk->wu.routine_no[0] = 99;
        } else {
            char_move(&ewk->wu);
        }

        pl_eff_trans_entry(ewk);
        break;

    case 99:
        ewk->wu.disp_flag = 0;
        ewk->wu.routine_no[0]++;
        break;

    default:
        all_cgps_put_back(&ewk->wu);
        push_effect_work(&ewk->wu);
        break;
    }
}

void fall_data_set(WORK_Other* ewk) {
    WORK_Other* oya_ef = (WORK_Other*)ewk->my_master;
    s16 pos_work;
    s16 id_work;

    ewk->wu.old_rno[0] = 44;
    id_work = oya_ef->master_id ^ 1;
    pos_work = plw[oya_ef->master_id].wu.xyz[0].disp.pos - plw[id_work].wu.xyz[0].disp.pos;
    ewk->wu.old_rno[2] = -8;

    if (ewk->wu.rl_flag) {
        ewk->wu.xyz[0].disp.pos += 32;

        if (pos_work > 0) {
            pos_work = 32;
        } else if (pos_work > -32) {
            pos_work = 32;
        } else {
            pos_work = -pos_work;
        }

        ewk->wu.old_rno[1] = oya_ef->wu.xyz[0].disp.pos + pos_work;
    } else {
        ewk->wu.xyz[0].disp.pos -= 32;

        if (pos_work > 0) {
            if (pos_work < 32) {
                pos_work = 32;
            }
        } else {
            pos_work = 32;
        }

        ewk->wu.old_rno[1] = oya_ef->wu.xyz[0].disp.pos - pos_work;
    }

    cal_all_speed_data(&ewk->wu, ewk->wu.old_rno[0], ewk->wu.old_rno[1], ewk->wu.old_rno[2], 2, 1);
}

s32 effect_D1_init(WORK_Other* oya, s32 /* unused */) {
#if defined(TARGET_PS2)
    s16 get_my_trans_mode(s32 curr);
#endif

    WORK_Other* ewk;
    s16 ix;

    if ((ix = pull_effect_work(3)) == -1) {
        return -1;
    }

    ewk = (WORK_Other*)frw[ix];
    ewk->wu.be_flag = 1;
    ewk->wu.id = 131;
    ewk->wu.work_id = 16;
    ewk->wu.cgromtype = 1;
    ewk->wu.disp_flag = 0;
    ewk->my_master = (u32*)oya;
    ewk->wu.my_family = 2;
    ewk->wu.char_index = 17;
    ewk->master_id = oya->master_id;
    ewk->wu.my_col_mode = 0x4200;
    ewk->wu.my_priority = ewk->wu.position_z = oya->wu.position_z;
    ewk->wu.sync_suzi = 0;
    ewk->wu.rl_flag = oya->wu.rl_flag;

    if (oya->master_id) {
        ewk->wu.my_col_code = 22;
    } else {
        ewk->wu.my_col_code = 6;
    }

    *ewk->wu.char_table = _etc_char_table;
    ewk->wu.xyz[1].disp.pos = oya->wu.xyz[1].disp.pos + 138;
    ewk->wu.xyz[0].disp.pos = oya->wu.xyz[0].disp.pos;
    ewk->wu.no_death_attack = 0;
    ewk->wu.my_mts = 14;
    ewk->wu.my_trans_mode = get_my_trans_mode(ewk->wu.my_mts);
    return 0;
}

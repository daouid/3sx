#include "sf33rd/Source/Game/texgroup.h"
#include "common.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"
#include "sf33rd/Source/Game/CHARID.h"
#include "sf33rd/Source/Game/GD3rd.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/RAMCNT.h"
#include "sf33rd/Source/Game/chren3rd.h"
#include "sf33rd/Source/Game/main.h"
#include "sf33rd/Source/Game/texcash.h"
#include "structs.h"

#if !defined(TARGET_PS2)
#include <stdlib.h>
#endif

typedef struct {
    // total size: 0x8
    s16 x;    // offset 0x0, size 0x2
    s16 y;    // offset 0x2, size 0x2
    u16 attr; // offset 0x4, size 0x2
    u16 code; // offset 0x6, size 0x2
} TexGroup_UNK_0;

u8 omSelObjNowOnMemoryType = 0xFF; // size: 0x1, address: 0x574414
TEX_GRP_LD texgrplds[100];         // size: 0x4B0, address: 0x6B49D0

const TexGroupData texgrpdat[100] = { { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 1460, 0, 1, 3040072, 210820, 2945368 },
                                      { 1568, 1465, 0, 1, 2552860, 116432, 2420192 },
                                      { 2592, 1468, 0, 1, 1630652, 72828, 1543992 },
                                      { 3552, 1472, 0, 1, 2231332, 114816, 2075148 },
                                      { 4992, 1476, 0, 1, 2093456, 110728, 1973588 },
                                      { 6144, 1479, 0, 1, 2987268, 116636, 2870824 },
                                      { 7392, 1483, 0, 1, 3342276, 158400, 3225900 },
                                      { 8384, 1487, 0, 1, 2700876, 151744, 2480392 },
                                      { 10208, 1492, 0, 1, 1849896, 142292, 1707212 },
                                      { 11776, 1495, 0, 1, 2241340, 137680, 2119316 },
                                      { 13280, 1499, 0, 1, 2337716, 116892, 2190948 },
                                      { 14656, 1502, 0, 1, 1649368, 71900, 1556728 },
                                      { 15712, 1506, 0, 1, 1715944, 80596, 1618808 },
                                      { 16800, 1510, 0, 1, 2286852, 135428, 2186344 },
                                      { 18272, 1514, 0, 1, 2649420, 116116, 2547656 },
                                      { 19456, 1518, 0, 1, 2672552, 144584, 2541372 },
                                      { 21120, 1522, 0, 1, 2770372, 177724, 2635716 },
                                      { 23008, 1525, 0, 1, 3792908, 222124, 3666100 },
                                      { 24704, 1528, 0, 1, 2345792, 131348, 2208892 },
                                      { 25856, 1531, 0, 1, 1830216, 125420, 1724240 },
                                      { 27040, 1452, 0, 0, 1483, 436, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 27104, 1454, 0, 0, 1128236, 134308, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 29152, 1455, 0, 0, 176650, 14180, 0 },
                                      { 29344, 1456, 0, 0, 595797, 92368, 0 },
                                      { 30640, 1461, 0, 2, 88153, 5788, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 30896, 1457, 0, 0, 35820, 3448, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 31152, 1446, 0, 0, 2148437, 120700, 0 },
                                      { 32432, 1444, 0, 0, 36241, 2580, 0 },
                                      { 36896, 1462, 0, 2, 147800, 4212, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 32560, 1458, 0, 0, 170685, 25088, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 34352, 1401, 0, 0, 47835, 1704, 0 },
                                      { 34384, 1410, 0, 0, 57961, 2180, 0 },
                                      { 34448, 1389, 0, 0, 81332, 19772, 0 },
                                      { 34576, 1395, 0, 0, 47809, 3256, 0 },
                                      { 34672, 1428, 0, 0, 15596, 1672, 0 },
                                      { 34704, 1405, 0, 0, 41072, 1832, 0 },
                                      { 34736, 1413, 0, 0, 16744, 2220, 0 },
                                      { 34832, 1425, 0, 0, 5845, 296, 0 },
                                      { 34864, 1398, 0, 0, 157729, 12208, 0 },
                                      { 34960, 1434, 0, 0, 96435, 4016, 0 },
                                      { 35024, 1386, 0, 0, 62413, 4568, 0 },
                                      { 35120, 1407, 0, 0, 32337, 1100, 0 },
                                      { 35152, 1443, 0, 0, 2302, 64, 0 },
                                      { 35184, 1440, 0, 0, 111032, 7372, 0 },
                                      { 35328, 1431, 0, 0, 214221, 17548, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 35648, 1392, 0, 0, 40123, 2100, 0 },
                                      { 35744, 1448, 0, 0, 102144, 8496, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 },
                                      { 35904, 74, 0, 0, 1024977, 54492, 0 },
                                      { 36096, 34, 0, 0, 335656, 6552, 0 },
                                      { 36160, 35, 0, 0, 67041, 1540, 0 },
                                      { 36192, 36, 0, 0, 171245, 11480, 0 },
                                      { 36288, 37, 0, 0, 84254, 2300, 0 },
                                      { 36320, 38, 0, 0, 57154, 1712, 0 },
                                      { 36352, 39, 0, 0, 120474, 6460, 0 },
                                      { 36384, 40, 0, 0, 62031, 2332, 0 },
                                      { 36416, 41, 0, 0, 20849, 3412, 0 },
                                      { 36448, 42, 0, 0, 10724, 444, 0 },
                                      { 36480, 43, 0, 0, 32145, 1072, 0 },
                                      { 36512, 44, 0, 0, 83717, 2832, 0 },
                                      { 36544, 45, 0, 0, 152553, 4676, 0 },
                                      { 36576, 46, 0, 0, 69000, 5992, 0 },
                                      { 36608, 47, 0, 0, 233695, 8904, 0 },
                                      { 36640, 48, 0, 0, 320671, 14508, 0 },
                                      { 36704, 49, 0, 0, 65229, 1536, 0 },
                                      { 36736, 50, 0, 0, 122647, 2968, 0 },
                                      { 36768, 51, 0, 0, 166764, 6628, 0 },
                                      { 36800, 52, 0, 0, 61918, 2868, 0 },
                                      { 36864, 53, 0, 0, 182919, 4776, 0 },
                                      { 37024, 1459, 0, 0, 621536, 42292, 0 },
                                      { 37408, 1384, 0, 0, 35825, 2488, 0 },
                                      { 37536, 1385, 0, 0, 135261, 12984, 0 },
                                      { 34576, 1416, 0, 0, 47809, 3256, 0 },
                                      { 34448, 1419, 0, 0, 81332, 19772, 0 },
                                      { 34736, 1422, 0, 0, 16744, 2220, 0 },
                                      { 34352, 1437, 0, 0, 47835, 1704, 0 },
                                      { 30640, 1469, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1488, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1496, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1503, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1507, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1511, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1515, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1519, 0, 2, 88153, 5788, 0 },
                                      { 30640, 1532, 0, 2, 88153, 5788, 0 },
                                      { 27104, 1453, 0, 0, 1126129, 134044, 0 },
                                      { 0, 65535, 0, 0, 0, 0, 0 } };

// forward decls
s32 load_any_texture_grpnum(u8 grp, u8 kokey);

void q_ldreq_texture_group(REQ* curr) {
    const TexGroupData* bsd;
    CharInitData* cit;
    CharInitData* cit2;
    uintptr_t ldadr;
    uintptr_t ldchd;
    s32 err;
    s16 i;
    u32* patchAdrs;
    u16* trsbas;
    TexGroup_UNK_0* trsptr;
    s16 count;
    s16 loop;

    bsd = &texgrpdat[curr->ix];

    switch (curr->rno) {
    case 0:
        if (fsCheckCommandExecuting() != 0) {
            break;
        }

        curr->rno = 1;
        curr->fnum = bsd->apfn;

        if (bsd->apfn == 0xFFFF) {
            *curr->result |= lpr_wrdata[curr->id];
            curr->be = 0;
        }

        if (bsd->num_of_1st == 0) {
            curr->group = obj_group_table[bsd->num_of_1st + 1];
        } else {
            curr->group = obj_group_table[bsd->num_of_1st];
        }

        curr->lds = &texgrplds[curr->group];

        if (curr->lds->ok) {
            if (bsd->ix1st == 1 || bsd->ix1st == 2) {
                switch (rckey_work[curr->lds->key].type) {
                case 3:
                    if (curr->id) {
                        rckey_work[curr->lds->key].type = 5;
                    }

                    break;

                case 4:
                    if (curr->id == 0) {
                        rckey_work[curr->lds->key].type = 5;
                    }

                    break;

                case 5:
                    break;
                }

                if (rckey_work[curr->lds->key].type == 5) {
                    *curr->result |= lpr_wrdata[curr->id];
                    curr->be = 0;
                    return;
                }

                // A duplicate transfer occurred. File number: %d\n
                flLogOut("二重転送が発生しました。ファイル番号：%d\n", bsd->apfn);
                while (1) {}
            }

            rckey_work[curr->lds->key].type = curr->kokey;
            *curr->result |= lpr_wrdata[curr->id];
            curr->be = 0;
            break;
        }

        /* fallthrough */

    case 1:
        err = fsOpen(curr);

        if (err == 0) {
            curr->rno = 0;
            return;
        }

        curr->rno = 2;
        /* fallthrough */

    case 2:
        curr->size = fsGetFileSize(curr->fnum);
        curr->sect = fsCalSectorSize(curr->size);
        curr->key = Pull_ramcnt_key(curr->sect << 0xB, curr->kokey, curr->group, curr->frre);
        curr->lds->key = curr->key;
        Set_size_data_ramcnt_key(curr->key, curr->size);
        curr->rno = 3;
        /* fallthrough */

    case 3:
        err = fsRequestFileRead(curr, curr->sect, (void*)Get_ramcnt_address(curr->key));

        if (err == 0) {
            Push_ramcnt_key(curr->key);
            fsClose(curr);
            curr->rno = 0;
            return;
        }

        curr->rno = 4;
        curr->be = 1;
        break;

    case 4:
        switch (fsCheckFileReaded(curr)) {
        case 1:
            fsClose(curr);
            ldadr = Get_ramcnt_address(curr->key);
            curr->lds->texture_table = ldadr + bsd->to_tex;
            curr->lds->trans_table = ldadr;
            curr->lds->ok = 1;

            switch (bsd->ix1st) {
            case 1:
                ldchd = ldadr + bsd->to_chd;

                // Explanation:
                //
                // The code above loads a bunch of data from the AFS partition.
                // This data includes character init data which starts at `ldchd`.
                // Data at `ldchd` starts with 25 4-byte ints which are offsets
                // from `ldchd` to the actual data.
                //
                // On PS2 it is okay to just add `ldchd` to each of these offsets
                // to turn them into pointers, because a 4-byte int can hold a pointer.
                // However on modern 64-bit platforms pointers are bigger, meaning we
                // can't add `ldchd` to the offsets inplace. That's why we have to
                // allocate a separate memory region for `cit` and compute the pointers
                // that comprise it there.
                //
                // Because 25 is the number of members in CharInitData struct, `i` goes
                // to 25 too.

#if defined(TARGET_PS2)
                for (i = 0; i < 25; i++) {
                    ((u32*)ldchd)[i] += ldchd;
                }

                cit = (CharInitData*)ldchd;
#else
                cit = (CharInitData*)malloc(sizeof(CharInitData));

                for (i = 0; i < 25; i++) {
                    ((uintptr_t*)cit)[i] = ldchd + ((u32*)ldchd)[i];
                }
#endif

                cit2 = &char_init_data[plid_data[plt_req[curr->id]]];
                *cit2 = *cit;

#if !defined(TARGET_PS2)
                free(cit);
#endif

                parabora_own_table[plt_req[curr->id]] = cit2->prot;

                // Q specific code
                if (curr->ix == 18) {
#if defined(TARGET_PS2)
                    patchAdrs = ((u32**)ldchd)[8];
                    patchAdrs[37] = patchAdrs[3];
#else
                    cit2->cbca[37] = cit2->cbca[3];
#endif
                }

                // Akuma specific code
                if (curr->ix == 15) {
                    trsbas = (u16*)(((u32*)texgrplds[15].trans_table)[166] + texgrplds[15].trans_table);
                    count = *trsbas;
                    count -= 1;
                    trsbas[0] = count;
                    trsbas += 1;
                    trsptr = (TexGroup_UNK_0*)trsbas;
                    trsptr[0].x += trsptr[1].x;
                    trsptr[0].y += trsptr[1].y;
                    trsptr[0].attr = trsptr[1].attr;
                    trsptr[0].code = trsptr[1].code;

                    for (loop = 1; loop < count; loop++) {
                        trsptr[loop] = trsptr[loop + 1];
                    }
                }
            }

            *curr->result |= lpr_wrdata[curr->id];
            curr->be = 0;
            break;

        case 0:
            break;

        default:
            Push_ramcnt_key(curr->key);
            fsClose(curr);
            curr->be = 2;
            curr->rno = 0;
            break;
        }

        break;
    }
}

void Init_texgrplds_work() {
    s16 i;

    // Zero out the 0-th element of texgrplds
    for (i = 0; i < sizeof(TEX_GRP_LD) / sizeof(u32); i++) {
        ((u32*)texgrplds)[i] = 0;
    }

    for (i = 1; i < 100; i++) {
        texgrplds[i] = texgrplds[0];
    }
}

void reservMemKeySelObj() {
    TEX_GRP_LD* lds;
    s32 size;

    size = fsCalSectorSize(0x11372CU) << 0xB;
    lds = &texgrplds[obj_group_table[0x69E0]];
    lds->key = Pull_ramcnt_key(size, 0xD, 0, 1);

    if (lds->key < 0) {
        while (1) {}
    }
}

void checkSelObjFileLoaded() {
    const TexGroupData* bsd;
    TEX_GRP_LD* lds;
    uintptr_t ldadr;
    s32 rnum;

    if (omSelObjNowOnMemoryType == mpp_w.language) {
        return;
    }

    if (mpp_w.language) {
        bsd = &texgrpdat[0x62];
    } else {
        bsd = &texgrpdat[0x17];
    }

    lds = &texgrplds[obj_group_table[0x69E0]];

    while (1) {
        rnum = load_it_use_this_key(bsd->apfn, lds->key);

        if (rnum != 0) {
            break;
        }
    }

    ldadr = Get_ramcnt_address(lds->key);
    lds->texture_table = ldadr + bsd->to_tex;
    lds->trans_table = ldadr;
    lds->ok = 1;
    omSelObjNowOnMemoryType = mpp_w.language;
    Clear_texcash_work();
}

void purge_texture_group_of_this(u16 patnum) {
#if defined(TARGET_PS2)
    void purge_texture_group(u16 grp);
#endif

    purge_texture_group(obj_group_table[patnum]);
}

void purge_texture_group(u8 grp) {
    if (texgrplds[grp].ok != 0) {
        texgrplds[grp].ok = 0;
        Push_ramcnt_key(texgrplds[grp].key);
    }
}

void purge_player_texture(s16 id) {
    s16 emid;
    s16 pkey;

    emid = (id + 1) & 1;

    if ((pkey = Search_ramcnt_type(lpt_seldat[2])) != 0) {
        while (1) {
            rckey_work[pkey].type = lpt_seldat[emid];

            if (!(pkey = Search_ramcnt_type(lpt_seldat[2]))) {
                break;
            }
        }
    }

    while (1) {
        pkey = Search_ramcnt_type(lpt_seldat[id]);

        if (pkey == 0) {
            break;
        }

        Push_ramcnt_key(pkey);
    }
}

s32 load_any_texture_patnum(u16 patnum, u8 kokey, u8 _unused) {
#if defined(TARGET_PS2)
    s32 load_any_texture_grpnum(u16 grp, u16 kokey);
#endif

    return load_any_texture_grpnum(obj_group_table[patnum], kokey);
}

s32 load_any_texture_grpnum(u8 grp, u8 kokey) {
    const TexGroupData* bsd;
    TEX_GRP_LD* lds;
    uintptr_t ldadr;

    if (grp == 0) {
        return 0;
    }

    lds = &texgrplds[grp];
    bsd = &texgrpdat[grp];

    if (lds->ok) {
        return 0;
    }

    lds->key = load_it_use_any_key(bsd->apfn, kokey, grp);
    ldadr = Get_ramcnt_address(lds->key);
    lds->texture_table = ldadr + bsd->to_tex;
    lds->trans_table = ldadr;
    lds->ok = 1;
    return 1;
}

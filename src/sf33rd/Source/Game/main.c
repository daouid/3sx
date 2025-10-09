#include "sf33rd/Source/Game/main.h"
#include "common.h"
#include "port/sdl/sdl_app.h"
#include "sf33rd/AcrSDK/common/mlPAD.h"
#include "sf33rd/AcrSDK/ps2/flps2debug.h"
#include "sf33rd/AcrSDK/ps2/flps2etc.h"
#include "sf33rd/AcrSDK/ps2/flps2render.h"
#include "sf33rd/AcrSDK/ps2/foundaps2.h"
#include "sf33rd/Source/Common/MemMan.h"
#include "sf33rd/Source/Common/PPGFile.h"
#include "sf33rd/Source/Common/PPGWork.h"
#include "sf33rd/Source/Compress/zlibApp.h"
#include "sf33rd/Source/Game/AcrUtil.h"
#include "sf33rd/Source/Game/DC_Ghost.h"
#include "sf33rd/Source/Game/EFFECT.h"
#include "sf33rd/Source/Game/GD3rd.h"
#include "sf33rd/Source/Game/IOConv.h"
#include "sf33rd/Source/Game/MTRANS.h"
#include "sf33rd/Source/Game/PLCNT.h"
#include "sf33rd/Source/Game/RAMCNT.h"
#include "sf33rd/Source/Game/SYS_sub.h"
#include "sf33rd/Source/Game/SYS_sub2.h"
#include "sf33rd/Source/Game/Sound3rd.h"
#include "sf33rd/Source/Game/WORK_SYS.h"
#include "sf33rd/Source/Game/bg.h"
#include "sf33rd/Source/Game/color3rd.h"
#include "sf33rd/Source/Game/debug/Debug.h"
#include "sf33rd/Source/Game/init3rd.h"
#include "sf33rd/Source/Game/texcash.h"
#include "sf33rd/Source/Game/workuser.h"
#include "sf33rd/Source/PS2/mc/knjsub.h"
#include "sf33rd/Source/PS2/mc/mcsub.h"
#include "sf33rd/Source/PS2/ps2Quad.h"
#include "structs.h"

#if defined(_WIN32)
#include <windef.h>
#include <ConsoleApi.h>
#include <stdio.h>
#endif

#include <memory.h>
#include <stdlib.h>

// =============================================================================
// CPS-3 ARCADE PERFECT TIMING
// =============================================================================

#define CPS3_REFRESH_RATE 59.599491
#define FRAME_TIME_NS 16778532ULL
#define MAX_FRAME_CATCHUP 4

#define SCREEN_WIDTH 384.0f
#define SCREEN_HEIGHT 224.0f
#define MAX_TASKS 11
#define MAX_CHARS 0x14
#define PPG_BUFFER_SIZE 0x60000
#define ZLIB_BUFFER_SIZE 0x10000
#define DEFAULT_COUNTRY 4

#ifdef NDEBUG
    #define DEBUG_CHECK(x) 0
    #define DEBUG_PRINT(...)
#else
    #define DEBUG_CHECK(x) (Debug_w[x])
    #define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#endif

// =============================================================================
// GLOBAL STATE
// =============================================================================

s32 system_init_level;
MPP mpp_w;

static u8 dctex_linear_mem[0x800];
static u8 texcash_melt_buffer_mem[0x1000];
static u8 tpu_free_mem[0x2000];

static u64 g_lastTime = 0;
static u64 g_accumulator = 0;
static u32 g_frameCount = 0;

static struct {
    f32 pos_h[8];
    f32 pos_v[8];
    f32 last_x1;
    f32 last_y1;
    u8 needs_h_mask;
    u8 needs_v_mask;
    u8 padding[2];
} g_viewportCache = {
    .last_x1 = -1.0f,
    .last_y1 = -1.0f,
    .needs_h_mask = 0,
    .needs_v_mask = 0
};

// =============================================================================
// HIGH-RESOLUTION TIMER
// =============================================================================

static inline u64 getTimeNanoseconds(void) {
#if defined(_WIN32)
    static LARGE_INTEGER s_frequency = {0};
    LARGE_INTEGER count;
    
    if (s_frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&s_frequency);
    }
    
    QueryPerformanceCounter(&count);
    return (u64)((count.QuadPart * 1000000000ULL) / s_frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u64)ts.tv_sec * 1000000000ULL + (u64)ts.tv_nsec;
#endif
}

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

static void distributeScratchPadAddress(void);
static void processInput(void);
static void updateGameState(void);
static void renderFrame(void);
static void handleDebugInput(void);
static void processGameLogic(void);
static void cpLoopTask(void);
static void MaskScreenEdge(void);
static void appCopyKeyData(void);
static inline void processPlayerInput(void);
static inline void updateViewport(void);
static void cpInitTask(void);
static void cpReadyTask(u16 num, void* func_adrs);
static void cpExitTask(u16 num);
static u8* mppMalloc(u32 size);
static void njUserInit(void);
static s32 njUserMain(void);
static int initConsole(void);
static int initializeSystems(void);
static void cleanupSystems(void);

// =============================================================================
// MAIN ENTRY POINT
// =============================================================================

int main(void) {
#if defined(_WIN32)
    initConsole();
#endif

    DEBUG_PRINT("Street Fighter III: 3rd Strike - CPS-3 Arcade Perfect\n");
    DEBUG_PRINT("Refresh Rate: %.6f Hz | Frame Time: %.3f ms\n\n", 
                CPS3_REFRESH_RATE, FRAME_TIME_NS / 1000000.0);

    if (SDLApp_Init() != 0) {
        fprintf(stderr, "FATAL: SDL initialization failed\n");
        return 1;
    }

    if (!initializeSystems()) {
        fprintf(stderr, "FATAL: System initialization failed\n");
        SDLApp_Quit();
        return 1;
    }

    g_lastTime = getTimeNanoseconds();
    g_accumulator = 0;
    g_frameCount = 0;

    DEBUG_PRINT("Entering main loop...\n\n");

    int running = 1;
    
    while (running) {
        u64 currentTime = getTimeNanoseconds();
        u64 frameTime = currentTime - g_lastTime;
        g_lastTime = currentTime;

        if (frameTime > (FRAME_TIME_NS * MAX_FRAME_CATCHUP)) {
            frameTime = FRAME_TIME_NS * MAX_FRAME_CATCHUP;
            DEBUG_PRINT("WARNING: Frame time clamped (%.2f ms behind)\n", 
                       (frameTime - FRAME_TIME_NS) / 1000000.0);
        }

        g_accumulator += frameTime;

        running = SDLApp_PollEvents();
        if (!running) {
            break;
        }

        while (g_accumulator >= FRAME_TIME_NS) {
            processInput();
            updateGameState();
            g_accumulator -= FRAME_TIME_NS;
        }

        renderFrame();
        g_frameCount++;
    }

    DEBUG_PRINT("\nShutdown complete. Frames rendered: %u\n", g_frameCount);
    cleanupSystems();
    SDLApp_Quit();
    
    return 0;
}

// =============================================================================
// INITIALIZATION
// =============================================================================

static int initConsole(void) {
#if defined(_WIN32)
    if (AttachConsole(ATTACH_PARENT_PROCESS) == 0) {
        if (AllocConsole() == 0) {
            return 0;
        }
    }

    FILE* fp = NULL;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    
    return 1;
#else
    return 1;
#endif
}

static int initializeSystems(void) {
    if (flInitialize(flPs2State.DispWidth, flPs2State.DispHeight) != 0) {
        fprintf(stderr, "ERROR: flInitialize failed\n");
        return 0;
    }

    flSetRenderState(FLRENDER_BACKCOLOR, 0);
    flSetDebugMode(0);

    system_init_level = 0;
    ppgWorkInitializeApprication();
    distributeScratchPadAddress();
    njdp2d_init();
    njUserInit();
    palCreateGhost();
    ppgMakeConvTableTexDC();
    appSetupBasePriority();
    MemcardInit();

    return 1;
}

static void cleanupSystems(void) {
}

static void distributeScratchPadAddress(void) {
    dctex_linear = (s16*)dctex_linear_mem;
    texcash_melt_buffer = (u8*)texcash_melt_buffer_mem;
    tpu_free = (TexturePoolUsed*)tpu_free_mem;
}

// =============================================================================
// INPUT PROCESSING
// =============================================================================

static void processInput(void) {
    flPADGetALL();
    keyConvert();
    handleDebugInput();
    Interrupt_Flag = 0;
    processPlayerInput();
    appCopyKeyData();
}

static inline void processPlayerInput(void) {
    const int play_mode = Play_Mode;
    const int game_pause = Game_pause;
    
    if ((play_mode != 3 && play_mode != 1) || (game_pause != 0x81)) {
        p1sw_1 = p1sw_0;
        p2sw_1 = p2sw_0;
        p3sw_1 = p3sw_0;
        p4sw_1 = p4sw_0;
        
        p1sw_0 = p1sw_buff;
        p2sw_0 = p2sw_buff;
        p3sw_0 = p3sw_buff;
        p4sw_0 = p4sw_buff;

        if ((task[3].condition == 1) && (Mode_Type == 4) && (play_mode == 1)) {
            u16 sw_buff = p2sw_0;
            p2sw_0 = p1sw_0;
            p1sw_0 = sw_buff;
        }
    }
}

static void handleDebugInput(void) {
    if ((Usage != 7 && Usage != 2) || test_flag) {
        return;
    }

    if (mpp_w.sysStop) {
        if (mpp_w.sysStop == 1) {
            sysSLOW = 1;

            switch (io_w.data[1].sw_new) {
            case 0x2000:
                mpp_w.sysStop = 0;
            case 0x80:
                Slow_Timer = 1;
                break;

            default:
                switch (io_w.data[1].sw & 0x880) {
                case 0x880:
                    sysFF = DEBUG_CHECK(1);
                    if (sysFF == 0) {
                        sysFF = 1;
                    }
                    sysSLOW = 1;
                    Slow_Timer = 1;
                    break;

                case 0x800:
                    if (Slow_Timer == 0) {
                        Slow_Timer = DEBUG_CHECK(0);
                        if (Slow_Timer == 0) {
                            Slow_Timer = 1;
                        }
                        sysFF = 1;
                    }
                    break;

                default:
                    Slow_Timer = 2;
                    break;
                }
                break;
            }
        }
    } else if (io_w.data[1].sw_new & 0x2000) {
        mpp_w.sysStop = 1;
    }
}

static void appCopyKeyData(void) {
    PLsw[0][1] = PLsw[0][0];
    PLsw[1][1] = PLsw[1][0];
    PLsw[0][0] = p1sw_buff;
    PLsw[1][0] = p2sw_buff;
}

// =============================================================================
// GAME STATE UPDATE
// =============================================================================

static void updateGameState(void) {
    processGameLogic();

    Interrupt_Flag = 1;
    Interrupt_Timer++;
    Record_Timer++;

    Scrn_Renew();
    Irl_Family();
    Irl_Scrn();
    BGM_Server();
}

static void processGameLogic(void) {
    CPU_Time_Lag[0] = 0;
    CPU_Time_Lag[1] = 0;
    CPU_Rec[0] = 0;
    CPU_Rec[1] = 0;

    Check_Replay_Status(0, Replay_Status[0]);
    Check_Replay_Status(1, Replay_Status[1]);

    Frame_Zoom_X = Screen_Zoom_X + SA_Zoom_X;
    Frame_Zoom_Y = Screen_Zoom_Y + SA_Zoom_Y;

    if (sys_w.disp.now == sys_w.disp.new) {
        cpLoopTask();

        if ((Game_pause != 0x81) && (Mode_Type == 1) && (Play_Mode == 1)) {
            if ((plw[0].wu.operator == 0) && (CPU_Rec[0] == 0) && (Replay_Status[0] == 1)) {
                p1sw_0 = 0;
                Check_Replay_Status(0, 1);

                if (DEBUG_CHECK(0x21)) {
                    flPrintColor(0xFFFFFFFF);
                    flPrintL(0x10, 0xA, "FAKE REC! PL1");
                }
            }

            if ((plw[1].wu.operator == 0) && (CPU_Rec[1] == 0) && (Replay_Status[1] == 1)) {
                p2sw_0 = 0;
                Check_Replay_Status(1, 1);

                if (DEBUG_CHECK(0x21)) {
                    flPrintColor(0xFFFFFFFF);
                    flPrintL(0x10, 0xA, "FAKE REC!     PL2");
                }
            }
        }
    } else {
        sys_w.disp.now = sys_w.disp.new;
    }
}

// =============================================================================
// RENDERING
// =============================================================================

static void renderFrame(void) {
    SDLApp_BeginFrame();
    initRenderState(0);

    updateViewport();

    appViewSetItems(&mpp_w.vprm);
    appViewMatrix();
    flAdjustScreen(X_Adjust + Correct_X[0], Y_Adjust + Correct_Y[0]);
    
    setBackGroundColor(DEBUG_CHECK(0x43) ? 0xFF0000FF : 0xFF000000);
    
    appSetupTempPriority();

    render_start();
    mpp_w.inGame = 0;
    
    njUserMain();

    MaskScreenEdge();
    seqsBeforeProcess();
    njdp2d_draw();
    seqsAfterProcess();

    if (DEBUG_CHECK(6) == 0) {
        CP3toPS2Draw();
    }

    KnjFlush();
    render_end();

    u32 sysinfodisp = 0;
    if (DEBUG_CHECK(2) == 2) {
        sysinfodisp = 3;
    } else if (DEBUG_CHECK(2) == 1) {
        sysinfodisp = 2;
    }

    if (mpp_w.sysStop == 2) {
        sysinfodisp = 0;
    } else if (mpp_w.sysStop == 1) {
        sysinfodisp &= ~2;
    }

    flSetDebugMode(sysinfodisp);
    disp_effect_work();
    flFlip(0);
}

static inline void updateViewport(void) {
    mpp_w.ds_h[0] = mpp_w.ds_h[1];
    mpp_w.ds_v[0] = mpp_w.ds_v[1];
    mpp_w.ds_h[1] = 100;
    mpp_w.ds_v[1] = 100;
    mpp_w.vprm.x0 = 0.0f;
    mpp_w.vprm.y0 = 0.0f;
    mpp_w.vprm.x1 = (mpp_w.ds_h[0] * SCREEN_WIDTH) / 100.0f;
    mpp_w.vprm.y1 = (mpp_w.ds_v[0] * SCREEN_HEIGHT) / 100.0f;
    mpp_w.vprm.ne = -1.0f;
    mpp_w.vprm.fa = 1.0f;
}

static void MaskScreenEdge(void) {
    VPRM prm;
    appViewGetItems(&prm);

    if (prm.x1 != g_viewportCache.last_x1 || prm.y1 != g_viewportCache.last_y1) {
        g_viewportCache.last_x1 = prm.x1;
        g_viewportCache.last_y1 = prm.y1;
        
        g_viewportCache.needs_h_mask = (prm.x1 < SCREEN_WIDTH);
        g_viewportCache.needs_v_mask = (prm.y1 < SCREEN_HEIGHT);

        if (g_viewportCache.needs_h_mask) {
            g_viewportCache.pos_h[0] = g_viewportCache.pos_h[4] = mpp_w.vprm.x1;
            g_viewportCache.pos_h[2] = g_viewportCache.pos_h[6] = SCREEN_WIDTH;
            g_viewportCache.pos_h[1] = g_viewportCache.pos_h[3] = mpp_w.vprm.y0;
            g_viewportCache.pos_h[5] = g_viewportCache.pos_h[7] = SCREEN_HEIGHT;
        }

        if (g_viewportCache.needs_v_mask) {
            g_viewportCache.pos_v[0] = g_viewportCache.pos_v[4] = mpp_w.vprm.x0;
            g_viewportCache.pos_v[2] = g_viewportCache.pos_v[6] = SCREEN_WIDTH;
            g_viewportCache.pos_v[1] = g_viewportCache.pos_v[3] = mpp_w.vprm.y1;
            g_viewportCache.pos_v[5] = g_viewportCache.pos_v[7] = SCREEN_HEIGHT;
        }
    }

    if (g_viewportCache.needs_h_mask) {
        njdp2d_sort(g_viewportCache.pos_h, PrioBase[0], 0xFF000000, 0);
    }

    if (g_viewportCache.needs_v_mask) {
        njdp2d_sort(g_viewportCache.pos_v, PrioBase[0], 0xFF000000, 0);
    }
}

// =============================================================================
// TASK MANAGEMENT
// =============================================================================

static void cpLoopTask(void) {
    struct _TASK* task_ptr = task;

    disp_ramcnt_free_area();

    if (sysSLOW) {
        if (--Slow_Timer == 0) {
            sysSLOW = 0;
            Game_pause &= 0x7F;
        } else {
            Game_pause |= 0x80;
        }
    }

    Process_Counter = 1;

    if (Turbo && (Game_pause != 0x81)) {
        if (--Turbo_Timer == 0) {
            Turbo_Timer = Turbo;
            Process_Counter = 2;
        }
    }

    for (current_task_num = 0; current_task_num < MAX_TASKS; current_task_num++, task_ptr++) {
        if (task_ptr->condition == 1) {
            if (task_ptr->func_adrs != NULL) {
                task_ptr->func_adrs(task_ptr);
            }
        } else if (task_ptr->condition == 2) {
            task_ptr->condition = 1;
        }
    }
}

static void cpInitTask(void) {
    memset(&task, 0, sizeof(task));
}

static void cpReadyTask(u16 num, void* func_adrs) {
    if (num >= MAX_TASKS) {
        fprintf(stderr, "ERROR: cpReadyTask - invalid task %u (max %d)\n", num, MAX_TASKS);
        return;
    }

    if (func_adrs == NULL) {
        fprintf(stderr, "ERROR: cpReadyTask - NULL function for task %u\n", num);
        return;
    }

    struct _TASK* task_ptr = task + num;
    memset(task_ptr, 0, sizeof(struct _TASK));
    task_ptr->func_adrs = func_adrs;
    task_ptr->condition = 2;
}

static void cpExitTask(u16 num) {
    if (num >= MAX_TASKS) {
        fprintf(stderr, "ERROR: cpExitTask - invalid task %u (max %d)\n", num, MAX_TASKS);
        return;
    }

    struct _TASK* task_ptr = task + num;
    task_ptr->condition = 0;

    if (task_ptr->callback_adrs != NULL) {
        task_ptr->callback_adrs();
    }
}

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

s32 mppGetFavoritePlayerNumber(void) {
    s32 max = 1;
    s32 num = 0;

    if (DEBUG_CHECK(0x2D)) {
        return DEBUG_CHECK(0x2D) - 1;
    }

    for (s32 i = 0; i < MAX_CHARS; i++) {
        if (max <= mpp_w.useChar[i]) {
            max = mpp_w.useChar[i];
            num = i + 1;
        }
    }

    return num;
}

static u8* mppMalloc(u32 size) {
    u8* ptr = flAllocMemory(size);
    if (ptr == NULL) {
        fprintf(stderr, "FATAL: Memory allocation failed (%u bytes)\n", size);
        exit(1);
    }
    return ptr;
}

// =============================================================================
// GAME INITIALIZATION
// =============================================================================

static void njUserInit(void) {
    u32 size;
    u8* mem_ptr;

    DEBUG_PRINT("Initializing game systems...\n");

    sysFF = 1;
    mpp_w.sysStop = 0;
    mpp_w.inGame = 0;
    mpp_w.ctrDemo = 0;
    mpp_w.language = 0;
    mpp_w.langload = -1;
    mpp_w.pal50Hz = 0;

    mpp_w.vprm.x0 = 0.0f;
    mpp_w.vprm.y0 = 0.0f;
    mpp_w.vprm.x1 = SCREEN_WIDTH;
    mpp_w.vprm.y1 = SCREEN_HEIGHT;
    mpp_w.vprm.ne = -1.0f;
    mpp_w.vprm.fa = 1.0f;
    
    appViewSetItems(&mpp_w.vprm);
    appViewMatrix();

    mmSystemInitialize();
    flGetFrame(&mpp_w.fmsFrame);

    size = seqsGetUseMemorySize();
    mem_ptr = mppMalloc(size);
    seqsInitialize(mem_ptr);
    DEBUG_PRINT("  ✓ Sequences: %u bytes\n", size);

    mem_ptr = mppMalloc(PPG_BUFFER_SIZE);
    ppg_Initialize(mem_ptr, PPG_BUFFER_SIZE);
    DEBUG_PRINT("  ✓ PPG: %u bytes\n", PPG_BUFFER_SIZE);

    mem_ptr = mppMalloc(ZLIB_BUFFER_SIZE);
    zlib_Initialize(mem_ptr, ZLIB_BUFFER_SIZE);
    DEBUG_PRINT("  ✓ zlib: %u bytes\n", ZLIB_BUFFER_SIZE);

    size = flGetSpace();
    mem_ptr = mppMalloc(size);
    mpp_w.ramcntBuff = mem_ptr;
    Init_ram_control_work(mpp_w.ramcntBuff, size);
    DEBUG_PRINT("  ✓ RAM control: %u bytes\n", size);

    for (s32 i = 0; i < MAX_CHARS; i++) {
        mpp_w.useChar[i] = 0;
    }

    Interrupt_Timer = 0;
    SA_Zoom_X = 0.0f;
    SA_Zoom_Y = 0.0f;
    Disp_Size_H = 100;
    Disp_Size_V = 100;
    mpp_w.ds_h[0] = mpp_w.ds_h[1] = Disp_Size_H;
    mpp_w.ds_v[0] = mpp_w.ds_v[1] = Disp_Size_V;

    Country = DEFAULT_COUNTRY;
    if (Country == 0) {
        fprintf(stderr, "CRITICAL: Country was 0, forcing to %d\n", DEFAULT_COUNTRY);
        Country = DEFAULT_COUNTRY;
    }
    
    Screen_PAL = 0;
    Turbo = 0;
    Turbo_Timer = 1;

    Screen_Zoom_X = 1.0f;
    Screen_Zoom_Y = 1.0f;
    Setup_Disp_Size(0);
    Correct_X[0] = 0;
    Correct_Y[0] = 0;
    Frame_Zoom_X = Screen_Zoom_X + SA_Zoom_X;
    Frame_Zoom_Y = Screen_Zoom_Y + SA_Zoom_Y;
    Zoom_Base_Position_X = 0;
    Zoom_Base_Position_Y = 0;
    Zoom_Base_Position_Z = 0;

    sys_w.disp.now = sys_w.disp.new = 1;
    sys_w.pause = 0;
    sys_w.reset = 0;

    Init_sound_system();
    Init_bgm_work();
    Setup_Directory_Record_Data();
    sndInitialLoad();

    cpInitTask();
    cpReadyTask(INIT_TASK_NUM, Init_Task);

    DEBUG_PRINT("  ✓ Initialization complete\n\n");
}

static s32 njUserMain(void) {
    CPU_Time_Lag[0] = 0;
    CPU_Time_Lag[1] = 0;
    CPU_Rec[0] = 0;
    CPU_Rec[1] = 0;
    
    Check_Replay_Status(0, Replay_Status[0]);
    Check_Replay_Status(1, Replay_Status[1]);
    
    Frame_Zoom_X = Screen_Zoom_X + SA_Zoom_X;
    Frame_Zoom_Y = Screen_Zoom_Y + SA_Zoom_Y;
    
    return sys_w.gd_error;
}
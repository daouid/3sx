#include "common.h"
#include "port/sdl/sdl_pad.h"
#include "port/sdl/sdl_app.h"
#include "ggpo_wrapper.h"

#include <libpad2.h>

#include <string.h>

int scePad2Init(int mode) {
    // Do nothing
    return 1;
}

int scePad2End(void) {
    not_implemented(__func__);
}

int scePad2GetState(int socket_number) {
    return SDLPad_IsGamepadConnected(socket_number) ? scePad2StateStable : scePad2StateNoLink;
}

int scePad2GetButtonProfile(int socket_number, unsigned char* profile) {
    // Profile for Digital controller
    // profile[0] = 0xF9;
    // profile[1] = 0xFF;
    // profile[2] = 0;
    // profile[3] = 0;

    // Profile for Dualshock 2
    profile[0] = 0xFF;
    profile[1] = 0xFF;
    profile[2] = 0xFF;
    profile[3] = 0xFF;

    return 4;
}

static int get_local_input(int socket_number) {
    SDLPad_ButtonState button_state;
    SDLPad_GetButtonState(socket_number, &button_state);

    int input = 0;
    input |= button_state.south << 0;
    input |= button_state.east << 1;
    input |= button_state.west << 2;
    input |= button_state.north << 3;
    input |= button_state.back << 4;
    input |= button_state.start << 5;
    input |= button_state.left_stick << 6;
    input |= button_state.right_stick << 7;
    input |= button_state.left_shoulder << 8;
    input |= button_state.right_shoulder << 9;
    input |= (button_state.left_trigger > 0) << 10;
    input |= (button_state.right_trigger > 0) << 11;
    input |= button_state.dpad_up << 12;
    input |= button_state.dpad_down << 13;
    input |= button_state.dpad_left << 14;
    input |= button_state.dpad_right << 15;
    return input;
}

int scePad2Read(int socket_number, scePad2ButtonState* data) {
    memset(data, 0, sizeof(scePad2ButtonState));

    GGPOSession* ggpo = SDLApp_GetGgpoSession();
    int inputs[2] = {0, 0};
    int disconnect_flags;

    if (ggpo) {
        int local_input = get_local_input(socket_number);
        ggpo_add_local_input_wrapper(ggpo, socket_number, &local_input, sizeof(local_input));
        ggpo_synchronize_input_wrapper(ggpo, (void *)inputs, sizeof(inputs), &disconnect_flags);
    } else {
        inputs[socket_number] = get_local_input(socket_number);
    }

    int input = inputs[socket_number];

    // sw0 and sw1 store the pressed state of each button as bits.
    // 0 = pressed, 1 = released

    data->sw0.byte = 0xFF;
    data->sw1.byte = 0xFF;

    data->sw0.bits.l3 = !((input >> 6) & 1);
    data->sw0.bits.r3 = !((input >> 7) & 1);
    data->sw0.bits.select = !((input >> 4) & 1);
    data->sw0.bits.start = !((input >> 5) & 1);
    data->sw0.bits.left = !((input >> 14) & 1);
    data->sw0.bits.right = !((input >> 15) & 1);
    data->sw0.bits.up = !((input >> 12) & 1);
    data->sw0.bits.down = !((input >> 13) & 1);

    data->sw1.bits.l1 = !((input >> 8) & 1);
    data->sw1.bits.r1 = !((input >> 9) & 1);
    data->sw1.bits.l2 = !((input >> 10) & 1);
    data->sw1.bits.r2 = !((input >> 11) & 1);
    data->sw1.bits.cross = !((input >> 0) & 1);
    data->sw1.bits.circle = !((input >> 1) & 1);
    data->sw1.bits.square = !((input >> 2) & 1);
    data->sw1.bits.triangle = !((input >> 3) & 1);

    // This sets stick positions
    // (Sticks are not supported yet, that's why we just set positions to neutral)

    data->lJoyH = 0x7F;
    data->lJoyV = 0x7F;
    data->rJoyH = 0x7F;
    data->rJoyV = 0x7F;

    // This sets button pressure

    data->crossP = ((input >> 0) & 1) ? 0xFF : 0;
    data->circleP = ((input >> 1) & 1) ? 0xFF : 0;
    data->squareP = ((input >> 2) & 1) ? 0xFF : 0;
    data->triangleP = ((input >> 3) & 1) ? 0xFF : 0;
    data->upP = ((input >> 12) & 1) ? 0xFF : 0;
    data->downP = ((input >> 13) & 1) ? 0xFF : 0;
    data->leftP = ((input >> 14) & 1) ? 0xFF : 0;
    data->rightP = ((input >> 15) & 1) ? 0xFF : 0;

    return sizeof(scePad2ButtonState);
}

int scePad2CreateSocket(scePad2SocketParam* socket, void* addr) {
    return socket->port;
}

int scePad2DeleteSocket(int) {
    not_implemented(__func__);
}

int sceVibGetProfile(int socket_number, unsigned char* profile) {
    profile[0] = 3; // Small and big motor
    return 1;
}

int sceVibSetActParam(int socket_number, int profile_size, unsigned char* profile, int data_size, unsigned char* data) {
    const bool is_small_enabled = profile[0] & 1;
    const bool is_big_enabled = profile[0] & 2;
    unsigned char big_value = 0;
    bool small_value = false;

    if (is_small_enabled) {
        small_value = data[0] & 1;
    }

    if (is_big_enabled) {
        if (is_small_enabled) {
            big_value = ((data[0] & 0xFE) >> 1) | ((data[1] & 1) << 7);
        } else {
            big_value = data[0];
        }
    }

    SDLPad_RumblePad(socket_number, small_value, big_value);
    return 1;
}
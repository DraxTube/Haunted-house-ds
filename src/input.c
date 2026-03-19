/*
 * input.c  —  DS input handling
 */

#include "input.h"
#include "riot.h"
#include <nds.h>

uint8_t input_fire0 = 0;

void input_init(void)
{
    riot_swcha = 0xFF;
    riot_swchb = 0xFF;
    input_fire0 = 0;
}

void input_update(void)
{
    uint32_t held = keysHeld();

    /* Joystick (Player 0 — upper nibble of SWCHA) */
    uint8_t joy = 0xFF;
    if (held & KEY_RIGHT) joy &= ~(1 << 7);
    if (held & KEY_LEFT)  joy &= ~(1 << 6);
    if (held & KEY_DOWN)  joy &= ~(1 << 5);
    if (held & KEY_UP)    joy &= ~(1 << 4);
    riot_swcha = joy;

    /* Fire button */
    input_fire0 = (held & (KEY_A | KEY_B)) ? 1 : 0;

    /* Console switches */
    uint8_t sw = 0xFF;
    if (held & KEY_START)  sw &= ~(1 << 0);   /* Reset */
    if (held & KEY_SELECT) sw &= ~(1 << 1);   /* Select */
    if (held & KEY_L)      sw &= ~(1 << 6);   /* Difficulty A */
    if (held & KEY_R)      sw &= ~(1 << 7);   /* Difficulty B */
    riot_swchb = sw;
}

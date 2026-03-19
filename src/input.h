#pragma once
#include <stdint.h>

/*
 * input.h  —  DS button → Atari 2600 joystick/switch mapping
 *
 * Atari 2600 controls via RIOT SWCHA register:
 *   Bit 7: P0 Right   (0 = pressed)
 *   Bit 6: P0 Left
 *   Bit 5: P0 Down
 *   Bit 4: P0 Up
 *   Bit 3: P1 Right
 *   Bit 2: P1 Left
 *   Bit 1: P1 Down
 *   Bit 0: P1 Up
 *
 * Fire button is in INPT4 (TIA read register).
 *
 * DS mapping:
 *   D-pad       → Joystick direction
 *   A / B       → Fire button
 *   START       → Game Reset (SWCHB bit 0)
 *   SELECT      → Game Select (SWCHB bit 1)
 *   L / R       → Difficulty switches
 */

void input_init(void);
void input_update(void);   /* Call every frame after scanKeys() */

extern uint8_t input_fire0;   /* 1 if fire button held */

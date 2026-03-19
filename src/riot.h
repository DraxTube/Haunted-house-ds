#pragma once
#include <stdint.h>

/*
 * riot.h  —  MOS 6532 RIOT (RAM/IO/Timer) emulation
 *
 * Address map (A7=1 selects RIOT over TIA):
 *   0x280  SWCHA  — Joystick directions
 *   0x282  SWCHB  — Console switches (Reset, Select, difficulty)
 *   0x284  INTIM  — Timer read
 *   0x294  TIM1T  — Set timer / 1
 *   0x295  TIM8T  — Set timer / 8
 *   0x296  TIM64T — Set timer / 64
 *   0x297  T1024T — Set timer / 1024
 */

void    riot_init(void);
void    riot_write(uint16_t addr, uint8_t val);
uint8_t riot_read(uint16_t addr);
void    riot_tick(int cycles);   /* called each instruction */

/* Input state set by input.c */
extern uint8_t riot_swcha;   /* 0=pressed for each direction bit */
extern uint8_t riot_swchb;   /* console switches */

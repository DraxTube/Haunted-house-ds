#pragma once
#include <stdint.h>

/*
 * tia.h  —  Atari TIA (Television Interface Adapter) emulation
 *
 * Handles graphics registers and audio for the Atari 2600.
 * Haunted House uses a subset of TIA features:
 *   - Background color (COLUBK)
 *   - Player 0/1 graphics (GRP0, GRP1, COLUP0, COLUP1, HMP0, HMP1)
 *   - Playfield (PF0, PF1, PF2, COLUPF, CTRLPF)
 *   - Ball, Missile (not used heavily but mapped)
 *   - Collision registers
 *   - Audio channels 0 and 1 (AUDC, AUDF, AUDV)
 */

/* TIA write registers (addresses 0x00-0x2C) */
#define TIA_VSYNC   0x00
#define TIA_VBLANK  0x01
#define TIA_WSYNC   0x02
#define TIA_RSYNC   0x03
#define TIA_NUSIZ0  0x04
#define TIA_NUSIZ1  0x05
#define TIA_COLUP0  0x06
#define TIA_COLUP1  0x07
#define TIA_COLUPF  0x08
#define TIA_COLUBK  0x09
#define TIA_CTRLPF  0x0A
#define TIA_REFP0   0x0B
#define TIA_REFP1   0x0C
#define TIA_PF0     0x0D
#define TIA_PF1     0x0E
#define TIA_PF2     0x0F
#define TIA_RESP0   0x10
#define TIA_RESP1   0x11
#define TIA_RESM0   0x12
#define TIA_RESM1   0x13
#define TIA_RESBL   0x14
#define TIA_AUDC0   0x15
#define TIA_AUDC1   0x16
#define TIA_AUDF0   0x17
#define TIA_AUDF1   0x18
#define TIA_AUDV0   0x19
#define TIA_AUDV1   0x1A
#define TIA_GRP0    0x1B
#define TIA_GRP1    0x1C
#define TIA_ENAM0   0x1D
#define TIA_ENAM1   0x1E
#define TIA_ENABL   0x1F
#define TIA_HMP0    0x20
#define TIA_HMP1    0x21
#define TIA_HMM0    0x22
#define TIA_HMM1    0x23
#define TIA_HMBL    0x24
#define TIA_VDELP0  0x25
#define TIA_VDELP1  0x26
#define TIA_VDELBL  0x27
#define TIA_RESMP0  0x28
#define TIA_RESMP1  0x29
#define TIA_HMOVE   0x2A
#define TIA_HMCLR   0x2B
#define TIA_CXCLR   0x2C

/* TIA state exposed for video rendering */
typedef struct {
    uint8_t colubk;   /* background colour */
    uint8_t colupf;   /* playfield colour  */
    uint8_t colup0;   /* player 0 colour   */
    uint8_t colup1;   /* player 1 colour   */
    uint8_t pf0, pf1, pf2;
    uint8_t grp0, grp1;
    uint8_t ctrlpf;
    uint8_t x_p0, x_p1;   /* player X positions */
    uint8_t audc0, audc1;
    uint8_t audf0, audf1;
    uint8_t audv0, audv1;
    int     scanline;
    int     vsync;
} TIAState;

void      tia_init(void);
void      tia_write(uint8_t reg, uint8_t val);
uint8_t   tia_read(uint16_t addr);
TIAState *tia_get_state(void);

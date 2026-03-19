#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
 * emu6502.h  —  Minimal MOS 6507 (6502 subset) emulator
 * Used to run the original Haunted House Atari 2600 ROM.
 *
 * The 6507 is a cost-reduced 6502 with a 13-bit address bus (8 KB).
 * Address map:
 *   0x0000-0x007F  RAM  (128 bytes, mirrored)
 *   0x0080-0x00FF  RIOT (timer + I/O)
 *   0x0100-0x01FF  (mirrors of RAM/TIA - not used by this game)
 *   0x1000-0x1FFF  ROM  (4 KB, mirrored from 0xF000-0xFFFF)
 */

/* CPU state */
typedef struct {
    uint16_t pc;    /* Program Counter */
    uint8_t  sp;    /* Stack Pointer */
    uint8_t  a;     /* Accumulator */
    uint8_t  x;     /* X index */
    uint8_t  y;     /* Y index */
    uint8_t  p;     /* Processor Status */
    uint64_t cycles;
} CPU6502;

/* Status flag bits */
#define FLAG_C  0x01
#define FLAG_Z  0x02
#define FLAG_I  0x04
#define FLAG_D  0x08
#define FLAG_B  0x10
#define FLAG_V  0x40
#define FLAG_N  0x80

/* ~19950 cycles per NTSC frame (3,579,545 Hz / 60 Hz / 3 clocks per cycle) */
#define CYCLES_PER_FRAME  19950

void     emu6502_init(const uint8_t *rom, uint32_t rom_size);
void     emu6502_reset(void);
int      emu6502_step(void);          /* Execute one instruction, return cycles used */
void     emu6502_run_frame(void);     /* Run one full frame worth of cycles */
uint8_t  emu6502_read(uint16_t addr);
void     emu6502_write(uint16_t addr, uint8_t val);
CPU6502 *emu6502_get_state(void);

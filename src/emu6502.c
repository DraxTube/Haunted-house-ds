/*
 * emu6502.c  —  MOS 6507 emulator core
 *
 * Implements all official 6502 opcodes used by Haunted House.
 * Cycle-accurate enough for correct game timing.
 */

#include "emu6502.h"
#include "tia.h"
#include "riot.h"
#include <string.h>
#include <stdio.h>

/* ---- Memory map -------------------------------------------------- */
static uint8_t  ram[128];          /* 0x0000-0x007F */
static uint8_t  rom_buf[4096];     /* mapped at 0x1000-0x1FFF */
static CPU6502  cpu;

/* ---- Memory access ----------------------------------------------- */
uint8_t emu6502_read(uint16_t addr)
{
    addr &= 0x1FFF;   /* 6507 has 13-bit address bus */

    if (addr & 0x1000)
        return rom_buf[addr & 0x0FFF];   /* ROM */

    if (addr & 0x0080)
        return riot_read(addr);           /* RIOT */

    if (addr < 0x0080)
        return tia_read(addr);            /* TIA read-back */

    return ram[addr & 0x7F];
}

void emu6502_write(uint16_t addr, uint8_t val)
{
    addr &= 0x1FFF;

    if (addr & 0x1000) return;            /* ROM is read-only */

    if (addr & 0x0080) {
        riot_write(addr, val);
        return;
    }

    /* TIA registers: A12=0, A7=0 */
    if (!(addr & 0x0080))
        tia_write(addr & 0x3F, val);

    ram[addr & 0x7F] = val;
}

/* ---- Helpers ----------------------------------------------------- */
static inline uint8_t fetch8(void)  { return emu6502_read(cpu.pc++); }
static inline uint16_t fetch16(void)
{
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();
    return (uint16_t)(lo | (hi << 8));
}

#define SET_NZ(v)  do { \
    cpu.p = (cpu.p & ~(FLAG_N|FLAG_Z)) \
          | ((v) & 0x80 ? FLAG_N : 0)  \
          | ((v) == 0   ? FLAG_Z : 0); \
} while(0)

#define PUSH(v)   emu6502_write(0x0100 | cpu.sp--, (v))
#define POP()     emu6502_read(0x0100 | ++cpu.sp)

/* Addressing mode helpers */
static inline uint16_t addr_zp(void)   { return fetch8(); }
static inline uint16_t addr_zpx(void)  { return (fetch8() + cpu.x) & 0xFF; }
static inline uint16_t addr_zpy(void)  { return (fetch8() + cpu.y) & 0xFF; }
static inline uint16_t addr_abs(void)  { return fetch16(); }
static inline uint16_t addr_absx(void) { return fetch16() + cpu.x; }
static inline uint16_t addr_absy(void) { return fetch16() + cpu.y; }
static inline uint16_t addr_indx(void)
{
    uint8_t zp = (fetch8() + cpu.x) & 0xFF;
    return emu6502_read(zp) | ((uint16_t)emu6502_read((zp+1)&0xFF) << 8);
}
static inline uint16_t addr_indy(void)
{
    uint8_t zp = fetch8();
    uint16_t base = emu6502_read(zp) | ((uint16_t)emu6502_read((zp+1)&0xFF) << 8);
    return base + cpu.y;
}

/* ADC helper */
static void do_adc(uint8_t m)
{
    uint16_t r = cpu.a + m + (cpu.p & FLAG_C ? 1 : 0);
    cpu.p &= ~(FLAG_C|FLAG_V|FLAG_N|FLAG_Z);
    if (r > 0xFF)           cpu.p |= FLAG_C;
    if (!((cpu.a ^ m) & 0x80) && ((cpu.a ^ r) & 0x80)) cpu.p |= FLAG_V;
    cpu.a = r & 0xFF;
    SET_NZ(cpu.a);
}

/* SBC helper */
static void do_sbc(uint8_t m)
{
    do_adc(m ^ 0xFF);
}

/* CMP helper */
static void do_cmp(uint8_t reg, uint8_t m)
{
    uint8_t r = reg - m;
    cpu.p = (cpu.p & ~(FLAG_C|FLAG_N|FLAG_Z))
          | (reg >= m ? FLAG_C : 0)
          | (r == 0   ? FLAG_Z : 0)
          | (r & 0x80 ? FLAG_N : 0);
}

/* Branch helper */
static int do_branch(bool cond)
{
    int8_t off = (int8_t)fetch8();
    if (cond) {
        uint16_t old = cpu.pc;
        cpu.pc += off;
        return ((cpu.pc ^ old) & 0xFF00) ? 4 : 3;
    }
    return 2;
}

/* ---- Main step --------------------------------------------------- */
int emu6502_step(void)
{
    uint8_t op = fetch8();
    uint8_t m, tmp;
    uint16_t addr;
    int cyc = 2;

    switch (op) {
    /* ----- LDA ----- */
    case 0xA9: cpu.a=fetch8();            SET_NZ(cpu.a); cyc=2; break;
    case 0xA5: cpu.a=emu6502_read(addr_zp());  SET_NZ(cpu.a); cyc=3; break;
    case 0xB5: cpu.a=emu6502_read(addr_zpx()); SET_NZ(cpu.a); cyc=4; break;
    case 0xAD: cpu.a=emu6502_read(addr_abs()); SET_NZ(cpu.a); cyc=4; break;
    case 0xBD: cpu.a=emu6502_read(addr_absx());SET_NZ(cpu.a); cyc=4; break;
    case 0xB9: cpu.a=emu6502_read(addr_absy());SET_NZ(cpu.a); cyc=4; break;
    case 0xA1: cpu.a=emu6502_read(addr_indx());SET_NZ(cpu.a); cyc=6; break;
    case 0xB1: cpu.a=emu6502_read(addr_indy());SET_NZ(cpu.a); cyc=5; break;
    /* ----- LDX ----- */
    case 0xA2: cpu.x=fetch8();            SET_NZ(cpu.x); cyc=2; break;
    case 0xA6: cpu.x=emu6502_read(addr_zp());  SET_NZ(cpu.x); cyc=3; break;
    case 0xB6: cpu.x=emu6502_read(addr_zpy()); SET_NZ(cpu.x); cyc=4; break;
    case 0xAE: cpu.x=emu6502_read(addr_abs()); SET_NZ(cpu.x); cyc=4; break;
    case 0xBE: cpu.x=emu6502_read(addr_absy());SET_NZ(cpu.x); cyc=4; break;
    /* ----- LDY ----- */
    case 0xA0: cpu.y=fetch8();            SET_NZ(cpu.y); cyc=2; break;
    case 0xA4: cpu.y=emu6502_read(addr_zp());  SET_NZ(cpu.y); cyc=3; break;
    case 0xB4: cpu.y=emu6502_read(addr_zpx()); SET_NZ(cpu.y); cyc=4; break;
    case 0xAC: cpu.y=emu6502_read(addr_abs()); SET_NZ(cpu.y); cyc=4; break;
    case 0xBC: cpu.y=emu6502_read(addr_absx());SET_NZ(cpu.y); cyc=4; break;
    /* ----- STA ----- */
    case 0x85: emu6502_write(addr_zp(),  cpu.a); cyc=3; break;
    case 0x95: emu6502_write(addr_zpx(), cpu.a); cyc=4; break;
    case 0x8D: emu6502_write(addr_abs(), cpu.a); cyc=4; break;
    case 0x9D: emu6502_write(addr_absx(),cpu.a); cyc=5; break;
    case 0x99: emu6502_write(addr_absy(),cpu.a); cyc=5; break;
    case 0x81: emu6502_write(addr_indx(),cpu.a); cyc=6; break;
    case 0x91: emu6502_write(addr_indy(),cpu.a); cyc=6; break;
    /* ----- STX ----- */
    case 0x86: emu6502_write(addr_zp(),  cpu.x); cyc=3; break;
    case 0x96: emu6502_write(addr_zpy(), cpu.x); cyc=4; break;
    case 0x8E: emu6502_write(addr_abs(), cpu.x); cyc=4; break;
    /* ----- STY ----- */
    case 0x84: emu6502_write(addr_zp(),  cpu.y); cyc=3; break;
    case 0x94: emu6502_write(addr_zpx(), cpu.y); cyc=4; break;
    case 0x8C: emu6502_write(addr_abs(), cpu.y); cyc=4; break;
    /* ----- Transfer ----- */
    case 0xAA: cpu.x=cpu.a; SET_NZ(cpu.x); break;
    case 0xA8: cpu.y=cpu.a; SET_NZ(cpu.y); break;
    case 0x8A: cpu.a=cpu.x; SET_NZ(cpu.a); break;
    case 0x98: cpu.a=cpu.y; SET_NZ(cpu.a); break;
    case 0x9A: cpu.sp=cpu.x; break;
    case 0xBA: cpu.x=cpu.sp; SET_NZ(cpu.x); break;
    /* ----- Stack ----- */
    case 0x48: PUSH(cpu.a); cyc=3; break;
    case 0x68: cpu.a=POP(); SET_NZ(cpu.a); cyc=4; break;
    case 0x08: PUSH(cpu.p|0x30); cyc=3; break;
    case 0x28: cpu.p=POP(); cyc=4; break;
    /* ----- ADC / SBC ----- */
    case 0x69: do_adc(fetch8()); cyc=2; break;
    case 0x65: do_adc(emu6502_read(addr_zp()));  cyc=3; break;
    case 0x75: do_adc(emu6502_read(addr_zpx())); cyc=4; break;
    case 0x6D: do_adc(emu6502_read(addr_abs())); cyc=4; break;
    case 0x7D: do_adc(emu6502_read(addr_absx()));cyc=4; break;
    case 0x79: do_adc(emu6502_read(addr_absy()));cyc=4; break;
    case 0xE9: do_sbc(fetch8()); cyc=2; break;
    case 0xE5: do_sbc(emu6502_read(addr_zp()));  cyc=3; break;
    case 0xF5: do_sbc(emu6502_read(addr_zpx())); cyc=4; break;
    case 0xED: do_sbc(emu6502_read(addr_abs())); cyc=4; break;
    case 0xFD: do_sbc(emu6502_read(addr_absx()));cyc=4; break;
    case 0xF9: do_sbc(emu6502_read(addr_absy()));cyc=4; break;
    /* ----- AND ----- */
    case 0x29: cpu.a &= fetch8();             SET_NZ(cpu.a); cyc=2; break;
    case 0x25: cpu.a &= emu6502_read(addr_zp());  SET_NZ(cpu.a); cyc=3; break;
    case 0x35: cpu.a &= emu6502_read(addr_zpx()); SET_NZ(cpu.a); cyc=4; break;
    case 0x2D: cpu.a &= emu6502_read(addr_abs()); SET_NZ(cpu.a); cyc=4; break;
    case 0x3D: cpu.a &= emu6502_read(addr_absx());SET_NZ(cpu.a); cyc=4; break;
    case 0x39: cpu.a &= emu6502_read(addr_absy());SET_NZ(cpu.a); cyc=4; break;
    /* ----- ORA ----- */
    case 0x09: cpu.a |= fetch8();             SET_NZ(cpu.a); cyc=2; break;
    case 0x05: cpu.a |= emu6502_read(addr_zp());  SET_NZ(cpu.a); cyc=3; break;
    case 0x15: cpu.a |= emu6502_read(addr_zpx()); SET_NZ(cpu.a); cyc=4; break;
    case 0x0D: cpu.a |= emu6502_read(addr_abs()); SET_NZ(cpu.a); cyc=4; break;
    case 0x1D: cpu.a |= emu6502_read(addr_absx());SET_NZ(cpu.a); cyc=4; break;
    case 0x19: cpu.a |= emu6502_read(addr_absy());cyc=4; break;
    /* ----- EOR ----- */
    case 0x49: cpu.a ^= fetch8();             SET_NZ(cpu.a); cyc=2; break;
    case 0x45: cpu.a ^= emu6502_read(addr_zp());  SET_NZ(cpu.a); cyc=3; break;
    case 0x55: cpu.a ^= emu6502_read(addr_zpx()); SET_NZ(cpu.a); cyc=4; break;
    case 0x4D: cpu.a ^= emu6502_read(addr_abs()); SET_NZ(cpu.a); cyc=4; break;
    case 0x5D: cpu.a ^= emu6502_read(addr_absx());SET_NZ(cpu.a); cyc=4; break;
    case 0x59: cpu.a ^= emu6502_read(addr_absy());SET_NZ(cpu.a); cyc=4; break;
    /* ----- CMP ----- */
    case 0xC9: do_cmp(cpu.a,fetch8()); cyc=2; break;
    case 0xC5: do_cmp(cpu.a,emu6502_read(addr_zp()));  cyc=3; break;
    case 0xD5: do_cmp(cpu.a,emu6502_read(addr_zpx())); cyc=4; break;
    case 0xCD: do_cmp(cpu.a,emu6502_read(addr_abs())); cyc=4; break;
    case 0xDD: do_cmp(cpu.a,emu6502_read(addr_absx()));cyc=4; break;
    case 0xD9: do_cmp(cpu.a,emu6502_read(addr_absy()));cyc=4; break;
    case 0xE0: do_cmp(cpu.x,fetch8()); cyc=2; break;
    case 0xE4: do_cmp(cpu.x,emu6502_read(addr_zp())); cyc=3; break;
    case 0xEC: do_cmp(cpu.x,emu6502_read(addr_abs()));cyc=4; break;
    case 0xC0: do_cmp(cpu.y,fetch8()); cyc=2; break;
    case 0xC4: do_cmp(cpu.y,emu6502_read(addr_zp())); cyc=3; break;
    case 0xCC: do_cmp(cpu.y,emu6502_read(addr_abs()));cyc=4; break;
    /* ----- INC/DEC ----- */
    case 0xE8: cpu.x++; SET_NZ(cpu.x); break;
    case 0xC8: cpu.y++; SET_NZ(cpu.y); break;
    case 0xCA: cpu.x--; SET_NZ(cpu.x); break;
    case 0x88: cpu.y--; SET_NZ(cpu.y); break;
    case 0xE6: addr=addr_zp();  tmp=emu6502_read(addr)+1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=5; break;
    case 0xF6: addr=addr_zpx(); tmp=emu6502_read(addr)+1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=6; break;
    case 0xEE: addr=addr_abs(); tmp=emu6502_read(addr)+1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=6; break;
    case 0xFE: addr=addr_absx();tmp=emu6502_read(addr)+1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=7; break;
    case 0xC6: addr=addr_zp();  tmp=emu6502_read(addr)-1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=5; break;
    case 0xD6: addr=addr_zpx(); tmp=emu6502_read(addr)-1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=6; break;
    case 0xCE: addr=addr_abs(); tmp=emu6502_read(addr)-1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=6; break;
    case 0xDE: addr=addr_absx();tmp=emu6502_read(addr)-1; emu6502_write(addr,tmp); SET_NZ(tmp); cyc=7; break;
    /* ----- Shifts ----- */
    case 0x0A: cpu.p=(cpu.p&~FLAG_C)|(cpu.a>>7?FLAG_C:0); cpu.a<<=1; SET_NZ(cpu.a); break;
    case 0x4A: cpu.p=(cpu.p&~FLAG_C)|(cpu.a&1?FLAG_C:0);  cpu.a>>=1; SET_NZ(cpu.a); break;
    case 0x2A: tmp=(cpu.p&FLAG_C)?1:0; cpu.p=(cpu.p&~FLAG_C)|(cpu.a>>7?FLAG_C:0); cpu.a=(cpu.a<<1)|tmp; SET_NZ(cpu.a); break;
    case 0x6A: tmp=(cpu.p&FLAG_C)?0x80:0; cpu.p=(cpu.p&~FLAG_C)|(cpu.a&1?FLAG_C:0); cpu.a=(cpu.a>>1)|tmp; SET_NZ(cpu.a); break;
    /* ----- BIT ----- */
    case 0x24: m=emu6502_read(addr_zp());  cpu.p=(cpu.p&~(FLAG_N|FLAG_V|FLAG_Z))|(m&0xC0)|((cpu.a&m)?0:FLAG_Z); cyc=3; break;
    case 0x2C: m=emu6502_read(addr_abs()); cpu.p=(cpu.p&~(FLAG_N|FLAG_V|FLAG_Z))|(m&0xC0)|((cpu.a&m)?0:FLAG_Z); cyc=4; break;
    /* ----- Branches ----- */
    case 0x90: cyc=do_branch(!(cpu.p&FLAG_C)); break;
    case 0xB0: cyc=do_branch( (cpu.p&FLAG_C)); break;
    case 0xF0: cyc=do_branch( (cpu.p&FLAG_Z)); break;
    case 0xD0: cyc=do_branch(!(cpu.p&FLAG_Z)); break;
    case 0x30: cyc=do_branch( (cpu.p&FLAG_N)); break;
    case 0x10: cyc=do_branch(!(cpu.p&FLAG_N)); break;
    case 0x70: cyc=do_branch( (cpu.p&FLAG_V)); break;
    case 0x50: cyc=do_branch(!(cpu.p&FLAG_V)); break;
    /* ----- Jump / Call ----- */
    case 0x4C: cpu.pc=fetch16(); cyc=3; break;
    case 0x6C: { uint16_t ptr=fetch16(); cpu.pc=emu6502_read(ptr)|(emu6502_read((ptr&0xFF00)|((ptr+1)&0xFF))<<8); cyc=5; } break;
    case 0x20: addr=fetch16(); PUSH((cpu.pc-1)>>8); PUSH((cpu.pc-1)&0xFF); cpu.pc=addr; cyc=6; break;
    case 0x60: cpu.pc=(POP()|(POP()<<8))+1; cyc=6; break;
    case 0x40: cpu.p=POP(); cpu.pc=POP()|(POP()<<8); cyc=6; break;
    /* ----- Flags ----- */
    case 0x18: cpu.p &= ~FLAG_C; break;
    case 0x38: cpu.p |=  FLAG_C; break;
    case 0x58: cpu.p &= ~FLAG_I; break;
    case 0x78: cpu.p |=  FLAG_I; break;
    case 0xB8: cpu.p &= ~FLAG_V; break;
    case 0xD8: cpu.p &= ~FLAG_D; break;
    case 0xF8: cpu.p |=  FLAG_D; break;
    /* ----- NOP ----- */
    case 0xEA: break;
    /* ----- BRK ----- */
    case 0x00:
        PUSH(cpu.pc>>8); PUSH(cpu.pc&0xFF); PUSH(cpu.p|FLAG_B);
        cpu.pc = emu6502_read(0xFFFE)|(emu6502_read(0xFFFF)<<8);
        cpu.p |= FLAG_I; cyc=7; break;
    default:
        /* Illegal opcode - just skip */
        break;
    }

    cpu.cycles += cyc;
    return cyc;
}

/* ---- Public API -------------------------------------------------- */
void emu6502_init(const uint8_t *rom, uint32_t rom_size)
{
    memset(ram, 0, sizeof(ram));
    memset(&cpu, 0, sizeof(cpu));
    if (rom_size == 4096)
        memcpy(rom_buf, rom, 4096);
    riot_init();
    emu6502_reset();
}

void emu6502_reset(void)
{
    cpu.pc = emu6502_read(0xFFFC) | ((uint16_t)emu6502_read(0xFFFD) << 8);
    cpu.sp = 0xFF;
    cpu.p  = FLAG_I;
}

void emu6502_run_frame(void)
{
    int cycles = 0;
    while (cycles < CYCLES_PER_FRAME)
        cycles += emu6502_step();
}

CPU6502 *emu6502_get_state(void) { return &cpu; }

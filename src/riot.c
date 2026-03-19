/*
 * riot.c  —  MOS 6532 RIOT emulation
 */

#include "riot.h"
#include <string.h>

uint8_t riot_swcha = 0xFF;   /* all directions released */
uint8_t riot_swchb = 0xFF;   /* console switches default */

static uint8_t  riot_ram[128];
static uint32_t timer_val   = 0;
static uint32_t timer_div   = 1;
static int      timer_acc   = 0;

void riot_init(void)
{
    memset(riot_ram, 0, sizeof(riot_ram));
    timer_val = 0xFF;
    timer_div = 1024;
}

void riot_write(uint16_t addr, uint8_t val)
{
    /* RAM: A7=1, A9=0 */
    if (!(addr & 0x200)) {
        riot_ram[addr & 0x7F] = val;
        return;
    }
    /* Timer writes */
    switch (addr & 0x1F) {
    case 0x14: timer_val = val; timer_div =    1; timer_acc = 0; break;
    case 0x15: timer_val = val; timer_div =    8; timer_acc = 0; break;
    case 0x16: timer_val = val; timer_div =   64; timer_acc = 0; break;
    case 0x17: timer_val = val; timer_div = 1024; timer_acc = 0; break;
    default: break;
    }
}

uint8_t riot_read(uint16_t addr)
{
    if (!(addr & 0x200))
        return riot_ram[addr & 0x7F];

    switch (addr & 0x1F) {
    case 0x00: return riot_swcha;
    case 0x02: return riot_swchb;
    case 0x04: return (uint8_t)(timer_val & 0xFF);  /* INTIM */
    default:   return 0;
    }
}

void riot_tick(int cycles)
{
    timer_acc += cycles;
    while (timer_acc >= (int)timer_div) {
        timer_acc -= timer_div;
        if (timer_val > 0) timer_val--;
    }
}

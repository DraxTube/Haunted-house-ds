/*
 * tia.c  —  TIA emulation (write-side + state)
 *
 * Full pixel-exact TIA emulation is very complex.
 * This implementation tracks register state accurately and
 * hands it to video.c for per-scanline rendering on the DS.
 */

#include "tia.h"
#include <string.h>

static TIAState tia;

void tia_init(void)
{
    memset(&tia, 0, sizeof(tia));
}

void tia_write(uint8_t reg, uint8_t val)
{
    switch (reg) {
    case TIA_VSYNC:  tia.vsync   = (val & 0x02) ? 1 : 0; break;
    case TIA_WSYNC:  /* CPU waits for end of scanline — handled in run_frame */ break;
    case TIA_COLUBK: tia.colubk  = val; break;
    case TIA_COLUPF: tia.colupf  = val; break;
    case TIA_COLUP0: tia.colup0  = val; break;
    case TIA_COLUP1: tia.colup1  = val; break;
    case TIA_PF0:    tia.pf0     = val; break;
    case TIA_PF1:    tia.pf1     = val; break;
    case TIA_PF2:    tia.pf2     = val; break;
    case TIA_GRP0:   tia.grp0    = val; break;
    case TIA_GRP1:   tia.grp1    = val; break;
    case TIA_CTRLPF: tia.ctrlpf  = val; break;
    case TIA_AUDC0:  tia.audc0   = val & 0x0F; break;
    case TIA_AUDC1:  tia.audc1   = val & 0x0F; break;
    case TIA_AUDF0:  tia.audf0   = val & 0x1F; break;
    case TIA_AUDF1:  tia.audf1   = val & 0x1F; break;
    case TIA_AUDV0:  tia.audv0   = val & 0x0F; break;
    case TIA_AUDV1:  tia.audv1   = val & 0x0F; break;
    case TIA_RESP0:  tia.x_p0    = tia.scanline & 0xFF; break;  /* simplified */
    case TIA_RESP1:  tia.x_p1    = tia.scanline & 0xFF; break;
    default: break;
    }
}

uint8_t tia_read(uint16_t addr)
{
    /* TIA read-back (collision + input ports) - simplified */
    (void)addr;
    return 0;
}

TIAState *tia_get_state(void) { return &tia; }

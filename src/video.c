/*
 * video.c  —  DS video rendering
 *
 * The TIA produces a 160×192 pixel NTSC frame (160 visible pixels × 192 scanlines).
 * We scale it to fill the DS top screen (256×192) using a simple 1.6× horizontal stretch.
 *
 * Atari colour palette: 128 entries (bits 7-1 of the colour byte).
 * Each entry is mapped to a 15-bit BGR555 DS colour.
 */

#include "video.h"
#include "tia.h"
#include <nds.h>
#include <string.h>

/* ---- forward declarations ---------------------------------------- */
static void blit_to_ds(void);
static void render_scanline(int line, TIAState *t);

/* ---- Atari 2600 NTSC palette -> BGR555 -------------------------- */
static const uint16_t atari_palette[128] = {
    /* Grey scale */
    RGB15( 0, 0, 0), RGB15( 4, 4, 4), RGB15( 8, 8, 8), RGB15(12,12,12),
    RGB15(16,16,16), RGB15(20,20,20), RGB15(24,24,24), RGB15(28,28,28),
    /* Gold */
    RGB15(16, 8, 0), RGB15(20,12, 0), RGB15(24,16, 0), RGB15(28,20, 0),
    RGB15(31,24, 4), RGB15(31,26, 8), RGB15(31,28,12), RGB15(31,30,16),
    /* Orange */
    RGB15(20, 4, 0), RGB15(24, 8, 0), RGB15(28,12, 0), RGB15(31,16, 0),
    RGB15(31,20, 4), RGB15(31,22, 8), RGB15(31,24,12), RGB15(31,26,16),
    /* Red-Orange */
    RGB15(20, 0, 0), RGB15(24, 4, 0), RGB15(28, 8, 0), RGB15(31,12, 0),
    RGB15(31,16, 0), RGB15(31,18, 4), RGB15(31,20, 8), RGB15(31,22,12),
    /* Pink */
    RGB15(18, 0, 8), RGB15(22, 4,12), RGB15(26, 8,16), RGB15(30,12,20),
    RGB15(31,16,24), RGB15(31,20,26), RGB15(31,22,28), RGB15(31,24,30),
    /* Purple */
    RGB15(12, 0,18), RGB15(16, 0,22), RGB15(20, 4,26), RGB15(24, 8,30),
    RGB15(28,12,31), RGB15(30,16,31), RGB15(31,20,31), RGB15(31,24,31),
    /* Purple-Blue */
    RGB15( 4, 0,20), RGB15( 8, 0,24), RGB15(12, 4,28), RGB15(16, 8,31),
    RGB15(20,12,31), RGB15(24,16,31), RGB15(28,20,31), RGB15(31,24,31),
    /* Blue */
    RGB15( 0, 0,20), RGB15( 0, 4,24), RGB15( 4, 8,28), RGB15( 8,12,31),
    RGB15(12,16,31), RGB15(16,20,31), RGB15(20,24,31), RGB15(24,28,31),
    /* Light blue */
    RGB15( 0, 8,20), RGB15( 0,12,24), RGB15( 4,16,28), RGB15( 8,20,31),
    RGB15(12,24,31), RGB15(16,28,31), RGB15(20,30,31), RGB15(24,31,31),
    /* Teal */
    RGB15( 0,14,14), RGB15( 0,18,18), RGB15( 4,22,22), RGB15( 8,26,26),
    RGB15(12,30,30), RGB15(16,31,31), RGB15(20,31,31), RGB15(24,31,31),
    /* Green-Blue */
    RGB15( 0,16, 8), RGB15( 0,20,12), RGB15( 4,24,16), RGB15( 8,28,20),
    RGB15(12,31,24), RGB15(16,31,26), RGB15(20,31,28), RGB15(24,31,30),
    /* Green */
    RGB15( 0,16, 0), RGB15( 0,20, 0), RGB15( 4,24, 4), RGB15( 8,28, 8),
    RGB15(12,31,12), RGB15(16,31,16), RGB15(20,31,20), RGB15(24,31,24),
    /* Yellow-Green */
    RGB15( 8,16, 0), RGB15(12,20, 0), RGB15(16,24, 4), RGB15(20,28, 8),
    RGB15(24,31,12), RGB15(26,31,16), RGB15(28,31,20), RGB15(30,31,24),
    /* Orange-Green */
    RGB15(16,14, 0), RGB15(20,18, 0), RGB15(24,22, 0), RGB15(28,26, 0),
    RGB15(31,30, 4), RGB15(31,31, 8), RGB15(31,31,12), RGB15(31,31,16),
    /* Light Orange */
    RGB15(22,10, 0), RGB15(26,14, 0), RGB15(30,18, 0), RGB15(31,22, 4),
    RGB15(31,26, 8), RGB15(31,28,12), RGB15(31,30,16), RGB15(31,31,20),
    /* White */
    RGB15(28,28,28), RGB15(29,29,29), RGB15(30,30,30), RGB15(31,31,31),
    RGB15(31,31,31), RGB15(31,31,31), RGB15(31,31,31), RGB15(31,31,31),
};

/* Convert a TIA colour byte to BGR555 */
static inline uint16_t tia_colour(uint8_t c)
{
    return atari_palette[(c >> 1) & 0x7F];
}

/* Draw one scanline into framebuf */
static void render_scanline(int line, TIAState *t)
{
    uint16_t bg = tia_colour(t->colubk);
    uint16_t pf = tia_colour(t->colupf);
    uint16_t p0 = tia_colour(t->colup0);
    uint16_t p1 = tia_colour(t->colup1);

    uint16_t *row = framebuf + line * ATARI_W;

    for (int x = 0; x < ATARI_W; x++) row[x] = bg;

    uint32_t pf_bits = ((t->pf0 >> 4) & 0xF) |
                       ((uint32_t)t->pf1 << 4) |
                       ((uint32_t)t->pf2 << 12);

    for (int bit = 0; bit < 20; bit++) {
        if (pf_bits & (1u << bit)) {
            int x = bit * 4;
            for (int i = 0; i < 4 && x+i < ATARI_W/2; i++) row[x+i] = pf;
        }
    }
    bool mirror = (t->ctrlpf & 0x01);
    for (int x = ATARI_W/2; x < ATARI_W; x++) {
        int src = mirror ? (ATARI_W - 1 - x) : (x - ATARI_W/2);
        row[x] = row[src];
    }

    uint8_t grp = t->grp0;
    int px0 = t->x_p0 & (ATARI_W-1);
    for (int b = 0; b < 8; b++) {
        if (grp & (0x80 >> b)) {
            int x = px0 + b;
            if (x < ATARI_W) row[x] = p0;
        }
    }

    grp = t->grp1;
    int px1 = t->x_p1 & (ATARI_W-1);
    for (int b = 0; b < 8; b++) {
        if (grp & (0x80 >> b)) {
            int x = px1 + b;
            if (x < ATARI_W) row[x] = p1;
        }
    }
}

#define ATARI_W  160
#define ATARI_H  192
static uint16_t framebuf[ATARI_W * ATARI_H];

static int main_bg = -1;

/* ------------------------------------------------------------------ */
void video_init(void)
{
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);

    main_bg = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    bgSetPriority(main_bg, 0);
    bgShow(main_bg);

    videoSetModeSub(MODE_5_2D);
    vramSetBankC(VRAM_C_SUB_BG);
    bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    dmaFillHalfWords(0, bgGetGfxPtr(main_bg), 256*192*2);

    swiWaitForVBlank();
    swiWaitForVBlank();
}

/* ------------------------------------------------------------------ */
void video_render(void)
{
    TIAState *t = tia_get_state();

    for (int line = 0; line < ATARI_H; line++)
        render_scanline(line, t);

    blit_to_ds();
}

static void blit_to_ds(void)
{
    uint16_t *dst = bgGetGfxPtr(main_bg);

    for (int y = 0; y < ATARI_H; y++) {
        uint16_t *src = framebuf + y * ATARI_W;
        uint16_t *row = dst + y * 256;
        for (int x = 0; x < 256; x++) {
            row[x] = src[(x * ATARI_W) / 256];
        }
    }
}

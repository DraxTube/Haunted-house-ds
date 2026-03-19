/*
 * Haunted House DS - Nintendo DS Port
 * Uses the original Atari 2600 ROM via an embedded 6502 emulator.
 *
 * Place your ROM at:  fat:/data/haunted_house.a26
 *
 * ROM is NOT included - user must provide their own copy.
 */

#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emu6502.h"
#include "tia.h"
#include "video.h"
#include "input.h"

/* ---- ROM path on the SD card ---- */
#define ROM_PATH   "fat:/data/haunted_house.a26"
#define ROM_SIZE   4096   /* Haunted House is a 4 KB ROM */

static uint8_t rom_data[ROM_SIZE];

/* ------------------------------------------------------------------ */
static bool load_rom(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size != ROM_SIZE) {
        fclose(f);
        return false;
    }

    fread(rom_data, 1, ROM_SIZE, f);
    fclose(f);
    return true;
}

/* ------------------------------------------------------------------ */
static void show_error(const char *msg)
{
    consoleDemoInit();
    iprintf("\n\n  Haunted House DS\n\n");
    iprintf("  ERROR:\n  %s\n\n", msg);
    iprintf("  Place the ROM at:\n");
    iprintf("  /data/haunted_house.a26\n\n");
    iprintf("  Press START to exit.\n");

    while (1) {
        swiWaitForVBlank();
        scanKeys();
        if (keysDown() & KEY_START) break;
    }
}

/* ------------------------------------------------------------------ */
int main(void)
{
    /* Basic DS init */
    defaultExceptionHandler();

    /* Init FAT (SD card access) */
    if (!fatInitDefault()) {
        show_error("FAT init failed.\nNo SD card?");
        return 1;
    }

    /* Load ROM from SD */
    if (!load_rom(ROM_PATH)) {
        show_error("ROM not found or wrong size.\nExpected 4096 bytes.");
        return 1;
    }

    /* Init subsystems */
    video_init();   /* Set up DS screens / tile engine */
    tia_init();     /* TIA (Atari graphics/sound chip) emulation */
    input_init();   /* Map DS buttons -> Atari joystick + console switches */

    /* Init 6502 core and map the ROM */
    emu6502_init(rom_data, ROM_SIZE);

    /* Main emulation loop */
    while (pmMainLoop()) {
        /* --- Input --- */
        scanKeys();
        input_update();

        /* --- Run ~1 frame worth of 6502 cycles (roughly 19950 cycles/frame) --- */
        emu6502_run_frame();

        /* --- Render TIA output to DS screens --- */
        video_render();

        /* --- Wait for VBlank --- */
        swiWaitForVBlank();
    }

    return 0;
}

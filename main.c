
#include <3ds.h>
#include <stdio.h>
#include <stdint.h>
#include "sunset_bg.h"


static void draw_sunset(uint8_t *fb, int w, int h) {
    if (!fb) return;

    int draw_w = w < SUNSET_BG_W ? w : SUNSET_BG_W;
    int draw_h = h < SUNSET_BG_H ? h : SUNSET_BG_H;

    for (int y = 0; y < draw_h; y++) {
        for (int x = 0; x < draw_w; x++) {
            /* Источник: обычная картинка, строка за строкой */
            uint16_t c = sunset_bg[y * SUNSET_BG_W + x];

            unsigned r = (c >> 11) & 31;
            unsigned g = (c >> 5) & 63;
            unsigned b = c & 31;

            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;

            /*
             * 3DS framebuffer хранит пиксели в порядке,
             * где соседние пиксели по X разделены высотой.
             */
            unsigned char *p =
                fb + 3 * (x * h + (h - 1 - y));

            p[0] = (uint8_t)b;
            p[1] = (uint8_t)g;
            p[2] = (uint8_t)r;
        }
    }
}

int main(void) {
    gfxInitDefault();

    PrintConsole bottom;
    consoleInit(GFX_BOTTOM, &bottom);

    const char *items[] = {
        "HOME", "GARAGE", "CAR SHOP", "RACE", "SETTINGS"
    };

    int selected = 0;

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_START) break;

        if (down & KEY_DOWN)
            selected = (selected + 1) % 5;

        if (down & KEY_UP)
            selected = (selected + 4) % 5;

        if (down & KEY_A) {
            consoleSelect(&bottom);
            consoleClear();
            printf("\x1b[3;7H%s", items[selected]);
            printf("\x1b[5;3HComing soon!");
            printf("\x1b[21;3HB: BACK");
            printf("\x1b[22;3HSTART: EXIT");
        } else {
            consoleSelect(&bottom);
            consoleClear();
            printf("\x1b[1;5HCOASTAL RACER 3DS");
            printf("\x1b[3;8HSUNSET EDITION");

            for (int i = 0; i < 5; i++) {
                printf("\x1b[%d;7H%s %s",
                       7 + i * 2,
                       i == selected ? ">" : " ",
                       items[i]);
            }

            printf("\x1b[19;3HUP/DOWN: SELECT");
            printf("\x1b[21;3HA: CONFIRM");
            printf("\x1b[22;3HSTART: EXIT");
        }

        uint16_t w, h;
        uint8_t *fb = gfxGetFramebuffer(
            GFX_TOP, GFX_LEFT, &w, &h
        );

        draw_sunset(fb, w, h);

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}

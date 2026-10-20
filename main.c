
#include <3ds.h>
#include <stdio.h>
#include <string.h>

#define ITEM_COUNT 5

static const char *items[ITEM_COUNT] = {
    "Career",
    "Garage",
    "Car Shop",
    "Tracks",
    "Settings"
};

static void draw_top(PrintConsole *top) {
    consoleSelect(top);
    consoleClear();
    printf("\x1b[8;12H\x1b[36mCOASTAL RACER 3DS\x1b[0m");
    printf("\x1b[11;14HPress START to exit");
}

static void draw_menu(PrintConsole *bottom, int sel,
                      const char *msg) {
    consoleSelect(bottom);
    consoleClear();

    printf("\x1b[2;10H\x1b[33m== MAIN MENU ==\x1b[0m");

    for (int i = 0; i < ITEM_COUNT; i++) {
        printf("\x1b[%d;8H%s %s",
               6 + i * 2,
               i == sel ? ">" : " ",
               items[i]);
    }

    if (msg && msg[0])
        printf("\x1b[22;2H%s", msg);
}

static void draw_garage(PrintConsole *bottom) {
    consoleSelect(bottom);
    consoleClear();

    printf("\x1b[2;12H\x1b[33m== GARAGE ==\x1b[0m");
    printf("\x1b[5;5HYour racing car");
    printf("\x1b[8;4HCoastal Racer");
    printf("\x1b[10;4H3D model: next step!");
    printf("\x1b[13;4HCar preview is coming");
    printf("\x1b[17;4HA - Select car");
    printf("\x1b[19;4HB - Back to menu");
}

int main(int argc, char **argv) {
    gfxInitDefault();

    PrintConsole top, bottom;
    consoleInit(GFX_TOP, &top);
    consoleInit(GFX_BOTTOM, &bottom);

    int sel = 0;
    int inGarage = 0;
    char msg[48] = "";

    draw_top(&top);
    draw_menu(&bottom, sel, msg);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if (inGarage) {
            if (down & KEY_B) {
                inGarage = 0;
                msg[0] = '\0';
                draw_menu(&bottom, sel, msg);
            } else if (down & KEY_A) {
                consoleSelect(&bottom);
                printf("\x1b[21;2HCar selection coming soon!");
            }
        } else {
            int changed = 0;

            if (down & KEY_DOWN) {
                sel = (sel + 1) % ITEM_COUNT;
                changed = 1;
            }

            if (down & KEY_UP) {
                sel = (sel + ITEM_COUNT - 1) % ITEM_COUNT;
                changed = 1;
            }

            if (down & KEY_A) {
                if (sel == 1) {
                    inGarage = 1;
                    draw_garage(&bottom);
                } else {
                    snprintf(msg, sizeof(msg),
                             "%s: coming soon", items[sel]);
                    changed = 1;
                }
            }

            if (changed)
                draw_menu(&bottom, sel, msg);
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}

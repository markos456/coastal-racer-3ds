
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        printf("Citro3D init failed!\n");
        gfxExit();
        return 1;
    }

    C3D_RenderTarget *top =
        C3D_RenderTargetCreate(
            400, 240,
            GPU_RB_RGBA8,
            GPU_RB_DEPTH24
        );

    if (top == NULL) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        top, GFX_TOP, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS
    );

    printf("COASTAL RACER 3DS\n");
    printf("-----------------\n");
    printf("Citro3D is running!\n");
    printf("\n");
    printf("Next: our first car!\n");
    printf("\n");
    printf("Press START to exit.");

    while (aptMainLoop()) {
        hidScanInput();

        if (hidKeysDown() & KEY_START)
            break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            top, C3D_CLEAR_ALL,
            0x1680A0FF, 0
        );

        C3D_FrameDrawOn(top);
        C3D_FrameEnd(0);
    }

    C3D_RenderTargetDelete(top);
    C3D_Fini();
    gfxExit();

    return 0;
}

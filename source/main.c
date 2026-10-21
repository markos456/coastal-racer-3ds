#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

int main(void)
{
    gfxInitDefault();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    C3D_RenderTarget *top =
        C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget *bottom =
        C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!top || !bottom) {
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    const u32 sky = C2D_Color32(18, 35, 65, 255);
    const u32 cyan = C2D_Color32(0, 220, 255, 255);
    const u32 white = C2D_Color32(245, 245, 245, 255);
    const u32 orange = C2D_Color32(255, 145, 25, 255);
    const u32 dark = C2D_Color32(12, 20, 35, 255);
    const u32 green = C2D_Color32(30, 220, 120, 255);

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        // Верхний экран: тестовая заставка
        C2D_TargetClear(top, sky);
        C2D_SceneBegin(top);

        C2D_DrawRectSolid(20, 20, 0, 360, 8, cyan);
        C2D_DrawRectSolid(20, 42, 0, 220, 35, dark);
        C2D_DrawRectSolid(28, 50, 0, 204, 8, white);
        C2D_DrawRectSolid(28, 64, 0, 145, 6, orange);

        // Условный автомобиль из простых фигур
        C2D_DrawRectSolid(130, 135, 0, 140, 38, orange);
        C2D_DrawRectSolid(155, 115, 0, 80, 25, cyan);
        C2D_DrawRectSolid(145, 165, 0, 35, 12, dark);
        C2D_DrawRectSolid(225, 165, 0, 35, 12, dark);

        C2D_DrawRectSolid(20, 210, 0, 360, 5, white);

        // Нижний экран: тестовые кнопки меню
        C2D_TargetClear(bottom, dark);
        C2D_SceneBegin(bottom);

        C2D_DrawRectSolid(20, 20, 0, 280, 45, cyan);
        C2D_DrawRectSolid(20, 80, 0, 280, 45, orange);
        C2D_DrawRectSolid(20, 140, 0, 280, 45, green);
        C2D_DrawRectSolid(20, 205, 0, 280, 5, white);

        C3D_FrameEnd(0);
    }

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}

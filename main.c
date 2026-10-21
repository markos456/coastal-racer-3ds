
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>

#include "vshader_shbin.h"

#define PI 3.14159265f
#define MENU_COUNT 4

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | \
     GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef enum {
    SCREEN_MAIN,
    SCREEN_PLAY,
    SCREEN_SETTINGS,
    SCREEN_SAVES,
    SCREEN_EXIT
} MenuScreen;

static DVLB_s *dvlb = NULL;
static shaderProgram_s program;
static C3D_Mtx projection;
static C3D_Mtx modelView;
static C3D_RenderTarget *target = NULL;

static int uProjection = -1;
static int uModelView = -1;
static int sceneReady = 0;

static PrintConsole bottomScreen;
static MenuScreen screen = SCREEN_MAIN;
static int selected = 0;
static int exitChoice = 1;
static float roadOffset = 0.0f;

static const char *statusMessage = "WELCOME TO THE COAST!";

/* Send one vertex: position + color. */
static void vertex(float x, float y, float z,
                   float r, float g, float b)
{
    C3D_ImmSendAttrib(x, y, z, 1.0f);
    C3D_ImmSendAttrib(r, g, b, 1.0f);
}

static void triangle(
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz,
    float r, float g, float b)
{
    vertex(ax, ay, az, r, g, b);
    vertex(bx, by, bz, r, g, b);
    vertex(cx, cy, cz, r, g, b);
}

static void quad(
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz,
    float dx, float dy, float dz,
    float r, float g, float b)
{
    triangle(ax, ay, az, bx, by, bz,
             cx, cy, cz, r, g, b);

    triangle(ax, ay, az, cx, cy, cz,
             dx, dy, dz, r, g, b);
}

/* Axis-aligned box. */
static void box(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float r, float g, float b)
{
    float a = x1, c = x2;
    float d = y1, e = y2;
    float f = z1, h = z2;

    quad(a,d,f, c,d,f, c,e,f, a,e,f, r,g,b);
    quad(c,d,h, a,d,h, a,e,h, c,e,h, r,g,b);

    quad(a,d,h, a,d,f, a,e,f, a,e,h,
         r*0.65f, g*0.65f, b*0.65f);

    quad(c,d,f, c,d,h, c,e,h, c,e,f,
         r*0.8f, g*0.8f, b*0.8f);

    quad(a,e,f, c,e,f, c,e,h, a,e,h, r,g,b);

    quad(a,d,h, c,d,h, c,d,f, a,d,f,
         r*0.5f, g*0.5f, b*0.5f);
}

/* Sunset, sun and ocean. */
static void drawEnvironment(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Sky: layered colors above the horizon. */
    quad(-80, 15, -40, 80, 15, -40,
         80, 4, -40, -80, 4, -40,
         0.20f, 0.12f, 0.38f);

    quad(-80, 4, -40, 80, 4, -40,
         80, 2, -40, -80, 2, -40,
         0.48f, 0.18f, 0.42f);

    quad(-80, 2, -40, 80, 2, -40,
         80, 0, -40, -80, 0, -40,
         0.95f, 0.30f, 0.22f);

    quad(-80, 0, -40, 80, 0, -40,
         80, -1.2f, -40, -80, -1.2f, -40,
         1.0f, 0.56f, 0.30f);

    /* Sun, centered in the sunset. */
    for (int i = 0; i < 32; i++) {
        float a1 = (float)i * 2.0f * PI / 32.0f;
        float a2 = (float)(i + 1) * 2.0f * PI / 32.0f;

        triangle(
            -4.0f, 1.1f, -30.0f,
            -4.0f + cosf(a1) * 2.0f,
             1.1f + sinf(a1) * 2.0f, -30.0f,
            -4.0f + cosf(a2) * 2.0f,
             1.1f + sinf(a2) * 2.0f, -30.0f,
            1.0f, 0.72f, 0.32f);
    }

    /* Wide ocean plane below the horizon. */
    quad(-100, -1.20f, -40,
          100, -1.20f, -40,
          100, -1.20f, -2.0f,
         -100, -1.20f, -2.0f,
          0.035f, 0.23f, 0.40f);

    /* A few simple ocean highlights. */
    for (int i = 0; i < 12; i++) {
        float z = -25.0f + (float)i * 1.8f;

        quad(-9.0f, -1.17f, z,
             -7.5f, -1.17f, z,
             -7.5f, -1.17f, z - 0.06f,
             -9.0f, -1.17f, z - 0.06f,
             0.12f, 0.38f, 0.52f);

        quad(7.5f, -1.17f, z - 0.5f,
             9.0f, -1.17f, z - 0.5f,
             9.0f, -1.17f, z - 0.56f,
             7.5f, -1.17f, z - 0.56f,
             0.12f, 0.38f, 0.52f);
    }

    C3D_ImmDrawEnd();
}

/* Wide road, with perspective toward the horizon. */
static void drawRoad(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Asphalt. The near end appears wider because of perspective. */
    quad(-4.8f, -1.10f, -2.2f,
          4.8f, -1.10f, -2.2f,
          4.8f, -1.10f, -38.0f,
         -4.8f, -1.10f, -38.0f,
          0.14f, 0.15f, 0.17f);

    /* Left and right road edges. */
    quad(-4.8f, -1.085f, -2.2f,
         -4.45f, -1.085f, -2.2f,
         -4.45f, -1.085f, -38.0f,
         -4.8f, -1.085f, -38.0f,
          0.95f, 0.79f, 0.53f);

    quad(4.45f, -1.085f, -2.2f,
         4.8f, -1.085f, -2.2f,
         4.8f, -1.085f, -38.0f,
         4.45f, -1.085f, -38.0f,
          0.95f, 0.79f, 0.53f);

    /* Moving dashed center line. */
    for (int i = 0; i < 13; i++) {
        float z1 = -37.0f + (float)i * 3.0f + roadOffset;

        while (z1 > -2.4f)
            z1 -= 36.0f;

        float z2 = z1 + 1.35f;

        if (z2 > -2.2f)
            z2 = -2.2f;

        quad(-0.12f, -1.075f, z1,
              0.12f, -1.075f, z1,
              0.12f, -1.075f, z2,
             -0.12f, -1.075f, z2,
              1.0f, 0.86f, 0.58f);
    }

    C3D_ImmDrawEnd();
}

/* Simple red sports car, facing forward along the road. */
static void drawCar(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Main body. */
    box(-0.65f, -1.02f, -3.8f,
         0.65f, -0.72f, -2.2f,
         0.85f, 0.025f, 0.035f);

    /* Cabin and windshield. */
    box(-0.43f, -0.72f, -3.45f,
         0.43f, -0.42f, -2.65f,
         0.12f, 0.22f, 0.30f);

    /* Roof. */
    box(-0.34f, -0.42f, -3.30f,
         0.34f, -0.37f, -2.80f,
         0.80f, 0.025f, 0.035f);

    /* Four wheels. */
    box(-0.78f, -1.10f, -3.65f,
     -0.58f, -0.80f, -3.30f,
      0.035f, 0.035f, 0.045f);

    box(0.58f, -1.10f, -3.65f,
      0.78f, -0.80f, -3.30f,
      0.035f, 0.035f, 0.045f);

    box(-0.78f, -1.10f, -2.65f,
     -0.58f, -0.80f, -2.30f,
      0.035f, 0.035f, 0.045f);

    box(0.58f, -1.10f, -2.65f,
      0.78f, -0.80f, -2.30f,
      0.035f, 0.035f, 0.045f);

    /* Rear spoiler. */
    box(-0.58f, -0.68f, -3.88f,
         0.58f, -0.61f, -3.78f,
         0.12f, 0.12f, 0.13f);

    /* Front headlights. */
    box(-0.50f, -0.90f, -2.23f,
        -0.22f, -0.82f, -2.19f,
         1.0f, 0.85f, 0.45f);

    box(0.22f, -0.90f, -2.23f,
         0.50f, -0.82f, -2.19f,
         1.0f, 0.85f, 0.45f);

    C3D_ImmDrawEnd();
}

static int initScene(void)
{
    dvlb = DVLB_ParseFile((u32 *)vshader_shbin,
                          vshader_shbin_size);

    if (!dvlb)
        return 0;

    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    uProjection = shaderInstanceGetUniformLocation(
        program.vertexShader, "projection");

    uModelView = shaderInstanceGetUniformLocation(
        program.vertexShader, "modelView");

    if (uProjection < 0 || uModelView < 0) {
        shaderProgramFree(&program);
        DVLB_Free(dvlb);
        dvlb = NULL;
        return 0;
    }

    C3D_AttrInfo *attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    Mtx_PerspTilt(
        &projection,
        C3D_AngleFromDegrees(50.0f),
        C3D_AspectRatioTop,
        0.1f, 100.0f, false);

    Mtx_Identity(&modelView);

    sceneReady = 1;
    return 1;
}

static void printItem(int row, const char *label, int active)
{
    printf("\x1b[%d;4H", row);

    if (active)
        printf("> %s <", label);
    else
        printf("  %s", label);
}

static void drawMenu(void)
{
    consoleSelect(&bottomScreen);
    consoleClear();

    printf("\x1b[1;5HCOASTAL RACER 3DS");
    printf("\x1b[3;7HSUNSET COAST");

    switch (screen) {
    case SCREEN_MAIN:
        printItem(8,  "PLAY",     selected == 0);
        printItem(11, "SETTINGS", selected == 1);
        printItem(14, "SAVES",    selected == 2);
        printItem(17, "EXIT",     selected == 3);

        printf("\x1b[21;2HUP/DOWN: SELECT");
        printf("\x1b[22;2HA: CONFIRM");
        break;

    case SCREEN_PLAY:
        printf("\x1b[6;4HSELECT GAME MODE");
        printItem(10, "OFFLINE",     selected == 0);
        printItem(13, "MULTIPLAYER", selected == 1);
        printf("\x1b[20;2HB: BACK");
        break;

    case SCREEN_SETTINGS:
        printf("\x1b[8;3HSETTINGS");
        printf("\x1b[11;3HComing soon!");
        printf("\x1b[14;3HSound and controls");
        printf("\x1b[20;2HB: BACK");
        break;

    case SCREEN_SAVES:
        printf("\x1b[8;3HSAVE DATA");
        printf("\x1b[11;3HComing soon!");
        printf("\x1b[14;3HProgress will be");
        printf("\x1b[15;3Hsaved here.");
        printf("\x1b[20;2HB: BACK");
        break;

    case SCREEN_EXIT:
        printf("\x1b[8;3HQUIT THE GAME?");
        printItem(12, "YES", exitChoice == 0);
        printItem(15, "NO",  exitChoice == 1);
        printf("\x1b[20;2HA: CONFIRM");
        break;
    }

    printf("\x1b[24;1H");
    printf("%.30s", statusMessage);
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, &bottomScreen);

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    target = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

    if (!target) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

    if (!initScene()) {
        C3D_RenderTargetDelete(target);
        C3D_Fini();
        gfxExit();
        return 1;
    }

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_B) {
            if (screen != SCREEN_MAIN) {
                screen = SCREEN_MAIN;
                selected = 0;
                statusMessage = "WELCOME TO THE COAST!";
            }
        }

        if (screen == SCREEN_MAIN) {
            if (down & KEY_UP)
                selected = (selected + MENU_COUNT - 1) % MENU_COUNT;

            if (down & KEY_DOWN)
                selected = (selected + 1) % MENU_COUNT;

            if (down & KEY_A) {
                switch (selected) {
                case 0:
                    screen = SCREEN_PLAY;
                    selected = 0;
                    break;
                case 1:
                    screen = SCREEN_SETTINGS;
                    break;
                case 2:
                    screen = SCREEN_SAVES;
                    break;
                case 3:
                    screen = SCREEN_EXIT;
                    exitChoice = 1;
                    break;
                }
            }
        } else if (screen == SCREEN_PLAY) {
            if (down & KEY_UP)
                selected = (selected + 1) % 2;

            if (down & KEY_DOWN)
                selected = (selected + 1) % 2;

            if (down & KEY_A) {
                if (selected == 0)
                    statusMessage = "OFFLINE: COMING SOON";
                else
                    statusMessage = "MULTIPLAYER: COMING SOON";
            }
        } else if (screen == SCREEN_EXIT) {
            if (down & KEY_UP || down & KEY_DOWN)
                exitChoice = 1 - exitChoice;

            if (down & KEY_A) {
                if (exitChoice == 0)
                    break;

                screen = SCREEN_MAIN;
                selected = 0;
            }
        }

        if (down & KEY_START)
            break;

        drawMenu();

        roadOffset += 0.10f;
        if (roadOffset >= 3.0f)
            roadOffset -= 3.0f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target, C3D_CLEAR_ALL, 0xF08A42FF, 0);

        C3D_FrameDrawOn(target);

        C3D_FVUnifMtx4x4(
            GPU_VERTEX_SHADER, uProjection, &projection);

        C3D_FVUnifMtx4x4(
            GPU_VERTEX_SHADER, uModelView, &modelView);

        drawEnvironment();
        drawRoad();
        drawCar();

        C3D_FrameEnd(0);
    }

    if (sceneReady)
        shaderProgramFree(&program);

    if (dvlb)
        DVLB_Free(dvlb);

    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}

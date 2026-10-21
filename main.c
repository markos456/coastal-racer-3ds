
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

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

static DVLB_s *dvlb;
static shaderProgram_s program;
static C3D_Mtx projection;
static C3D_Mtx modelView;
static C3D_RenderTarget *target;

static int uProjection;
static int uModelView;

static PrintConsole bottomScreen;
static MenuScreen screen = SCREEN_MAIN;
static int selected = 0;
static int exitChoice = 0;
static float roadOffset = 0.0f;

static const char *mainItems[MENU_COUNT] = {
    "PLAY",
    "SETTINGS",
    "SAVES",
    "EXIT"
};

static const char *statusMessage = "WELCOME TO THE COAST!";

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
    triangle(ax,ay,az, bx,by,bz, cx,cy,cz, r,g,b);
    triangle(ax,ay,az, cx,cy,cz, dx,dy,dz, r,g,b);
}

static void box(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float r, float g, float b)
{
    float a=x1, c=x2;
    float d=y1, e=y2;
    float f=z1, h=z2;

    quad(a,d,f, c,d,f, c,e,f, a,e,f, r,g,b);
    quad(c,d,h, a,d,h, a,e,h, c,e,h, r,g,b);

    quad(a,d,h, a,d,f, a,e,f, a,e,h,
         r*0.65f,g*0.65f,b*0.65f);

    quad(c,d,f, c,d,h, c,e,h, c,e,f,
         r*0.8f,g*0.8f,b*0.8f);

    quad(a,e,f, c,e,f, c,e,h, a,e,h, r,g,b);

    quad(a,d,h, c,d,h, c,d,f, a,d,f,
         r*0.5f,g*0.5f,b*0.5f);
}

static void drawSkyAndOcean(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    quad(-12,3.5f,-12, 12,3.5f,-12,
          12,0.15f,-12, -12,0.15f,-12,
          0.95f,0.30f,0.16f);

    quad(-12,0.55f,-11, 12,0.55f,-11,
          12,0.05f,-11, -12,0.05f,-11,
          1.0f,0.48f,0.35f);

    quad(-15,0.1f,-10, 15,0.1f,-10,
          15,-3,-10, -15,-3,-10,
          0.06f,0.30f,0.48f);

    for (int i=0; i<24; i++) {
        float a1 = i * 2.0f * PI / 24.0f;
        float a2 = (i+1) * 2.0f * PI / 24.0f;

        triangle(
            0,0.48f,-9.8f,
            cosf(a1)*0.34f,
            0.48f+sinf(a1)*0.34f,-9.8f,
            cosf(a2)*0.34f,
            0.48f+sinf(a2)*0.34f,-9.8f,
            1.0f,0.68f,0.25f);
    }

    C3D_ImmDrawEnd();
}

static void drawRoad(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    quad(-5,-2.5f,-10, 5,-2.5f,-10,
          1.55f,-0.28f,-2, -1.55f,-0.28f,-2,
          0.16f,0.17f,0.20f);

    quad(-5,-2.5f,-10, -4.65f,-2.5f,-10,
         -1.42f,-0.28f,-2, -1.55f,-0.28f,-2,
          0.90f,0.85f,0.70f);

    quad(4.65f,-2.5f,-10, 5,-2.5f,-10,
          1.55f,-0.28f,-2, 1.42f,-0.28f,-2,
          0.90f,0.85f,0.70f);

    for (int i=0; i<9; i++) {
        float t = (float)i/9.0f;
        float z1 = -9.5f+t*7.0f+roadOffset;
        float z2 = z1+0.32f+t*0.18f;

        if (z1 > -2.1f) {
            z1 -= 7.0f;
            z2 -= 7.0f;
        }

        if (z2 < -10 || z1 > -2)
            continue;

        if (z1 < -10) z1 = -10;
        if (z2 > -2) z2 = -2;

        float w1 = 0.035f*(-z1)/3.0f;
        float w2 = 0.035f*(-z2)/3.0f;
        float y1 = -2.8f/(-z1);
        float y2 = -2.8f/(-z2);

        quad(-w1,y1,z1, w1,y1,z1,
              w2,y2,z2, -w2,y2,z2,
              1.0f,0.88f,0.55f);
    }

    C3D_ImmDrawEnd();
}

static void drawCar(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    box(-0.65f,-0.22f,-3.8f, 0.65f,0.08f,-2.2f,
         0.85f,0.025f,0.035f);

    box(-0.43f,0.08f,-3.45f, 0.43f,0.38f,-2.65f,
         0.12f,0.22f,0.30f);

    box(-0.34f,0.38f,-3.30f, 0.34f,0.43f,-2.80f,
         0.80f,0.025f,0.035f);

    box(-0.78f,-0.30f,-3.65f, -0.58f,0,-3.30f,
         0.035f,0.035f,0.045f);

    box(0.58f,-0.30f,-3.65f, 0.78f,0,-3.30f,
         0.035f,0.035f,0.045f);

    box(-0.78f,-0.30f,-2.65f, -0.58f,0,-2.30f,
         0.035f,0.035f,0.045f);

    box(0.58f,-0.30f,-2.65f, 0.78f,0,-2.30f,
         0.035f,0.035f,0.045f);

    box(-0.58f,0.12f,-3.88f, 0.58f,0.19f,-3.78f,
         0.12f,0.12f,0.13f);

    box(-0.50f,-0.10f,-2.23f, -0.22f,-0.02f,-2.19f,
         1.0f,0.85f,0.45f);

    box(0.22f,-0.10f,-2.23f, 0.50f,-0.02f,-2.19f,
         1.0f,0.85f,0.45f);

    C3D_ImmDrawEnd();
}

static void initScene(void)
{
    dvlb = DVLB_ParseFile((u32 *)vshader_shbin,
                          vshader_shbin_size);

    if (!dvlb)
        return;

    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    uProjection = shaderInstanceGetUniformLocation(
        program.vertexShader, "projection");
    uModelView = shaderInstanceGetUniformLocation(
        program.vertexShader, "modelView");

    C3D_AttrInfo *attr = C3D_GetAttrInfo();
    AttrInfo_Init(attr);
    AttrInfo_AddLoader(attr, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attr, 1, GPU_FLOAT, 4);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    Mtx_PerspTilt(&projection,
        C3D_AngleFromDegrees(50.0f),
        C3D_AspectRatioTop, 0.1f, 100.0f, false);

    Mtx_Identity(&modelView);
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
        printItem(8,  "PLAY",      selected == 0);
        printItem(11, "SETTINGS",  selected == 1);
        printItem(14, "SAVES",     selected == 2);
        printItem(17, "EXIT",      selected == 3);
        printf("\x1b[21;2HUP/DOWN: SELECT");
        printf("\x1b[22;2HA: CONFIRM");
        break;

    case SCREEN_PLAY:
        printf("\x1b[6;4HSELECT GAME MODE");
        printItem(10, "OFFLINE", selected == 0);
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

    initScene();

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

        if (screen == SCREEN_MAIN ||
            screen == SCREEN_PLAY ||
            screen == SCREEN_SETTINGS ||
            screen == SCREEN_SAVES ||
            screen == SCREEN_EXIT) {
            drawMenu();
        }

        roadOffset += 0.045f;
        if (roadOffset >= 7.0f)
            roadOffset -= 7.0f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(target, C3D_CLEAR_ALL,
                              0xF08A42FF, 0);
        C3D_FrameDrawOn(target);

        C3D_FVUnifMtx4x4(
            GPU_VERTEX_SHADER, uProjection, &projection);
        C3D_FVUnifMtx4x4(
            GPU_VERTEX_SHADER, uModelView, &modelView);

        drawSkyAndOcean();
        drawRoad();
        drawCar();

        C3D_FrameEnd(0);
    }

    shaderProgramFree(&program);
    DVLB_Free(dvlb);
    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}

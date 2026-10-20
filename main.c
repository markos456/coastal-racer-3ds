
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <math.h>

#include "vshader_shbin.h"

#define MENU_COUNT 5
#define PI 3.14159265f

#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | \
     GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s *dvlb;
static shaderProgram_s program;
static C3D_Mtx projection;
static C3D_Mtx modelView;
static C3D_RenderTarget *target;

static int uProjection;
static int uModelView;

static PrintConsole bottomScreen;
static int menuSelected = 0;
static float roadOffset = 0.0f;

static const char *menuItems[MENU_COUNT] = {
    "DRIVE",
    "GARAGE",
    "CAR SHOP",
    "TRACKS",
    "OPTIONS"
};

static const char *statusMessage = "WELCOME TO THE COAST!";

/* Vertex helper: no rotation, so the car stays facing forward. */
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

/* Sunset background and ocean */
static void drawSkyAndOcean(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Warm sky */
    quad(-12.0f, 3.5f, -12.0f,
          12.0f, 3.5f, -12.0f,
          12.0f, 0.15f, -12.0f,
         -12.0f, 0.15f, -12.0f,
          0.95f, 0.30f, 0.16f);

    /* Pink sunset band */
    quad(-12.0f, 0.55f, -11.0f,
          12.0f, 0.55f, -11.0f,
          12.0f, 0.05f, -11.0f,
         -12.0f, 0.05f, -11.0f,
          1.0f, 0.48f, 0.35f);

    /* Ocean */
    quad(-15.0f, 0.10f, -10.0f,
          15.0f, 0.10f, -10.0f,
          15.0f,-3.00f, -10.0f,
         -15.0f,-3.00f, -10.0f,
          0.06f, 0.30f, 0.48f);

    /* Sun disc, made of triangles */
    float cx = 0.0f;
    float cy = 0.48f;
    float radius = 0.34f;
    float z = -9.8f;

    for (int i = 0; i < 24; i++) {
        float a1 = (float)i * 2.0f * PI / 24.0f;
        float a2 = (float)(i + 1) * 2.0f * PI / 24.0f;

        triangle(
            cx, cy, z,
            cx + cosf(a1)*radius,
            cy + sinf(a1)*radius, z,
            cx + cosf(a2)*radius,
            cy + sinf(a2)*radius, z,
            1.0f, 0.68f, 0.25f);
    }

    C3D_ImmDrawEnd();
}

/* Perspective road that moves toward the camera. */
static void drawRoad(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Main asphalt strip */
    quad(-5.0f, -2.5f, -10.0f,
          5.0f, -2.5f, -10.0f,
          1.55f, -0.28f, -2.0f,
         -1.55f, -0.28f, -2.0f,
          0.16f, 0.17f, 0.20f);

    /* Road edges */
    quad(-5.0f, -2.5f, -10.0f,
         -4.65f, -2.5f, -10.0f,
         -1.42f, -0.28f, -2.0f,
         -1.55f, -0.28f, -2.0f,
          0.90f, 0.85f, 0.70f);

    quad(4.65f, -2.5f, -10.0f,
          5.0f, -2.5f, -10.0f,
          1.55f, -0.28f, -2.0f,
          1.42f, -0.28f, -2.0f,
          0.90f, 0.85f, 0.70f);

    /* Dashed center line */
    for (int i = 0; i < 9; i++) {
        float t = (float)i / 9.0f;
        float z1 = -9.5f + t * 7.0f + roadOffset;
        float z2 = z1 + 0.32f + t * 0.18f;

        if (z1 > -2.1f) {
            z1 -= 7.0f;
            z2 -= 7.0f;
        }

        if (z2 < -10.0f || z1 > -2.0f)
            continue;

        if (z1 < -10.0f) z1 = -10.0f;
        if (z2 > -2.0f) z2 = -2.0f;

        float w1 = 0.035f * (-z1) / 3.0f;
        float w2 = 0.035f * (-z2) / 3.0f;
        float y1 = -0.28f * 10.0f / (-z1);
        float y2 = -0.28f * 10.0f / (-z2);

        quad(-w1,y1,z1, w1,y1,z1,
              w2,y2,z2,-w2,y2,z2,
              1.0f, 0.88f, 0.55f);
    }

    C3D_ImmDrawEnd();
}

/* Car remains in one place on the screen. */
static void drawCar(void)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Red body */
    box(-0.65f, -0.22f, -3.8f,
         0.65f,  0.08f, -2.2f,
         0.85f, 0.025f, 0.035f);

    /* Cabin and windows */
    box(-0.43f, 0.08f, -3.45f,
         0.43f, 0.38f, -2.65f,
         0.12f, 0.22f, 0.30f);

    /* Roof */
    box(-0.34f, 0.38f, -3.30f,
         0.34f, 0.43f, -2.80f,
         0.80f, 0.025f, 0.035f);

    /* Four wheels */
    box(-0.78f,-0.30f,-3.65f,
        -0.58f, 0.00f,-3.30f,
         0.035f,0.035f,0.045f);

    box(0.58f,-0.30f,-3.65f,
        0.78f, 0.00f,-3.30f,
        0.035f,0.035f,0.045f);

    box(-0.78f,-0.30f,-2.65f,
        -0.58f, 0.00f,-2.30f,
         0.035f,0.035f,0.045f);

    box(0.58f,-0.30f,-2.65f,
        0.78f, 0.00f,-2.30f,
        0.035f,0.035f,0.045f);

    /* Rear spoiler */
    box(-0.58f,0.12f,-3.88f,
         0.58f,0.19f,-3.78f,
         0.12f,0.12f,0.13f);

    /* Headlights */
    box(-0.50f,-0.10f,-2.23f,
        -0.22f,-0.02f,-2.19f,
         1.0f,0.85f,0.45f);

    box(0.22f,-0.10f,-2.23f,
         0.50f,-0.02f,-2.19f,
         1.0f,0.85f,0.45f);

    C3D_ImmDrawEnd();
}

static void initScene(void)
{
    dvlb = DVLB_ParseFile(
        (u32 *)vshader_shbin,
        vshader_shbin_size);

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

    Mtx_PerspTilt(
        &projection,
        C3D_AngleFromDegrees(50.0f),
        C3D_AspectRatioTop,
        0.1f, 100.0f, false);

    Mtx_Identity(&modelView);
}

static void drawMenu(void)
{
    consoleSelect(&bottomScreen);
    consoleClear();

    printf("\x1b[1;2HCOASTAL RACER 3DS");
    printf("\x1b[3;5H-- SUNSET DRIVE --");

    for (int i = 0; i < MENU_COUNT; i++) {
        printf("\x1b[%d;4H", 6 + i * 2);

        if (i == menuSelected)
            printf("> %s <", menuItems[i]);
        else
            printf("  %s", menuItems[i]);
    }

    printf("\x1b[18;2HUP/DOWN: SELECT");
    printf("\x1b[20;2HA: CONFIRM");
    printf("\x1b[22;2HSTART: EXIT");
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
        240, 400,
        GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8);

    if (!target) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        target, GFX_TOP, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS);

    initScene();

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();

        if (down & KEY_START)
            break;

        if (down & KEY_UP)
            menuSelected =
                (menuSelected + MENU_COUNT - 1) % MENU_COUNT;

        if (down & KEY_DOWN)
            menuSelected =
                (menuSelected + 1) % MENU_COUNT;

        if (down & KEY_A) {
            switch (menuSelected) {
                case 0: statusMessage = "DRIVE: COMING SOON"; break;
                case 1: statusMessage = "GARAGE: COMING SOON"; break;
                case 2: statusMessage = "CAR SHOP: COMING SOON"; break;
                case 3: statusMessage = "TRACKS: COMING SOON"; break;
                case 4: statusMessage = "OPTIONS: COMING SOON"; break;
            }
        }

        roadOffset += 0.045f;
        if (roadOffset >= 7.0f)
            roadOffset -= 7.0f;

        drawMenu();

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target, C3D_CLEAR_ALL,
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

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <math.h>
#include "vshader_shbin.h"

#define PI 3.14159265f

#define TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s *shaderDvlb;
static shaderProgram_s shaderProgram;
static C3D_Mtx projection;
static C3D_Mtx modelView;
static int uProjection, uModelView, uColor;
static C3D_RenderTarget *topTarget, *bottomTarget;

static float carX = 0.0f;
static float roadScroll = 0.0f;

static void setColor(float r, float g, float b)
{
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uColor, r, g, b, 1.0f);
}

static void triangle(
    float ax, float ay, float az,
    float bx, float by, float bz,
    float cx, float cy, float cz)
{
    C3D_ImmDrawBegin(GPU_TRIANGLES);
    C3D_ImmSendAttrib(ax, ay, az, 1.0f);
    C3D_ImmSendAttrib(bx, by, bz, 1.0f);
    C3D_ImmSendAttrib(cx, cy, cz, 1.0f);
    C3D_ImmDrawEnd();
}

static void quad(
    float x1, float y1, float z1,
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float x4, float y4, float z4)
{
    triangle(x1,y1,z1, x2,y2,z2, x3,y3,z3);
    triangle(x1,y1,z1, x3,y3,z3, x4,y4,z4);
}

static void drawRoad(void)
{
    setColor(0.16f, 0.18f, 0.21f);
    quad(-5.0f,-1.5f,-2.0f, 5.0f,-1.5f,-2.0f,
          5.0f,-1.5f,-45.0f, -5.0f,-1.5f,-45.0f);

    setColor(0.05f, 0.36f, 0.16f);
    quad(-35.0f,-1.65f,-2.0f, -5.0f,-1.65f,-2.0f,
         -5.0f,-1.65f,-45.0f, -35.0f,-1.65f,-45.0f);
    quad(5.0f,-1.65f,-2.0f, 35.0f,-1.65f,-2.0f,
         35.0f,-1.65f,-45.0f, 5.0f,-1.65f,-45.0f);

    setColor(0.95f, 0.95f, 0.88f);
    quad(-4.0f,-1.48f,-2.0f, -3.85f,-1.48f,-2.0f,
         -3.85f,-1.48f,-45.0f, -4.0f,-1.48f,-45.0f);
    quad(3.85f,-1.48f,-2.0f, 4.0f,-1.48f,-2.0f,
         4.0f,-1.48f,-45.0f, 3.85f,-1.48f,-45.0f);

    setColor(1.0f, 0.85f, 0.15f);
    for (int i = 0; i < 12; i++) {
        float z = -3.0f - i * 3.5f + roadScroll;
        if (z > -2.5f) z -= 42.0f;
        float width = 0.06f + (-z) * 0.002f;
        quad(-width,-1.46f,z, width,-1.46f,z,
             width,-1.46f,z-1.4f, -width,-1.46f,z-1.4f);
    }
}

static void drawCar(void)
{
    float x = carX;
    float z = -6.0f;

    setColor(0.92f, 0.12f, 0.06f);
    quad(x-0.85f,-1.35f,z, x+0.85f,-1.35f,z,
         x+0.85f,-0.95f,z, x-0.85f,-0.95f,z);

    quad(x-0.85f,-1.12f,z, x+0.85f,-1.12f,z,
         x+0.65f,-0.86f,z-0.8f, x-0.65f,-0.86f,z-0.8f);

    setColor(0.95f, 0.18f, 0.08f);
    quad(x-0.55f,-0.95f,z-0.65f, x+0.55f,-0.95f,z-0.65f,
         x+0.38f,-0.58f,z-1.45f, x-0.38f,-0.58f,z-1.45f);

    setColor(0.12f, 0.75f, 0.9f);
    quad(x-0.36f,-0.72f,z-1.42f, x+0.36f,-0.72f,z-1.42f,
         x+0.52f,-0.98f,z-0.72f, x-0.52f,-0.98f,z-0.72f);

    setColor(0.025f, 0.025f, 0.03f);
    for (int side = -1; side <= 1; side += 2) {
        for (int rear = 0; rear <= 1; rear++) {
            float wx = x + side * 0.78f;
            float wz = z - (rear ? 1.35f : 0.25f);
            quad(wx-0.15f,-1.45f,wz, wx+0.15f,-1.45f,wz,
                 wx+0.15f,-1.05f,wz, wx-0.15f,-1.05f,wz);
        }
    }
}

static void drawMenu(void)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(10, 20, 38, 255));
    C2D_SceneBegin(bottomTarget);

    C2D_DrawRectSolid(12, 15, 0, 296, 38,
        C2D_Color32(0, 160, 220, 255));
    C2D_DrawRectSolid(22, 75, 0, 276, 42,
        C2D_Color32(220, 55, 30, 255));
    C2D_DrawRectSolid(22, 130, 0, 276, 42,
        C2D_Color32(30, 110, 65, 255));
    C2D_DrawRectSolid(22, 185, 0, 276, 25,
        C2D_Color32(230, 230, 230, 255));

    // Make sure the queued 2D menu is submitted before ending the frame.
    C2D_Flush();
}

int main(void)
{
    gfxInitDefault();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!topTarget || !bottomTarget) {
        if (topTarget) C3D_RenderTargetDelete(topTarget);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        topTarget, GFX_TOP, GFX_LEFT, TRANSFER_FLAGS);

    // Explicitly configure the bottom screen output as well.
    C3D_RenderTargetSetOutput(
        bottomTarget, GFX_BOTTOM, GFX_LEFT, TRANSFER_FLAGS);

    shaderDvlb = DVLB_ParseFile((u32 *)vshader_shbin, vshader_shbin_size);
    if (!shaderDvlb) {
        C3D_RenderTargetDelete(topTarget);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(&shaderProgram, &shaderDvlb->DVLE[0]);
    C3D_BindProgram(&shaderProgram);

    uProjection = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "projection");
    uModelView = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "modelView");
    uColor = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "uColor");

    C3D_AttrInfo *attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    Mtx_PerspTilt(&projection, 55.0f * PI / 180.0f,
        400.0f / 240.0f, 0.1f, 100.0f, false);
    Mtx_Identity(&modelView);

    C3D_TexEnv *env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_START) break;
        if (held & KEY_LEFT) carX -= 0.035f;
        if (held & KEY_RIGHT) carX += 0.035f;
        if (carX < -2.5f) carX = -2.5f;
        if (carX >  2.5f) carX =  2.5f;

        roadScroll += 0.10f;
        if (roadScroll >= 3.5f) roadScroll -= 3.5f;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(topTarget, C3D_CLEAR_ALL, 0x17334FFF, 0);
        C3D_FrameDrawOn(topTarget);
        C3D_BindProgram(&shaderProgram);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uProjection, &projection);
        C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uModelView, &modelView);

        drawRoad();
        drawCar();

        // Draw the bottom screen in the same frame, after the 3D scene.
        drawMenu();

        C3D_FrameEnd(0);
    }

    shaderProgramFree(&shaderProgram);
    DVLB_Free(shaderDvlb);
    C3D_RenderTargetDelete(topTarget);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}

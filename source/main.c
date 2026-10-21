
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <string.h>

#include "vshader_shbin.h"

#define TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | \
     GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct {
    float x, y, z;
} Vertex;

/* Куб внутри координат видимой области */
static const Vertex cubeVertices[] = {
    {-0.45f,-0.45f, 0.20f}, { 0.45f,-0.45f, 0.20f}, { 0.45f, 0.45f, 0.20f},
    {-0.45f,-0.45f, 0.20f}, { 0.45f, 0.45f, 0.20f}, {-0.45f, 0.45f, 0.20f},

    {-0.45f,-0.45f,-0.20f}, {-0.45f, 0.45f,-0.20f}, { 0.45f, 0.45f,-0.20f},
    {-0.45f,-0.45f,-0.20f}, { 0.45f, 0.45f,-0.20f}, { 0.45f,-0.45f,-0.20f},

    {-0.45f,-0.45f,-0.20f}, {-0.45f,-0.45f, 0.20f}, {-0.45f, 0.45f, 0.20f},
    {-0.45f,-0.45f,-0.20f}, {-0.45f, 0.45f, 0.20f}, {-0.45f, 0.45f,-0.20f},

    { 0.45f,-0.45f,-0.20f}, { 0.45f, 0.45f, 0.20f}, { 0.45f,-0.45f, 0.20f},
    { 0.45f,-0.45f,-0.20f}, { 0.45f, 0.45f,-0.20f}, { 0.45f, 0.45f, 0.20f},

    {-0.45f, 0.45f,-0.20f}, {-0.45f, 0.45f, 0.20f}, { 0.45f, 0.45f, 0.20f},
    {-0.45f, 0.45f,-0.20f}, { 0.45f, 0.45f, 0.20f}, { 0.45f, 0.45f,-0.20f},

    {-0.45f,-0.45f,-0.20f}, { 0.45f,-0.45f, 0.20f}, {-0.45f,-0.45f, 0.20f},
    {-0.45f,-0.45f,-0.20f}, { 0.45f,-0.45f,-0.20f}, { 0.45f,-0.45f, 0.20f}
};

static C3D_RenderTarget* topTarget;
static C3D_RenderTarget* bottomTarget;

static DVLB_s* shaderDvlb;
static shaderProgram_s shaderProgram;
static bool shaderReady = false;

static void drawCube(void)
{
    C3D_BindProgram(&shaderProgram);

    AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(
        bufInfo,
        (void*)cubeVertices,
        sizeof(Vertex),
        1,
        0x0
    );

    Mtx projection;
    Mtx modelView;

    Mtx_Identity(&projection);
    Mtx_Identity(&modelView);

    int projectionLoc =
        shaderInstanceGetUniformLocation(
            shaderProgram.vertexShader, "projection");

    int modelViewLoc =
        shaderInstanceGetUniformLocation(
            shaderProgram.vertexShader, "modelView");

    int colorLoc =
        shaderInstanceGetUniformLocation(
            shaderProgram.vertexShader, "uColor");

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, projectionLoc, &projection);

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, modelViewLoc, &modelView);

    /* Яркий голубой цвет для проверки шейдера */
    C3D_FVUnifSet(
        GPU_VERTEX_SHADER, colorLoc,
        0.10f, 0.85f, 1.0f, 1.0f);

    C3D_DrawArrays(GPU_TRIANGLES, 0,
        sizeof(cubeVertices) / sizeof(cubeVertices[0]));
}

int main(void)
{
    gfxInitDefault();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C2D_Prepare();

    topTarget = C3D_RenderTargetCreate(
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

    bottomTarget = C3D_RenderTargetCreate(
        240, 320, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

    if (!topTarget || !bottomTarget) {
        if (topTarget) C3D_RenderTargetDelete(topTarget);
        if (bottomTarget) C3D_RenderTargetDelete(bottomTarget);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        topTarget, GFX_TOP, GFX_LEFT, TRANSFER_FLAGS);

    C3D_RenderTargetSetOutput(
        bottomTarget, GFX_BOTTOM, GFX_LEFT, TRANSFER_FLAGS);

    shaderDvlb = DVLB_ParseFile(
        (u32*)vshader_shbin, vshader_shbin_size);

    if (!shaderDvlb) {
        C3D_RenderTargetDelete(topTarget);
        C3D_RenderTargetDelete(bottomTarget);
        C2D_Fini();
        C3D_Fini();
        gfxExit();
        return 1;
    }

    shaderProgramInit(&shaderProgram);
    shaderReady = true;

    shaderProgramSetVsh(
        &shaderProgram, &shaderDvlb-> DVLE[0]);

    C3D_DepthTest(false, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);

    bool running = true;

    while (running) {
        hidScanInput();
        u32 keys = hidKeysDown();

        if (keys & KEY_START)
            running = false;

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        /* Верхний экран: фон и тестовая 3D-геометрия */
        C3D_FrameDrawOn(topTarget);

        C3D_RenderTargetClear(
            topTarget, C3D_CLEAR_ALL,
            C2D_Color32(20, 30, 65, 255), 0);

        if (shaderReady)
            drawCube();

        /* Нижний экран: тестовый интерфейс */
        C2D_SceneBegin(bottomTarget);

        C2D_TargetClear(
            bottomTarget, C2D_Color32(12, 18, 35, 255));

        C2D_DrawRectSolid(
            12, 15, 0, 296, 48,
            C2D_Color32(0, 180, 100, 255));

        C2D_DrawRectSolid(
            24, 100, 0, 272, 42,
            C2D_Color32(30, 100, 220, 255));

        C2D_DrawRectSolid(
            24, 155, 0, 272, 42,
            C2D_Color32(220, 110, 20, 255));

        C2D_DrawRectSolid(
            24, 210, 0, 272, 42,
            C2D_Color32(150, 40, 180, 255));

        C3D_FrameEnd();
    }

    if (shaderReady)
        shaderProgramFree(&shaderProgram);

    if (shaderDvlb)
        DVLB_Free(shaderDvlb);

    C3D_RenderTargetDelete(topTarget);
    C3D_RenderTargetDelete(bottomTarget);

    C2D_Fini();
    C3D_Fini();
    gfxExit();

    return 0;
}


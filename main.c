
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>

#include "vshader_shbin.h"

#define CLEAR_COLOR 0xF08A42FF

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

static int uProjection;
static int uModelView;

static void initScene(void)
{
    dvlb = DVLB_ParseFile(
        (u32 *)vshader_shbin,
        vshader_shbin_size
    );

    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    uProjection = shaderInstanceGetUniformLocation(
        program.vertexShader, "projection"
    );

    uModelView = shaderInstanceGetUniformLocation(
        program.vertexShader, "modelView"
    );

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
        0.1f,
        100.0f,
        false
    );

    Mtx_Identity(&modelView);
}

static void drawTriangle(void)
{
    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, uProjection, &projection
    );

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, uModelView, &modelView
    );

    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Red vertex */
    C3D_ImmSendAttrib(-1.0f, -0.8f, -3.0f, 1.0f);
    C3D_ImmSendAttrib(1.0f, 0.1f, 0.1f, 1.0f);

    /* Green vertex */
    C3D_ImmSendAttrib(1.0f, -0.8f, -3.0f, 1.0f);
    C3D_ImmSendAttrib(0.1f, 1.0f, 0.2f, 1.0f);

    /* Blue vertex */
    C3D_ImmSendAttrib(0.0f, 0.9f, -3.0f, 1.0f);
    C3D_ImmSendAttrib(0.1f, 0.3f, 1.0f, 1.0f);

    C3D_ImmDrawEnd();
}

int main(void)
{
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        gfxExit();
        return 1;
    }

    C3D_RenderTarget *target = C3D_RenderTargetCreate(
        240, 400,
        GPU_RB_RGBA8,
        GPU_RB_DEPTH24_STENCIL8
    );

    if (!target) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(
        target,
        GFX_TOP,
        GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS
    );

    initScene();

    while (aptMainLoop()) {
        hidScanInput();

        if (hidKeysDown() & KEY_START)
            break;

        consoleSelect(NULL);
        consoleClear();
        printf("\x1b[2;4HCOASTAL RACER 3DS");
        printf("\x1b[5;7H3D TEST");
        printf("\x1b[8;3HNext stop: SUPRA A70");
        printf("\x1b[12;3HSTART: EXIT");

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        C3D_RenderTargetClear(
            target,
            C3D_CLEAR_ALL,
            CLEAR_COLOR,
            0
        );

        C3D_FrameDrawOn(target);
        drawTriangle();

        C3D_FrameEnd(0);
    }

    shaderProgramFree(&program);
    DVLB_Free(dvlb);
    C3D_RenderTargetDelete(target);
    C3D_Fini();
    gfxExit();

    return 0;
}

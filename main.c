
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
static C3D_Mtx projection, modelView;
static int uProjection, uModelView;

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
    float f=z1, g2=z2;

    quad(a,d,f, c,d,f, c,e,f, a,e,f, r,g,b);
    quad(c,d,g2, a,d,g2, a,e,g2, c,e,g2, r,g,b);
    quad(a,d,g2, a,d,f, a,e,f, a,e,g2, r*0.65f,g*0.65f,b*0.65f);
    quad(c,d,f, c,d,g2, c,e,g2, c,e,f, r*0.8f,g*0.8f,b*0.8f);
    quad(a,e,f, c,e,f, c,e,g2, a,e,g2, r,g,b);
    quad(a,d,g2, c,d,g2, c,d,f, a,d,f, r*0.5f,g*0.5f,b*0.5f);
}

static void initScene(void)
{
    dvlb = DVLB_ParseFile(
        (u32 *)vshader_shbin, vshader_shbin_size);

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

static void drawCar(void)
{
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,
        uProjection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,
        uModelView, &modelView);

    C3D_ImmDrawBegin(GPU_TRIANGLES);

    /* Red sports-car body */
    box(-0.65f, -0.22f, -3.8f,
         0.65f,  0.08f, -2.2f,
         0.85f, 0.025f, 0.035f);

    /* Raised cabin and dark windows */
    box(-0.43f, 0.08f, -3.45f,
         0.43f, 0.38f, -2.65f,
         0.12f, 0.22f, 0.30f);

    /* Roof */
    box(-0.34f, 0.38f, -3.30f,
         0.34f, 0.43f, -2.80f,
         0.80f, 0.025f, 0.035f);

    /* Four black wheels */
    box(-0.78f, -0.30f, -3.65f,
        -0.58f,  0.00f, -3.30f,
         0.035f, 0.035f, 0.045f);

    box(0.58f, -0.30f, -3.65f,
        0.78f,  0.00f, -3.30f,
        0.035f, 0.035f, 0.045f);

    box(-0.78f, -0.30f, -2.65f,
        -0.58f,  0.00f, -2.30f,
         0.035f, 0.035f, 0.045f);

    box(0.58f, -0.30f, -2.65f,
        0.78f,  0.00f, -2.30f,
        0.035f, 0.035f, 0.045f);

    /* Rear spoiler */
    box(-0.58f, 0.12f, -3.88f,
         0.58f, 0.19f, -3.78f,
         0.12f, 0.12f, 0.13f);

    /* Headlights at the front */
    box(-0.50f, -0.10f, -2.23f,
        -0.22f, -0.02f, -2.19f,
         1.0f, 0.85f, 0.45f);

    box(0.22f, -0.10f, -2.23f,
         0.50f, -0.02f, -2.19f,
         1.0f, 0.85f, 0.45f);

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
        240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);

    if (!target) {
        C3D_Fini();
        gfxExit();
        return 1;
    }

    C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT,
        DISPLAY_TRANSFER_FLAGS);

    initScene();

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START)
            break;

        consoleSelect(NULL);
        consoleClear();
        printf("\x1b[2;5HCOASTAL RACER 3DS");
        printf("\x1b[5;7HSUPRA PROJECT");
        printf("\x1b[8;4HFirst 3D car!");
        printf("\x1b[12;3HSTART: EXIT");

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target, C3D_CLEAR_ALL,
            CLEAR_COLOR, 0);
        C3D_FrameDrawOn(target);
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

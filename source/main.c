#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Shader (генерируется из vshader.v.pica через picasso + bi#include "vshader_shbin.h"
#include "vshader_shbin.h"
// ---- Unit cube (36 vertices, centered at origin) ----
typedef struct { float x, y, z; } vertex;
static const vertex cubeVerts[36] = {
    {-0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},
    { 0.5f,-0.5f,-0.5f},{-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f},{-0.5f,-0.5f, 0.5f},{-0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f},{-0.5f, 0.5f, 0.5f},{-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},{ 0.5f, 0.5f, 0.5f},
    {-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},
    {-0.5f, 0.5f, 0.5f},{ 0.5f, 0.5f,-0.5f},{-0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f},
};

// ---- Globals ----
static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView, uLoc_color;
static C3D_Mtx projection;
static C3D_RenderTarget* topTarget;
static C3D_RenderTarget* bottomTarget;
static C3D_BufInfo* bufInfo;
static void* vbo_data;

// Car state
static float carX = 0.0f;
static float carZ = 0.0f;
static float carAngle = 0.0f;
static float carSpeed = 0.0f;

// ---- Drawing helpers ----
static void drawBox(C3D_Mtx* view, float x, float y, float z,
                    float sx, float sy, float sz, u32 color) {
    C3D_Mtx modelView;
    Mtx_Copy(&modelView, view);
    Mtx_Translate(&modelView, x, y, z, true);
    Mtx_Scale(&modelView, sx, sy, sz);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_color,
                  ((color >> 24) & 0xFF) / 255.0f,
                  ((color >> 16) & 0xFF) / 255.0f,
                  ((color >>  8) & 0xFF) / 255.0f,
                  ( color        & 0xFF) / 255.0f);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawCar(C3D_Mtx* view) {
    // Корпус (низкий, широкий)
    C3D_Mtx carView;
    Mtx_Copy(&carView, view);
    Mtx_Translate(&carView, carX, 0.0f, carZ, true);
    Mtx_RotateY(&carView, carAngle, true);

    // Основной корпус — тёмно-красный
    drawBox(&carView, 0.0f, 0.55f, 0.0f, 1.8f, 0.55f, 4.2f, 0xB01010FF);
    // Крыша / кабина
    drawBox(&carView, 0.0f, 1.05f, -0.2f, 1.4f, 0.55f, 2.0f, 0x901010FF);
    // Капот (чуть выше носа)
    drawBox(&carView, 0.0f, 0.75f, 1.5f, 1.6f, 0.25f, 1.2f, 0xB01010FF);
    // Стёкла (тёмно-синие)
    drawBox(&carView, 0.0f, 1.10f, 0.8f, 1.35f, 0.40f, 0.05f, 0x203050FF);
    drawBox(&carView, 0.0f, 1.10f, -1.2f, 1.35f, 0.40f, 0.05f, 0x203050FF);
    // Фары (жёлтые)
    drawBox(&carView, -0.6f, 0.60f, 2.15f, 0.35f, 0.20f, 0.05f, 0xFFEE88FF);
    drawBox(&carView,  0.6f, 0.60f, 2.15f, 0.35f, 0.20f, 0.05f, 0xFFEE88FF);
    // Задние фонари (красные)
    drawBox(&carView, -0.6f, 0.60f, -2.15f, 0.35f, 0.20f, 0.05f, 0xFF3030FF);
    drawBox(&carView,  0.6f, 0.60f, -2.15f, 0.35f, 0.20f, 0.05f, 0xFF3030FF);

    // Колёса (4 шт, чёрные)
    drawBox(&carView, -0.95f, 0.35f,  1.4f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f,  1.4f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView, -0.95f, 0.35f, -1.4f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f, -1.4f, 0.35f, 0.70f, 0.70f, 0x101010FF);
}

static void renderTopScreen(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(topTarget, C2D_Color32(0xFF, 0x8C, 0x42, 0xFF));
    C2D_SceneBegin(topTarget);

    // Камера сзади и чуть сверху, смотрит на закат
    C3D_Mtx view;
    Mtx_LookAt(&view,
               FVec3_New(carX + 6.0f * sinf(carAngle),
                         3.0f,
                         carZ + 6.0f * cosf(carAngle)),
               FVec3_New(carX, 0.8f, carZ),
               FVec3_New(0.0f, 1.0f, 0.0f),
               false);

    // Дорога (тёмно-серый плоский бокс)
    drawBox(&view, 0.0f, -0.05f, carZ - 20.0f, 8.0f, 0.1f, 80.0f, 0x2A2A2AFF);

    // Разметка (жёлтая полоса)
    drawBox(&view, 0.0f, 0.01f, carZ - 20.0f, 0.15f, 0.02f, 80.0f, 0xE0C040FF);

    // Солнце (большой жёлто-оранжевый бокс вдалеке)
    drawBox(&view, 0.0f, 6.0f, carZ - 60.0f, 12.0f, 12.0f, 0.5f, 0xFFD070FF);

    // Машина
    drawCar(&view);

    C3D_FrameEnd(0);
}

// ---- Кнопки на нижнем экране ----
typedef struct { float x, y, w, h; const char* label; u32 color; } Button;
static Button buttons[4] = {
    {  20.0f, 150.0f, 70.0f, 60.0f, "<",  0x3080FFFF },
    { 130.0f, 150.0f, 70.0f, 60.0f, ">",  0x3080FFFF },
    { 240.0f,  40.0f, 60.0f, 80.0f, "GAS",0x30C040FF },
    { 240.0f, 150.0f, 60.0f, 60.0f, "BRK",0xC03030FF },
};

static int buttonHit(float px, float py) {
    for (int i = 0; i < 4; i++) {
        if (px >= buttons[i].x && px <= buttons[i].x + buttons[i].w &&
            py >= buttons[i].y && py <= buttons[i].y + buttons[i].h) return i;
    }
    return -1;
}

static void renderBottomScreen(int activeBtn) {
    C2D_TargetClear(bottomTarget, C2D_Color32(0x20, 0x20, 0x30, 0xFF));
    C2D_SceneBegin(bottomTarget);

    for (int i = 0; i < 4; i++) {
        Button* b = &buttons[i];
        u32 c = b->color;
        if (i == activeBtn) c = 0xFFFFFFFF;
        C2D_DrawRectSolid(b->x, b->y, 0.0f, b->w, b->h, C2D_Color32(c>>24, (c>>16)&0xFF, (c>>8)&0xFF, c&0xFF));
        C2D_DrawRectSolid(b->x + 2, b->y + 2, 0.0f, b->w - 4, b->h - 4,
                          C2D_Color32(0x40, 0x40, 0x50, 0xFF));
        // (Текст можно добавить через C2D_DrawText, если подключишь шрифт)
    }
    C2D_Flush();
}

int main(void) {
    gfxInitDefault();
    hidInit();
    romfsInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    // ---- Shader ----
    vshader_dvlb = DVLB_ParseFile((u32*)vshader_v_pica_shbin, vshader_v_pica_shbin_size);
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
    C3D_BindProgram(&program);

    uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
    uLoc_modelView  = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
    uLoc_color      = shaderInstanceGetUniformLocation(program.vertexShader, "uColor");

    // ---- VBO ----
    vbo_data = linearAlloc(sizeof(cubeVerts));
    memcpy(vbo_data, cubeVerts, sizeof(cubeVerts));
    bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 1, 0);

    // ---- Attributes ----
    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    // ---- Projection ----
    Mtx_PerspTilt(&projection, C3D_Rad(50.0f), 400.0f / 240.0f, 0.1f, 100.0f);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    int activeBtn = -1;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        if (kDown & KEY_START) break;

        touchPosition touch;
        hidTouchRead(&touch);

        // Determine active button
        activeBtn = -1;
        if (kHeld & KEY_TOUCH) {
            activeBtn = buttonHit(touch.px, touch.py);
        }

        // ---- Logic ----
        bool gas   = (activeBtn == 2) || (kHeld & KEY_A);
        bool brake = (activeBtn == 3) || (kHeld & KEY_B);
        bool left  = (activeBtn == 0) || (kHeld & KEY_DLEFT);
        bool right = (activeBtn == 1) || (kHeld & KEY_DRIGHT);

        if (gas)   carSpeed += 0.08f;
        if (brake) carSpeed -= 0.12f;
        if (carSpeed > 2.0f) carSpeed = 2.0f;
        if (carSpeed < -1.0f) carSpeed = -1.0f;
        if (!gas && !brake) carSpeed *= 0.96f;

        float steer = 0.0f;
        if (left)  steer =  0.06f;
        if (right) steer = -0.06f;
        if (fabsf(carSpeed) > 0.01f) carAngle += steer * (carSpeed > 0 ? 1.0f : -1.0f);

        carX += sinf(carAngle) * carSpeed;
        carZ += cosf(carAngle) * carSpeed;

        // ---- Render ----
        renderTopScreen();
        renderBottomScreen(activeBtn);
    }

    // ---- Cleanup ----
    shaderProgramFree(&program);
    DVLB_Free(vshader_dvlb);
    linearFree(vbo_data);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    hidExit();
    gfxExit();
    return 0;
}

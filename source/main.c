
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <math.h>
#include <string.h>
#include "vshader_shbin.h"

#define PI 3.14159265f

typedef struct {
    float x, y, z;
} Vertex;

typedef enum {
    MODE_MENU,
    MODE_RACE,
    MODE_GARAGE,
    MODE_SETTINGS
} GameMode;

static const Vertex cubeVerts[] = {
    {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
    { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
    {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f},
    {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
    {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
    {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}
};

static C3D_RenderTarget *topTarget;
static C3D_RenderTarget *bottomTarget;
static DVLB_s *shaderDvlb;
static shaderProgram_s shaderProgram;
static bool shaderReady = false;
static void *vertexBuffer = NULL;

static int projectionLocation;
static int modelViewLocation;

static C2D_TextBuf textBuffer;
static C2D_Text titleText, raceText, garageText;
static C2D_Text settingsText, backText, controlsText;
static C2D_Text leftText, rightText, gasText, brakeText;

static GameMode mode = MODE_MENU;
static float carX = 0.0f;
static float carZ = 0.0f;
static float speed = 0.0f;
static float steering = 0.0f;

static void parseText(C2D_Text *text, const char *str)
{
    C2D_TextParse(text, textBuffer, str);
    C2D_TextOptimize(text);
}

static void prepare3D(void)
{
    C3D_BindProgram(&shaderProgram);

    C3D_AttrInfo *attrs = C3D_GetAttrInfo();
    AttrInfo_Init(attrs);
    AttrInfo_AddLoader(attrs, 0, GPU_FLOAT, 3);

    C3D_BufInfo *bufs = C3D_GetBufInfo();
    BufInfo_Init(bufs);
    BufInfo_Add(bufs, vertexBuffer, sizeof(Vertex), 1, 0x0);
}

static C3D_Mtx viewMatrix;
static C3D_Mtx modelMatrix;
static C3D_Mtx combinedMatrix;

static void drawBox(float x, float y, float z,
                    float sx, float sy, float sz)
{
    Mtx_Identity(&modelMatrix);
    Mtx_Scale(&modelMatrix, sx, sy, sz);
    Mtx_Translate(&modelMatrix, x, y, z, true);

    Mtx_Multiply(&combinedMatrix, &viewMatrix, &modelMatrix);

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, modelViewLocation, &combinedMatrix);

    /* Не записываем цвет в регистры матрицы modelView. */
    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawPalm(float x, float z)
{
    drawBox(x, 1.2f, z, 0.22f, 2.4f, 0.22f);
    drawBox(x, 2.35f, z, 1.6f, 0.16f, 0.28f);
    drawBox(x, 2.25f, z, 0.28f, 0.16f, 1.6f);
}

static void drawWorld(void)
{
    drawBox(0, -1.5f, carZ + 40, 150, 0.5f, 160);
    drawBox(0, -1.05f, carZ + 40, 34, 0.4f, 150);
    drawBox(0, -0.75f, carZ + 40, 27, 0.3f, 150);
    drawBox(0, -0.45f, carZ + 40, 8.5f, 0.25f, 150);

    for (int i = 0; i < 9; i++) {
        float z = carZ + 8.0f + i * 13.0f;
        drawPalm(-11.5f, z);
        drawPalm(11.5f, z + 5.0f);
    }

    drawBox(-4.1f, -0.29f, carZ + 40, 0.18f, 0.06f, 150);
    drawBox( 4.1f, -0.29f, carZ + 40, 0.18f, 0.06f, 150);

    for (int i = 0; i < 18; i++) {
        drawBox(0, -0.28f, carZ + 4.0f + i * 8.0f,
                0.16f, 0.06f, 3.5f);
    }
}

static void drawCar(void)
{
    float z = carZ + 1.5f;

    drawBox(carX, 0.15f, z, 1.65f, 0.55f, 2.7f);
    drawBox(carX, 0.58f, z - 0.25f, 1.15f, 0.48f, 1.25f);

    drawBox(carX - 0.88f, -0.05f, z - 0.8f, 0.28f, 0.48f, 0.55f);
    drawBox(carX + 0.88f, -0.05f, z - 0.8f, 0.28f, 0.48f, 0.55f);
    drawBox(carX - 0.88f, -0.05f, z + 0.8f, 0.28f, 0.48f, 0.55f);
    drawBox(carX + 0.88f, -0.05f, z + 0.8f, 0.28f, 0.48f, 0.55f);
}

static void drawTopScene(void)
{
    C3D_FrameDrawOn(topTarget);
    C3D_RenderTargetClear(
        topTarget, C3D_CLEAR_ALL, C2D_Color32(100, 190, 245, 255), 0);

    prepare3D();

    C3D_Mtx projection;
    Mtx_PerspTilt(&projection, PI / 3.0f,
                  400.0f / 240.0f, 0.1f, 300.0f, true);

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, projectionLocation, &projection);

    C3D_Mtx camera;
    C3D_FVec eye = {{carX * 0.25f, 5.0f, carZ - 9.0f, 1.0f}};
    C3D_FVec target = {{carX, 0.0f, carZ + 10.0f, 1.0f}};
    C3D_FVec up = {{0.0f, 1.0f, 0.0f, 0.0f}};

    Mtx_LookAt(&camera, eye, target, up, true);
    Mtx_Copy(&viewMatrix, &camera);

    drawWorld();
    drawCar();

    /* 2D-текст рисуем после 3D-сцены. */
    C2D_SceneBegin(topTarget);

    if (mode == MODE_MENU) {
        C2D_DrawText(&titleText, C2D_AlignCenter,
                     20, 18, 0.5f, 0.8f, 0.8f);
    }
}

static void drawButton(float x, float y, float w, float h,
                       C2D_Text *label, u32 color)
{
    C2D_DrawRectSolid(x, y, 0.5f, w, h, color);
    C2D_DrawText(label, C2D_AlignCenter,
                 x, y + h * 0.33f, 0.6f, 0.65f, 0.65f);
}

static void drawBottomMenu(void)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(18, 35, 55, 255));
    C2D_SceneBegin(bottomTarget);

    drawButton(20, 35, 280, 45, &raceText,
               C2D_Color32(35, 175, 80, 255));
    drawButton(20, 95, 280, 45, &garageText,
               C2D_Color32(40, 115, 210, 255));
    drawButton(20, 155, 280, 45, &settingsText,
               C2D_Color32(100, 100, 125, 255));

    C2D_DrawText(&controlsText, C2D_AlignCenter,
                 20, 220, 0.5f, 0.55f, 0.55f);
}

static void drawBottomPage(C2D_Text *heading)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(18, 35, 55, 255));
    C2D_SceneBegin(bottomTarget);

    C2D_DrawText(heading, C2D_AlignCenter,
                 15, 25, 0.5f, 0.8f, 0.8f);

    drawButton(65, 155, 190, 48, &backText,
               C2D_Color32(190, 65, 65, 255));
}

static void drawBottomControls(void)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(20, 30, 40, 255));
    C2D_SceneBegin(bottomTarget);

    drawButton(10, 145, 75, 65, &leftText,
               C2D_Color32(50, 115, 210, 255));
    drawButton(95, 145, 75, 65, &rightText,
               C2D_Color32(50, 115, 210, 255));
    drawButton(220, 35, 90, 65, &gasText,
               C2D_Color32(35, 185, 75, 255));
    drawButton(220, 125, 90, 65, &brakeText,
               C2D_Color32(210, 55, 55, 255));

    C2D_DrawText(&backText, C2D_AlignCenter,
                 10, 15, 0.5f, 0.6f, 0.6f);
}

int main(void)
{
    gfxInitDefault();
    hidInit();

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

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!topTarget || !bottomTarget)
        goto cleanup;

    shaderDvlb = DVLB_ParseFile(
        (u32 *)vshader_shbin, vshader_shbin_size);

    if (!shaderDvlb)
        goto cleanup;

    shaderProgramInit(&shaderProgram);
    shaderReady = true;
    shaderProgramSetVsh(&shaderProgram, &shaderDvlb->DVLE[0]);

    projectionLocation = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "projection");
    modelViewLocation = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "modelView");

    if (projectionLocation < 0 || modelViewLocation < 0)
        goto cleanup;

    vertexBuffer = linearAlloc(sizeof(cubeVerts));
    if (!vertexBuffer)
        goto cleanup;

    memcpy(vertexBuffer, cubeVerts, sizeof(cubeVerts));

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    textBuffer = C2D_TextBufNew(4096);
    if (!textBuffer)
        goto cleanup;

    parseText(&titleText, "COASTAL RACER");
    parseText(&raceText, "RACE");
    parseText(&garageText, "GARAGE");
    parseText(&settingsText, "SETTINGS");
    parseText(&backText, "B: MENU");
    parseText(&controlsText, "SELECT A MODE");
    parseText(&leftText, "LEFT");
    parseText(&rightText, "RIGHT");
    parseText(&gasText, "GAS");
    parseText(&brakeText, "BRAKE");

    while (aptMainLoop()) {
        hidScanInput();

        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        touchPosition touch = {0};
        hidTouchRead(&touch);

        if (down & KEY_START)
            break;

        if (mode == MODE_MENU) {
            if (down & KEY_TOUCH) {
                if (touch.py >= 35 && touch.py <= 80)
                    mode = MODE_RACE;
                else if (touch.py >= 95 && touch.py <= 140)
                    mode = MODE_GARAGE;
                else if (touch.py >= 155 && touch.py <= 200)
                    mode = MODE_SETTINGS;
            }

            if (down & KEY_A)
                mode = MODE_RACE;
        } else if (mode == MODE_RACE) {
            if (down & KEY_B) {
                mode = MODE_MENU;
                speed = 0.0f;
            }

            if (held & KEY_TOUCH) {
                if (touch.px < 85 && touch.py > 145)
                    steering = -1.0f;
                else if (touch.px < 170 && touch.py > 145)
                    steering = 1.0f;
                else
                    steering = 0.0f;

                if (touch.px > 215 && touch.py < 110)
                    speed += 0.025f;
                if (touch.px > 215 && touch.py >= 110)
                    speed -= 0.04f;
            } else {
                steering = 0.0f;
            }

            if (held & KEY_UP) speed += 0.015f;
            if (held & KEY_DOWN) speed -= 0.025f;
            if (held & KEY_LEFT) steering = -1.0f;
            if (held & KEY_RIGHT) steering = 1.0f;

            if (speed < 0.0f) speed = 0.0f;
            if (speed > 0.8f) speed = 0.8f;

            carX += steering * (0.04f + speed * 0.08f);
            if (carX < -2.7f) carX = -2.7f;
            if (carX > 2.7f) carX = 2.7f;

            carZ += speed;
        } else {
            if ((down & KEY_B) ||
                ((down & KEY_TOUCH) &&
                 touch.py >= 145 && touch.py <= 210)) {
                mode = MODE_MENU;
            }
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        drawTopScene();

        if (mode == MODE_MENU)
            drawBottomMenu();
        else if (mode == MODE_RACE)
            drawBottomControls();
        else if (mode == MODE_GARAGE)
            drawBottomPage(&garageText);
        else
            drawBottomPage(&settingsText);

        C3D_FrameEnd(0);
    }

cleanup:
    if (textBuffer)
        C2D_TextBufDelete(textBuffer);

    if (vertexBuffer)
        linearFree(vertexBuffer);

    if (shaderReady)
        shaderProgramFree(&shaderProgram);

    if (shaderDvlb)
        DVLB_Free(shaderDvlb);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}


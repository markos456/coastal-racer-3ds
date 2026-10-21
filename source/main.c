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

static C3D_Mtx projection;
static C3D_Mtx viewMatrix;
static C3D_Mtx modelMatrix;
static C3D_Mtx combinedMatrix;

static int projectionLocation;
static int modelViewLocation;

static C2D_Font font;
static C2D_TextBuf textBuffer;
static C2D_Text titleText;
static C2D_Text raceText;
static C2D_Text garageText;
static C2D_Text settingsText;
static C2D_Text backText;
static C2D_Text controlsText;

static GameMode mode = MODE_MENU;
static float carX = 0.0f;
static float carZ = 0.0f;
static float speed = 0.0f;
static float steering = 0.0f;

static void parseText(C2D_Text *text, const char *str)
{
    C2D_TextParse(text, textBuffer, font, str);
    C2D_TextOptimize(text);
}

static void drawBox(float x, float y, float z,
                    float sx, float sy, float sz,
                    u32 color)
{
    Mtx_Identity(&modelMatrix);
    Mtx_Scale(&modelMatrix, sx, sy, sz);
    Mtx_Translate(&modelMatrix, x, y, z, true);

    Mtx_Multiply(&combinedMatrix, &viewMatrix, &modelMatrix);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,
                     modelViewLocation, &combinedMatrix);

    C3D_FVUnifSet(GPU_VERTEX_SHADER, 4,
        ((color >> 24) & 255) / 255.0f,
        ((color >> 16) & 255) / 255.0f,
        ((color >> 8) & 255) / 255.0f,
        (color & 255) / 255.0f);

    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawPalm(float x, float z)
{
    drawBox(x, 1.2f, z, 0.22f, 2.4f, 0.22f, C2D_Color32(125, 75, 35, 255));
    drawBox(x, 2.35f, z, 1.6f, 0.16f, 0.28f, C2D_Color32(20, 155, 55, 255));
    drawBox(x, 2.25f, z, 0.28f, 0.16f, 1.6f, C2D_Color32(30, 175, 65, 255));
}

static void drawWorld(void)
{
    /* Океан */
    drawBox(0, -1.5f, carZ + 40,
            150, 0.5f, 160, C2D_Color32(20, 115, 210, 255));

    /* Песчаный остров */
    drawBox(0, -1.05f, carZ + 40,
            34, 0.4f, 150, C2D_Color32(225, 195, 115, 255));

    /* Зелёная земля */
    drawBox(0, -0.75f, carZ + 40,
            27, 0.3f, 150, C2D_Color32(40, 155, 65, 255));

    /* Пальмы по краям */
    for (int i = 0; i < 9; i++) {
        float z = carZ + 8.0f + i * 13.0f;
        drawPalm(-11.5f, z);
        drawPalm(11.5f, z + 5.0f);
    }

    /* Асфальт */
    drawBox(0, -0.45f, carZ + 40,
            8.5f, 0.25f, 150, C2D_Color32(55, 58, 65, 255));

    /* Края трассы */
    drawBox(-4.1f, -0.29f, carZ + 40,
            0.18f, 0.06f, 150, C2D_Color32(245, 245, 245, 255));
    drawBox(4.1f, -0.29f, carZ + 40,
            0.18f, 0.06f, 150, C2D_Color32(245, 245, 245, 255));

    /* Центральная разметка */
    for (int i = 0; i < 18; i++) {
        drawBox(0, -0.28f, carZ + 4.0f + i * 8.0f,
                0.16f, 0.06f, 3.5f, C2D_Color32(255, 230, 80, 255));
    }
}

static void drawCar(void)
{
    float z = carZ + 1.5f;

    /* Кузов */
    drawBox(carX, 0.15f, z,
            1.65f, 0.55f, 2.7f, C2D_Color32(220, 35, 40, 255));

    /* Кабина */
    drawBox(carX, 0.58f, z - 0.25f,
            1.15f, 0.48f, 1.25f, C2D_Color32(45, 190, 235, 255));

    /* Колёса */
    drawBox(carX - 0.88f, -0.05f, z - 0.8f,
            0.28f, 0.48f, 0.55f, C2D_Color32(25, 25, 25, 255));
    drawBox(carX + 0.88f, -0.05f, z - 0.8f,
            0.28f, 0.48f, 0.55f, C2D_Color32(25, 25, 25, 255));
    drawBox(carX - 0.88f, -0.05f, z + 0.8f,
            0.28f, 0.48f, 0.55f, C2D_Color32(25, 25, 25, 255));
    drawBox(carX + 0.88f, -0.05f, z + 0.8f,
            0.28f, 0.48f, 0.55f, C2D_Color32(25, 25, 25, 255));
}

static void drawTopScene(void)
{
    C2D_TargetClear(topTarget, C2D_Color32(100, 190, 245, 255));
    C2D_SceneBegin(topTarget);

    C3D_Mtx camera;
    C3D_Mtx lookProjection;

    Mtx_PerspTilt(&lookProjection, PI / 3.0f,
                  400.0f / 240.0f, 0.1f, 300.0f, true);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,
                     projectionLocation, &lookProjection);

    C3D_FVec eye = { { carX * 0.25f, 5.0f, carZ - 9.0f, 1.0f } };
    C3D_FVec target = { { carX, 0.0f, carZ + 10.0f, 1.0f } };
    C3D_FVec up = { { 0.0f, 1.0f, 0.0f, 0.0f } };

    Mtx_LookAt(&camera, &eye, &target, &up, true);
    Mtx_Copy(&viewMatrix, &camera);

    drawWorld();
    drawCar();

    if (mode == MODE_MENU) {
        C2D_DrawText(&titleText, C2D_AlignCenter,
                     20.0f, 18.0f, 0.5f, 0.8f, 0.8f);
    }
}

static void drawButton(float x, float y, float w, float h,
                       const char *label, u32 color)
{
    C2D_DrawRectSolid(x, y, 0.5f, w, h, color);

    C2D_Text text;
    C2D_TextParse(&text, textBuffer, font, label);
    C2D_TextOptimize(&text);

    C2D_DrawText(&text, C2D_AlignCenter,
                 x, y + h * 0.33f, 0.6f, 0.65f, 0.65f);
}

static void drawBottomMenu(void)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(18, 35, 55, 255));
    C2D_SceneBegin(bottomTarget);

    drawButton(20, 35, 280, 45, "RACE", C2D_Color32(35, 175, 80, 255));
    drawButton(20, 95, 280, 45, "GARAGE", C2D_Color32(40, 115, 210, 255));
    drawButton(20, 155, 280, 45, "SETTINGS", C2D_Color32(100, 100, 125, 255));

    C2D_DrawText(&controlsText, C2D_AlignCenter,
                 20, 220, 0.5f, 0.55f, 0.55f);
}

static void drawBottomPage(const char *heading)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(18, 35, 55, 255));
    C2D_SceneBegin(bottomTarget);

    C2D_Text headingText;
    C2D_TextParse(&headingText, textBuffer, font, heading);
    C2D_TextOptimize(&headingText);

    C2D_DrawText(&headingText, C2D_AlignCenter,
                 15, 25, 0.5f, 0.8f, 0.8f);

    drawButton(65, 155, 190, 48, "BACK", C2D_Color32(190, 65, 65, 255));
}

static void drawBottomControls(void)
{
    C2D_TargetClear(bottomTarget, C2D_Color32(20, 30, 40, 255));
    C2D_SceneBegin(bottomTarget);

    drawButton(10, 145, 75, 65, "LEFT", C2D_Color32(50, 115, 210, 255));
    drawButton(95, 145, 75, 65, "RIGHT", C2D_Color32(50, 115, 210, 255));
    drawButton(220, 35, 90, 65, "GAS", C2D_Color32(35, 185, 75, 255));
    drawButton(220, 125, 90, 65, "BRAKE", C2D_Color32(210, 55, 55, 255));

    C2D_DrawText(&backText, C2D_AlignCenter,
                 10, 15, 0.5f, 0.6f, 0.6f);
}

int main(void)
{
    gfxInitDefault();
    hidInit();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    shaderDvlb = DVLB_ParseFile((u32 *)vshader_shbin,
                                vshader_shbin_size);
    if (!shaderDvlb)
        goto cleanup;

    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(&shaderProgram, &shaderDvlb->DVLE[0]);
    C3D_BindProgram(&shaderProgram);

    projectionLocation = shaderInstanceGetUniformLocation(
        shaderProgramGetVertexShader(&shaderProgram), "projection");
    modelViewLocation = shaderInstanceGetUniformLocation(
        shaderProgramGetVertexShader(&shaderProgram), "modelView");

    C3D_BufInfo bufferInfo;
    C3D_AttrInfo attributeInfo;

    C3D_BufInfo_Init(&bufferInfo);
    C3D_BufInfo_Add(&bufferInfo, cubeVerts, sizeof(Vertex), 1, 0x0);

    C3D_AttrInfo_Init(&attributeInfo);
    C3D_AttrInfo_AddLoader(&attributeInfo, 0, GPU_FLOAT, 3);

    C3D_SetBufInfo(&bufferInfo);
    C3D_SetAttrInfo(&attributeInfo);

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    font = C2D_FontLoadSystem(CFG_REGION_USA);
    textBuffer = C2D_TextBufNew(4096);

    if (!font || !textBuffer)
        goto cleanup;

    parseText(&titleText, "COASTAL RACER");
    parseText(&raceText, "RACE");
    parseText(&garageText, "GARAGE");
    parseText(&settingsText, "SETTINGS");
    parseText(&backText, "B: MENU");
    parseText(&controlsText, "SELECT A MODE");

    while (aptMainLoop()) {
        hidScanInput();

        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();
        touchPosition touch;
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

            if (held & KEY_UP)
                speed += 0.015f;
            if (held & KEY_DOWN)
                speed -= 0.025f;
            if (held & KEY_LEFT)
                steering = -1.0f;
            if (held & KEY_RIGHT)
                steering = 1.0f;

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

        if (mode == MODE_MENU) {
            drawBottomMenu();
        } else if (mode == MODE_RACE) {
            drawBottomControls();
        } else if (mode == MODE_GARAGE) {
            drawBottomPage("GARAGE");
        } else {
            drawBottomPage("SETTINGS");
        }

        C3D_FrameEnd(0);
    }

cleanup:
    if (textBuffer)
        C2D_TextBufDelete(textBuffer);
    if (font)
        C2D_FontFree(font);

    shaderProgramFree(&shaderProgram);

    if (shaderDvlb)
        DVLB_Free(shaderDvlb);

    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}


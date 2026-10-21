
#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#include "vshader_shbin.h"

#define PI 3.14159265f
#define CAR_ACCEL 0.025f
#define CAR_MAX_SPEED 0.8f
#define ROAD_WIDTH 8.0f
#define SECTOR_LENGTH 35.0f

typedef struct {
    float x, y, z;
} Vertex;

typedef struct {
    float x, y, w, h;
} Rect;

typedef enum {
    MODE_MENU,
    MODE_RACE,
    MODE_GARAGE,
    MODE_SETTINGS
} GameMode;

enum {
    MENU_START,
    MENU_GARAGE,
    MENU_SETTINGS
};

enum {
    CTRL_LEFT,
    CTRL_RIGHT,
    CTRL_GAS,
    CTRL_BRAKE
};

static const Vertex cubeVerts[36] = {
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
    {-0.5f,-0.5f,-0.5f},{ 0.5f,-0.5f, 0.5f},{-0.5f,-0.5f, 0.5f}
};

static const Rect menuRects[3] = {
    {20, 48, 280, 44},
    {20, 104, 280, 44},
    {20, 160, 280, 44}
};

static const Rect controlRects[4] = {
    {10, 155, 65, 65},
    {85, 155, 65, 65},
    {225, 45, 85, 75},
    {225, 140, 85, 75}
};

static DVLB_s* shaderDvlb = NULL;
static shaderProgram_s shaderProgram;
static C3D_RenderTarget* topTarget = NULL;
static C3D_RenderTarget* bottomTarget = NULL;
static C3D_Mtx projection;
static C2D_TextBuf textBuffer = NULL;

static C2D_Text menuText[3];
static C2D_Text controlText[4];
static C2D_Text titleText;
static C2D_Text subtitleText;
static C2D_Text backText;

static int locProjection = -1;
static int locModelView = -1;
static int locColor = -1;

static void* vertexBuffer = NULL;

static GameMode gameMode = MODE_MENU;
static int selectedMenu = MENU_START;
static int selectedPage = 0;

static float carX = 0.0f;
static float carZ = 0.0f;
static float carAngle = 0.0f;
static float carSpeed = 0.0f;

static bool showSpeed = true;

static void parseText(C2D_Text* text, const char* value)
{
    C2D_TextParse(text, textBuffer, value);
    C2D_TextOptimize(text);
}

static void drawBox(
    const C3D_Mtx* view,
    float x, float y, float z,
    float sx, float sy, float sz,
    u32 color
) {
    C3D_Mtx modelView;
    Mtx_Copy(&modelView, view);

    Mtx_Translate(&modelView, x, y, z, true);
    Mtx_Scale(&modelView, sx, sy, sz);

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER, locModelView, &modelView
    );

    C3D_FVUnifSet(
        GPU_VERTEX_SHADER,
        locColor,
        ((color >> 24) & 0xFF) / 255.0f,
        ((color >> 16) & 0xFF) / 255.0f,
        ((color >> 8) & 0xFF) / 255.0f,
        (color & 0xFF) / 255.0f
    );

    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawCar(const C3D_Mtx* view)
{
    C3D_Mtx carView;
    Mtx_Copy(&carView, view);

    Mtx_Translate(&carView, carX, 0.0f, carZ, true);
    Mtx_RotateY(&carView, carAngle, true);

    /* Кузов и крыша */
    drawBox(&carView, 0, 0.55f, 0,
            1.8f, 0.55f, 4.2f, 0xB01010FF);

    drawBox(&carView, 0, 1.05f, -0.2f,
            1.35f, 0.55f, 2.0f, 0x901010FF);

    drawBox(&carView, 0, 0.75f, 1.45f,
            1.55f, 0.25f, 1.25f, 0xD02020FF);

    /* Стёкла */
    drawBox(&carView, 0, 1.10f, 0.80f,
            1.25f, 0.35f, 0.06f, 0x203050FF);

    drawBox(&carView, 0, 1.10f, -1.20f,
            1.25f, 0.35f, 0.06f, 0x203050FF);

    /* Фары */
    drawBox(&carView, -0.60f, 0.62f, 2.13f,
            0.35f, 0.18f, 0.06f, 0xFFEE88FF);

    drawBox(&carView, 0.60f, 0.62f, 2.13f,
            0.35f, 0.18f, 0.06f, 0xFFEE88FF);

    drawBox(&carView, -0.60f, 0.62f, -2.13f,
            0.35f, 0.18f, 0.06f, 0xFF3030FF);

    drawBox(&carView, 0.60f, 0.62f, -2.13f,
            0.35f, 0.18f, 0.06f, 0xFF3030FF);

    /* Колёса */
    drawBox(&carView, -0.95f, 0.35f, 1.35f,
            0.35f, 0.70f, 0.70f, 0x101010FF);

    drawBox(&carView, 0.95f, 0.35f, 1.35f,
            0.35f, 0.70f, 0.70f, 0x101010FF);

    drawBox(&carView, -0.95f, 0.35f, -1.35f,
            0.35f, 0.70f, 0.70f, 0x101010FF);

    drawBox(&carView, 0.95f, 0.35f, -1.35f,
            0.35f, 0.70f, 0.70f, 0x101010FF);
}

static void drawPalm(const C3D_Mtx* view, float x, float z)
{
    /* Ствол */
    drawBox(view, x, 1.15f, z,
            0.25f, 2.3f, 0.25f, 0x85502AFF);

    /* Крона */
    drawBox(view, x, 2.35f, z,
            2.0f, 0.20f, 0.45f, 0x20B050FF);

    drawBox(view, x, 2.40f, z,
            0.45f, 0.20f, 2.0f, 0x168840FF);

    drawBox(view, x - 0.45f, 2.25f, z - 0.4f,
            1.2f, 0.18f, 0.35f, 0x30C050FF);

    drawBox(view, x + 0.45f, 2.25f, z + 0.4f,
            1.2f, 0.18f, 0.35f, 0x30C050FF);
}

static void drawIslandWorld(const C3D_Mtx* view, float centerZ)
{
    /* Океан */
    drawBox(view, 0, -1.0f, centerZ,
            400, 0.25f, 320, 0x087DB8FF);

    /* Берега по сторонам дороги */
    drawBox(view, -13, -0.45f, centerZ,
            18, 0.75f, 300, 0xD7C28AFF);

    drawBox(view, 13, -0.45f, centerZ,
            18, 0.75f, 300, 0xD7C28AFF);

    /* Трава на острове */
    drawBox(view, -17, -0.05f, centerZ,
            12, 0.40f, 300, 0x258A45FF);

    drawBox(view, 17, -0.05f, centerZ,
            12, 0.40f, 300, 0x258A45FF);

    /*
       Сектора повторяются вдоль дороги.
       Это процедурные декорации, а не бесконечный физический мир.
    */
    float sectorBase =
        floorf(centerZ / SECTOR_LENGTH) * SECTOR_LENGTH;

    for (int i = -2; i < 7; i++) {
        float z = sectorBase + i * SECTOR_LENGTH;

        drawPalm(view, -12.0f, z);
        drawPalm(view, 12.0f, z + 12.0f);

        /* Небольшие зелёные кусты */
        drawBox(view, -9.0f, 0.25f, z + 8.0f,
                1.3f, 0.8f, 1.3f, 0x147039FF);

        drawBox(view, 9.0f, 0.25f, z + 22.0f,
                1.3f, 0.8f, 1.3f, 0x147039FF);
    }
}

static void drawRoad(const C3D_Mtx* view, float centerZ)
{
    drawBox(view, 0, -0.08f, centerZ,
            ROAD_WIDTH, 0.15f, 300, 0x303038FF);

    /* Краевые линии */
    drawBox(view, -4.05f, 0.01f, centerZ,
            0.12f, 0.025f, 300, 0xF0F0F0FF);

    drawBox(view, 4.05f, 0.01f, centerZ,
            0.12f, 0.025f, 300, 0xF0F0F0FF);

    /* Центральная разметка */
    for (int i = -2; i < 18; i++) {
        float z = centerZ + i * 8.0f;

        drawBox(view, 0, 0.015f, z,
                0.12f, 0.025f, 3.0f, 0xF5D35AFF);
    }
}

static int hitRect(float x, float y, const Rect* r, int count)
{
    for (int i = 0; i < count; i++) {
        if (x >= r[i].x && x <= r[i].x + r[i].w &&
            y >= r[i].y && y <= r[i].y + r[i].h) {
            return i;
        }
    }

    return -1;
}

static void drawButton(
    const Rect* r,
    const C2D_Text* text,
    bool selected
) {
    u32 border = selected
        ? C2D_Color32(0, 245, 255, 255)
        : C2D_Color32(0, 145, 195, 255);

    C2D_DrawRectSolid(
        r->x, r->y, 0.1f, r->w, r->h, border
    );

    C2D_DrawRectSolid(
        r->x + 3, r->y + 3, 0.2f,
        r->w - 6, r->h - 6,
        C2D_Color32(17, 28, 47, 255)
    );

    C2D_DrawText(
        text,
        C2D_AlignCenter,
        r->x + r->w * 0.5f,
        r->y + r->h * 0.5f - 9,
        0.3f,
        0.65f,
        0.65f
    );
}

static void drawBottomMenu(void)
{
    C2D_TargetClear(
        bottomTarget,
        C2D_Color32(9, 16, 31, 255)
    );

    C2D_SceneBegin(bottomTarget);

    C2D_DrawRectSolid(
        10, 10, 0.0f, 300, 28,
        C2D_Color32(0, 115, 160, 255)
    );

    C2D_DrawText(
        &titleText, C2D_AlignCenter,
        160, 15, 0.1f, 0.55f, 0.55f
    );

    for (int i = 0; i < 3; i++) {
        drawButton(
            &menuRects[i],
            &menuText[i],
            i == selectedMenu
        );
    }

    C2D_DrawText(
        &subtitleText, C2D_AlignCenter,
        160, 218, 0.1f, 0.45f, 0.45f
    );

    C2D_SceneEnd();
}

static void drawBottomControls(int active)
{
    C2D_TargetClear(
        bottomTarget,
        C2D_Color32(9, 16, 31, 255)
    );

    C2D_SceneBegin(bottomTarget);

    for (int i = 0; i < 4; i++) {
        drawButton(
            &controlRects[i],
            &controlText[i],
            i == active
        );
    }

    if (showSpeed) {
        C2D_DrawText(
            &subtitleText, C2D_AlignCenter,
            160, 15, 0.1f, 0.55f, 0.55f
        );
    }

    C2D_SceneEnd();
}

static void drawBottomPage(const char* heading)
{
    C2D_TargetClear(
        bottomTarget,
        C2D_Color32(9, 16, 31, 255)
    );

    C2D_SceneBegin(bottomTarget);

    C2D_DrawText(
        &titleText, C2D_AlignCenter,
        160, 35, 0.1f, 0.6f, 0.6f
    );

    C2D_DrawText(
        &backText, C2D_AlignCenter,
        160, 95, 0.1f, 0.75f, 0.75f
    );

    C2D_DrawRectSolid(
        45, 145, 0.1f, 230, 48,
        C2D_Color32(0, 155, 195, 255)
    );

    C2D_DrawText(
        &subtitleText, C2D_AlignCenter,
        160, 160, 0.2f, 0.55f, 0.55f
    );

    (void)heading;

    C2D_SceneEnd();
}

static void drawTopScene(void)
{
    C2D_TargetClear(
        topTarget,
        C2D_Color32(100, 190, 245, 255)
    );

    C2D_SceneBegin(topTarget);

    C3D_Mtx view;

    float cameraDistance =
        gameMode == MODE_MENU ? 10.0f : 7.0f;

    float cameraHeight =
        gameMode == MODE_MENU ? 4.0f : 3.0f;

    Mtx_LookAt(
        &view,
        FVec3_New(
            carX - cameraDistance * sinf(carAngle),
            cameraHeight,
            carZ - cameraDistance * cosf(carAngle)
        ),
        FVec3_New(carX, 0.8f, carZ + 5.0f),
        FVec3_New(0, 1, 0),
        false
    );

    C3D_FVUnifMtx4x4(
        GPU_VERTEX_SHADER,
        locProjection,
        &projection
    );

    drawIslandWorld(&view, carZ + 45.0f);
    drawRoad(&view, carZ + 40.0f);
    drawCar(&view);

    C2D_SceneEnd();
}

int main(void)
{
    gfxInitDefault();
    hidInit();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) {
        hidExit();
        gfxExit();
        return 1;
    }

    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) {
        C3D_Fini();
        hidExit();
        gfxExit();
        return 1;
    }

    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!topTarget || !bottomTarget)
        goto cleanup;

    shaderDvlb = DVLB_ParseFile(
        (u32*)vshader_shbin,
        vshader_shbin_size
    );

    if (!shaderDvlb)
        goto cleanup;

    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(
        &shaderProgram,
        &shaderDvlb->DVLE[0]
    );

    C3D_BindProgram(&shaderProgram);

    locProjection = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "projection"
    );

    locModelView = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "modelView"
    );

    locColor = shaderInstanceGetUniformLocation(
        shaderProgram.vertexShader, "uColor"
    );

    if (locProjection < 0 ||
        locModelView < 0 ||
        locColor < 0)
        goto cleanup;

    vertexBuffer = linearAlloc(sizeof(cubeVerts));

    if (!vertexBuffer)
        goto cleanup;

    memcpy(vertexBuffer, cubeVerts, sizeof(cubeVerts));

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vertexBuffer, sizeof(Vertex), 1, 0);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    Mtx_PerspTilt(
        &projection,
        50.0f * PI / 180.0f,
        400.0f / 240.0f,
        0.1f,
        300.0f,
        false
    );

    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    textBuffer = C2D_TextBufNew(4096);

    if (!textBuffer)
        goto cleanup;

    parseText(&titleText, "COASTAL RACER 3DS");
    parseText(&subtitleText, "TROPICAL ISLAND");
    parseText(&backText, "BACK");

    parseText(&menuText[MENU_START], "START RACE");
    parseText(&menuText[MENU_GARAGE], "GARAGE");
    parseText(&menuText[MENU_SETTINGS], "SETTINGS");

    parseText(&controlText[CTRL_LEFT], "LEFT");
    parseText(&controlText[CTRL_RIGHT], "RIGHT");
    parseText(&controlText[CTRL_GAS], "GAS");
    parseText(&controlText[CTRL_BRAKE], "BRAKE");

    while (aptMainLoop()) {
        hidScanInput();

        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_START)
            break;

        touchPosition touch;
        hidTouchRead(&touch);

        int touchedMenu = -1;
        int touchedControl = -1;

        if (held & KEY_TOUCH) {
            if (gameMode == MODE_MENU) {
                touchedMenu = hitRect(
                    touch.px, touch.py,
                    menuRects, 3
                );
            } else if (gameMode == MODE_RACE) {
                touchedControl = hitRect(
                    touch.px, touch.py,
                    controlRects, 4
                );
            } else {
                Rect backRect = {45, 75, 230, 60};

                if (hitRect(
                    touch.px, touch.py,
                    &backRect, 1
                ) == 0) {
                    selectedPage = 1;
                } else {
                    selectedPage = 0;
                }
            }
        }

        if (gameMode == MODE_MENU) {
            if (down & KEY_DUP)
                selectedMenu =
                    (selectedMenu + 2) % 3;

            if (down & KEY_DDOWN)
                selectedMenu =
                    (selectedMenu + 1) % 3;

            if (touchedMenu >= 0)
                selectedMenu = touchedMenu;

            bool activate =
                (down & KEY_A) ||
                ((down & KEY_TOUCH) && touchedMenu >= 0);

            if (activate) {
                if (selectedMenu == MENU_START) {
                    gameMode = MODE_RACE;
                } else if (selectedMenu == MENU_GARAGE) {
                    gameMode = MODE_GARAGE;
                } else {
                    gameMode = MODE_SETTINGS;
                }
            }
        } else if (gameMode == MODE_GARAGE ||
                   gameMode == MODE_SETTINGS) {
            if ((down & KEY_B) ||
                ((down & KEY_TOUCH) && selectedPage == 1)) {
                gameMode = MODE_MENU;
                selectedPage = 0;
            }

            if (down & KEY_A) {
                if (gameMode == MODE_SETTINGS)
                    showSpeed = !showSpeed;
            }
        } else {
            bool gas =
                touchedControl == CTRL_GAS ||
                (held & KEY_A);

            bool brake =
                touchedControl == CTRL_BRAKE ||
                (held & KEY_B);

            bool left =
                touchedControl == CTRL_LEFT ||
                (held & KEY_DLEFT);

            bool right =
                touchedControl == CTRL_RIGHT ||
                (held & KEY_DRIGHT);

            if (down & KEY_X) {
                gameMode = MODE_MENU;
                carSpeed = 0.0f;
            }

            if (gas)
                carSpeed += CAR_ACCEL;

            if (brake)
                carSpeed -= CAR_ACCEL * 1.5f;

            if (!gas && !brake)
                carSpeed *= 0.985f;

            if (carSpeed > CAR_MAX_SPEED)
                carSpeed = CAR_MAX_SPEED;

            if (carSpeed < -0.35f)
                carSpeed = -0.35f;

            if (fabsf(carSpeed) > 0.005f) {
                float steering = carSpeed > 0 ? 1.0f : -1.0f;

                if (left)
                    carAngle += 0.035f * steering;

                if (right)
                    carAngle -= 0.035f * steering;
            }

            carX += sinf(carAngle) * carSpeed;
            carZ += cosf(carAngle) * carSpeed;

            /* Не даём машине слишком далеко уйти с трассы */
            if (carX < -12.0f) carX = -12.0f;
            if (carX > 12.0f) carX = 12.0f;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        drawTopScene();

        if (gameMode == MODE_MENU) {
            drawBottomMenu();
        } else if (gameMode == MODE_RACE) {
            drawBottomControls(touchedControl);
        } else {
            drawBottomPage(
                gameMode == MODE_GARAGE
                    ? "GARAGE"
                    : "SETTINGS"
            );
        }

        C3D_FrameEnd(0);
    }

cleanup:
    if (textBuffer)
        C2D_TextBufDelete(textBuffer);

    if (vertexBuffer)
        linearFree(vertexBuffer);

    if (shaderDvlb)
        DVLB_Free(shaderDvlb);

    C2D_Fini();
    C3D_Fini();

    gfxExit();
    hidExit();

    return 0;
}



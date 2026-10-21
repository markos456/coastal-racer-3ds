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

typedef struct {
    float x, y, z;
} Vertex;

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

typedef struct {
    float x, y, w, h;
    u32 color;
} Button;

static Button buttons[] = {
    { 15, 150, 75, 60, 0x3080FFFF },
    {105, 150, 75, 60, 0x3080FFFF },
    {225,  35, 75, 80, 0x30C040FF },
    {225, 145, 75, 65, 0xC03030FF }
};

enum {
    BTN_LEFT,
    BTN_RIGHT,
    BTN_GAS,
    BTN_BRAKE
};

static DVLB_s* shaderDvlb;
static shaderProgram_s shaderProgram;
static C3D_RenderTarget* topTarget;
static C3D_RenderTarget* bottomTarget;
static C3D_Mtx projection;

static int locProjection;
static int locModelView;
static int locColor;

static void* vertexBuffer = NULL;

static float carX = 0.0f;
static float carZ = 0.0f;
static float carAngle = 0.0f;
static float carSpeed = 0.0f;

static C2D_SpriteSheet spriteSheetTop;
static C2D_SpriteSheet spriteSheetBottom;
static C2D_Sprite spriteTop;
static C2D_Sprite spriteBottom;
static bool isTitleScreen = true;

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

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, locModelView, &modelView);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, locColor, ((color >> 24) & 0xFF) / 255.0f, ((color >> 16) & 0xFF) / 255.0f, ((color >> 8)  & 0xFF) / 255.0f, (color & 0xFF) / 255.0f);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawCar(const C3D_Mtx* view) {
    C3D_Mtx carView;
    Mtx_Copy(&carView, view);
    Mtx_Translate(&carView, carX, 0.0f, carZ, true);
    Mtx_RotateY(&carView, carAngle, true);

    drawBox(&carView, 0.0f, 0.55f, 0.0f, 1.8f, 0.55f, 4.2f, 0xB01010FF);
    drawBox(&carView, 0.0f, 1.05f, -0.2f, 1.35f, 0.55f, 2.0f, 0x901010FF);
    drawBox(&carView, 0.0f, 0.75f, 1.45f, 1.55f, 0.25f, 1.25f, 0xB01010FF);
    drawBox(&carView, 0.0f, 1.10f, 0.80f, 1.25f, 0.35f, 0.06f, 0x203050FF);
    drawBox(&carView, 0.0f, 1.10f, -1.20f, 1.25f, 0.35f, 0.06f, 0x203050FF);
    drawBox(&carView, -0.60f, 0.62f, 2.13f, 0.35f, 0.18f, 0.06f, 0xFFEE88FF);
    drawBox(&carView, 0.60f, 0.62f, 2.13f, 0.35f, 0.18f, 0.06f, 0xFFEE88FF);
    drawBox(&carView, -0.60f, 0.62f, -2.13f, 0.35f, 0.18f, 0.06f, 0xFF3030FF);
    drawBox(&carView, 0.60f, 0.62f, -2.13f, 0.35f, 0.18f, 0.06f, 0xFF3030FF);
    drawBox(&carView, -0.95f, 0.35f,  1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f,  1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView, -0.95f, 0.35f, -1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f, -1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
}

static void drawPalm(const C3D_Mtx* view, float x, float z, float scale) {
    drawBox(view, x, 1.25f * scale, z, 0.18f * scale, 2.5f * scale, 0.18f * scale, 0x80502AFF);
    drawBox(view, x - 0.72f * scale, 2.55f * scale, z, 1.55f * scale, 0.12f * scale, 0.24f * scale, 0x267A38FF);
    drawBox(view, x + 0.72f * scale, 2.55f * scale, z, 1.55f * scale, 0.12f * scale, 0.24f * scale, 0x267A38FF);
    drawBox(view, x, 2.55f * scale, z - 0.72f * scale, 0.24f * scale, 0.12f * scale, 1.55f * scale, 0x299344FF);
    drawBox(view, x, 2.55f * scale, z + 0.72f * scale, 0.24f * scale, 0.12f * scale, 1.55f * scale, 0x299344FF);
    drawBox(view, x - 0.42f * scale, 2.32f * scale, z - 0.42f * scale, 0.95f * scale, 0.10f * scale, 0.22f * scale, 0x207A30FF);
    drawBox(view, x + 0.42f * scale, 2.32f * scale, z + 0.42f * scale, 0.95f * scale, 0.10f * scale, 0.22f * scale, 0x207A30FF);
}

static void drawSun(const C3D_Mtx* view, float z) {
    drawBox(view, 0.0f, 9.0f, z, 15.0f, 11.0f, 0.35f, 0xFFB52EFF);
}

static int hitButton(float x, float y) {
    for (int i = 0; i < 4; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].h) {
            return i;
        }
    }
    return -1;
}

static void drawBottomUI(int activeButton) {
    C2D_TargetClear(bottomTarget, C2D_Color32(20, 25, 40, 255));
    C2D_SceneBegin(bottomTarget);

    for (int i = 0; i < 4; i++) {
        Button* b = &buttons[i];
        u32 c = b->color;
        if (i == activeButton) c = 0xFFFFFFFF;
        C2D_DrawRectSolid(b->x, b->y, 0.0f, b->w, b->h, C2D_Color32((c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
        C2D_DrawRectSolid(b->x + 3, b->y + 3, 0.1f, b->w - 6, b->h - 6, C2D_Color32(45, 50, 65, 255));
    }
}

int main(void) {
    gfxInitDefault();
    hidInit();
    romfsInit();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) goto cleanup;
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) goto cleanup;
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!topTarget || !bottomTarget) goto cleanup;

    // Загружаем картинки и ПРИНУДИТЕЛЬНО масштабируем их под экраны
    spriteSheetTop = C2D_SpriteSheetLoad("romfs:/top.t3x");
    if (spriteSheetTop) {
        C2D_SpriteFromSheet(&spriteTop, spriteSheetTop, 0);
        C2D_SpriteSetPos(&spriteTop, 0, 0);
        // ИСПРАВЛЕНО: получаем ширину и высоту напрямую из текстуры
        float scaleX = 400.0f / spriteTop.image.subtex->width;
        float scaleY = 240.0f / spriteTop.image.subtex->height;
        C2D_SpriteSetScale(&spriteTop, scaleX, scaleY);
    }
    
    spriteSheetBottom = C2D_SpriteSheetLoad("romfs:/bottom.t3x");
    if (spriteSheetBottom) {
        C2D_SpriteFromSheet(&spriteBottom, spriteSheetBottom, 0);
        C2D_SpriteSetPos(&spriteBottom, 0, 0);
        // ИСПРАВЛЕНО: получаем ширину и высоту напрямую из текстуры
        float scaleX = 320.0f / spriteBottom.image.subtex->width;
        float scaleY = 240.0f / spriteBottom.image.subtex->height;
        C2D_SpriteSetScale(&spriteBottom, scaleX, scaleY);
    }

    shaderDvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    if (!shaderDvlb) goto cleanup;

    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(&shaderProgram, &shaderDvlb->DVLE[0]);
    C3D_BindProgram(&shaderProgram);

    locProjection = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "projection");
    locModelView = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "modelView");
    locColor = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "uColor");

    vertexBuffer = linearAlloc(sizeof(cubeVerts));
    if (!vertexBuffer) goto cleanup;
    memcpy(vertexBuffer, cubeVerts, sizeof(cubeVerts));

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vertexBuffer, sizeof(Vertex), 1, 0);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    float fov = 50.0f * (PI / 180.0f);
    Mtx_PerspTilt(&projection, fov, 400.0f / 240.0f, 0.1f, 220.0f, false);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    int activeButton = -1;

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_START) {
            if (isTitleScreen) isTitleScreen = false;
            else break;
        }

        touchPosition touch;
        hidTouchRead(&touch);

        if (isTitleScreen) {
            if ((down & KEY_A) || (down & KEY_TOUCH)) {
                isTitleScreen = false;
            }
        } else {
            activeButton = -1;
            if (held & KEY_TOUCH) activeButton = hitButton(touch.px, touch.py);

            bool gas = (activeButton == BTN_GAS) || (held & KEY_A);
            bool brake = (activeButton == BTN_BRAKE) || (held & KEY_B);
            bool left = (activeButton == BTN_LEFT) || (held & KEY_DLEFT);
            bool right = (activeButton == BTN_RIGHT) || (held & KEY_DRIGHT);

            if (gas) carSpeed += CAR_ACCEL;
            if (brake) carSpeed -= CAR_ACCEL * 1.5f;
            if (!gas && !brake) carSpeed *= 0.985f;

            if (carSpeed > CAR_MAX_SPEED) carSpeed = CAR_MAX_SPEED;
            if (carSpeed < -0.35f) carSpeed = -0.35f;

            if (fabsf(carSpeed) > 0.005f) {
                if (left) carAngle += 0.035f * (carSpeed > 0 ? 1.0f : -1.0f);
                if (right) carAngle -= 0.035f * (carSpeed > 0 ? 1.0f : -1.0f);
            }

            carX += sinf(carAngle) * carSpeed;
            carZ += cosf(carAngle) * carSpeed;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
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

typedef struct { float x, y, z; } Vertex;

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

typedef struct { float x, y, w, h; u32 color; } Button;

static Button buttons[] = {
    { 15, 150, 75, 60, 0x3080FFFF },
    {105, 150, 75, 60, 0x3080FFFF },
    {225,  35, 75, 80, 0x30C040FF },
    {225, 145, 75, 65, 0xC03030FF }
};

enum { BTN_LEFT, BTN_RIGHT, BTN_GAS, BTN_BRAKE };

static DVLB_s* shaderDvlb;
static shaderProgram_s shaderProgram;
static C3D_RenderTarget* topTarget;
static C3D_RenderTarget* bottomTarget;
static C3D_Mtx projection;

static int locProjection, locModelView, locColor;
static void* vertexBuffer = NULL;

static float carX = 0.0f, carZ = 0.0f, carAngle = 0.0f, carSpeed = 0.0f;

static C2D_SpriteSheet spriteSheetTop;
static C2D_SpriteSheet spriteSheetBottom;
static C2D_Sprite spriteTop;
static C2D_Sprite spriteBottom;
static bool isTitleScreen = true;

static void drawBox(const C3D_Mtx* view, float x, float y, float z, float sx, float sy, float sz, u32 color) {
    C3D_Mtx modelView;
    Mtx_Copy(&modelView, view);
    Mtx_Translate(&modelView, x, y, z, true);
    Mtx_Scale(&modelView, sx, sy, sz);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, locModelView, &modelView);
    C3D_FVUnifSet(GPU_VERTEX_SHADER, locColor, ((color >> 24) & 0xFF) / 255.0f, ((color >> 16) & 0xFF) / 255.0f, ((color >> 8)  & 0xFF) / 255.0f, (color & 0xFF) / 255.0f);
    C3D_DrawArrays(GPU_TRIANGLES, 0, 36);
}

static void drawCar(const C3D_Mtx* view) {
    C3D_Mtx carView;
    Mtx_Copy(&carView, view);
    Mtx_Translate(&carView, carX, 0.0f, carZ, true);
    Mtx_RotateY(&carView, carAngle, true);

    drawBox(&carView, 0.0f, 0.55f, 0.0f, 1.8f, 0.55f, 4.2f, 0xB01010FF);
    drawBox(&carView, 0.0f, 1.05f, -0.2f, 1.35f, 0.55f, 2.0f, 0x901010FF);
    drawBox(&carView, 0.0f, 0.75f, 1.45f, 1.55f, 0.25f, 1.25f, 0xB01010FF);
    drawBox(&carView, 0.0f, 1.10f, 0.80f, 1.25f, 0.35f, 0.06f, 0x203050FF);
    drawBox(&carView, 0.0f, 1.10f, -1.20f, 1.25f, 0.35f, 0.06f, 0x203050FF);
    drawBox(&carView, -0.60f, 0.62f, 2.13f, 0.35f, 0.18f, 0.06f, 0xFFEE88FF);
    drawBox(&carView, 0.60f, 0.62f, 2.13f, 0.35f, 0.18f, 0.06f, 0xFFEE88FF);
    drawBox(&carView, -0.60f, 0.62f, -2.13f, 0.35f, 0.18f, 0.06f, 0xFF3030FF);
    drawBox(&carView, 0.60f, 0.62f, -2.13f, 0.35f, 0.18f, 0.06f, 0xFF3030FF);
    drawBox(&carView, -0.95f, 0.35f,  1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f,  1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView, -0.95f, 0.35f, -1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
    drawBox(&carView,  0.95f, 0.35f, -1.35f, 0.35f, 0.70f, 0.70f, 0x101010FF);
}

static void drawPalm(const C3D_Mtx* view, float x, float z, float scale) {
    drawBox(view, x, 1.25f * scale, z, 0.18f * scale, 2.5f * scale, 0.18f * scale, 0x80502AFF);
    drawBox(view, x - 0.72f * scale, 2.55f * scale, z, 1.55f * scale, 0.12f * scale, 0.24f * scale, 0x267A38FF);
    drawBox(view, x + 0.72f * scale, 2.55f * scale, z, 1.55f * scale, 0.12f * scale, 0.24f * scale, 0x267A38FF);
    drawBox(view, x, 2.55f * scale, z - 0.72f * scale, 0.24f * scale, 0.12f * scale, 1.55f * scale, 0x299344FF);
    drawBox(view, x, 2.55f * scale, z + 0.72f * scale, 0.24f * scale, 0.12f * scale, 1.55f * scale, 0x299344FF);
    drawBox(view, x - 0.42f * scale, 2.32f * scale, z - 0.42f * scale, 0.95f * scale, 0.10f * scale, 0.22f * scale, 0x207A30FF);
    drawBox(view, x + 0.42f * scale, 2.32f * scale, z + 0.42f * scale, 0.95f * scale, 0.10f * scale, 0.22f * scale, 0x207A30FF);
}

static void drawSun(const C3D_Mtx* view, float z) {
    drawBox(view, 0.0f, 9.0f, z, 15.0f, 11.0f, 0.35f, 0xFFB52EFF);
}

static int hitButton(float x, float y) {
    for (int i = 0; i < 4; i++) {
        if (x >= buttons[i].x && x <= buttons[i].x + buttons[i].w &&
            y >= buttons[i].y && y <= buttons[i].y + buttons[i].h) return i;
    }
    return -1;
}

static void drawBottomUI(int activeButton) {
    C2D_TargetClear(bottomTarget, C2D_Color32(20, 25, 40, 255));
    C2D_SceneBegin(bottomTarget);

    for (int i = 0; i < 4; i++) {
        Button* b = &buttons[i];
        u32 c = b->color;
        if (i == activeButton) c = 0xFFFFFFFF;
        C2D_DrawRectSolid(b->x, b->y, 0.0f, b->w, b->h, C2D_Color32((c >> 24) & 0xFF, (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF));
        C2D_DrawRectSolid(b->x + 3, b->y + 3, 0.1f, b->w - 6, b->h - 6, C2D_Color32(45, 50, 65, 255));
    }
}

int main(void) {
    gfxInitDefault();
    hidInit();
    romfsInit();

    if (!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE)) goto cleanup;
    if (!C2D_Init(C2D_DEFAULT_MAX_OBJECTS)) goto cleanup;
    C2D_Prepare();

    topTarget = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bottomTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!topTarget || !bottomTarget) goto cleanup;

    // Безопасная загрузка спрайтов
    spriteSheetTop = C2D_SpriteSheetLoad("romfs:/top.t3x");
    if (spriteSheetTop) {
        C2D_SpriteFromSheet(&spriteTop, spriteSheetTop, 0);
        C2D_SpriteSetPos(&spriteTop, 0, 0);
        if (spriteTop.image.subtex && spriteTop.image.subtex->width > 0) {
            float scaleX = 400.0f / spriteTop.image.subtex->width;
            float scaleY = 240.0f / spriteTop.image.subtex->height;
            C2D_SpriteSetScale(&spriteTop, scaleX, scaleY);
        }
    }
    
    spriteSheetBottom = C2D_SpriteSheetLoad("romfs:/bottom.t3x");
    if (spriteSheetBottom) {
        C2D_SpriteFromSheet(&spriteBottom, spriteSheetBottom, 0);
        C2D_SpriteSetPos(&spriteBottom, 0, 0);
        if (spriteBottom.image.subtex && spriteBottom.image.subtex->width > 0) {
            float scaleX = 320.0f / spriteBottom.image.subtex->width;
            float scaleY = 240.0f / spriteBottom.image.subtex->height;
            C2D_SpriteSetScale(&spriteBottom, scaleX, scaleY);
        }
    }

    shaderDvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
    if (!shaderDvlb) goto cleanup;

    shaderProgramInit(&shaderProgram);
    shaderProgramSetVsh(&shaderProgram, &shaderDvlb->DVLE[0]);
    C3D_BindProgram(&shaderProgram);

    locProjection = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "projection");
    locModelView = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "modelView");
    locColor = shaderInstanceGetUniformLocation(shaderProgram.vertexShader, "uColor");

    vertexBuffer = linearAlloc(sizeof(cubeVerts));
    if (!vertexBuffer) goto cleanup;
    memcpy(vertexBuffer, cubeVerts, sizeof(cubeVerts));

    C3D_BufInfo* bufInfo = C3D_GetBufInfo();
    BufInfo_Init(bufInfo);
    BufInfo_Add(bufInfo, vertexBuffer, sizeof(Vertex), 1, 0);

    C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attrInfo);
    AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3);

    float fov = 50.0f * (PI / 180.0f);
    Mtx_PerspTilt(&projection, fov, 400.0f / 240.0f, 0.1f, 220.0f, false);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);

    int activeButton = -1;

    while (aptMainLoop()) {
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();

        if (down & KEY_START) {
            if (isTitleScreen) isTitleScreen = false;
            else break;
        }

        touchPosition touch;
        hidTouchRead(&touch);

        if (isTitleScreen) {
            if ((down & KEY_A) || (down & KEY_TOUCH)) isTitleScreen = false;
        } else {
            activeButton = -1;
            if (held & KEY_TOUCH) activeButton = hitButton(touch.px, touch.py);

            bool gas = (activeButton == BTN_GAS) || (held & KEY_A);
            bool brake = (activeButton == BTN_BRAKE) || (held & KEY_B);
            bool left = (activeButton == BTN_LEFT) || (held & KEY_DLEFT);
            bool right = (activeButton == BTN_RIGHT) || (held & KEY_DRIGHT);

            if (gas) carSpeed += CAR_ACCEL;
            if (brake) carSpeed -= CAR_ACCEL * 1.5f;
            if (!gas && !brake) carSpeed *= 0.985f;

            if (carSpeed > CAR_MAX_SPEED) carSpeed = CAR_MAX_SPEED;
            if (carSpeed < -0.35f) carSpeed = -0.35f;

            if (fabsf(carSpeed) > 0.005f) {
                if (left) carAngle += 0.035f * (carSpeed > 0 ? 1.0f : -1.0f);
                if (right) carAngle -= 0.035f * (carSpeed > 0 ? 1.0f : -1.0f);
            }

            carX += sinf(carAngle) * carSpeed;
            carZ += cosf(carAngle) * carSpeed;
        }

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

        if (isTitleScreen) {
            C2D_TargetClear(topTarget, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(topTarget);
            if (spriteSheetTop) C2D_DrawSprite(&spriteTop);

            C2D_TargetClear(bottomTarget, C2D_Color32(0, 0, 0, 255));
            C2D_SceneBegin(bottomTarget);
            if (spriteSheetBottom) C2D_DrawSprite(&spriteBottom);
        } else {
            C2D_TargetClear(topTarget, C2D_Color32(255, 105, 55, 255));
            C2D_SceneBegin(topTarget);
            C2D_DrawRectSolid(0, 0, 0.0f, 400, 55, C2D_Color32(115, 55, 125, 255));
            C2D_DrawRectSolid(0, 55, 0.0f, 400, 45, C2D_Color32(190, 65, 100, 255));
            C2D_DrawRectSolid(0, 100, 0.0f, 400, 45, C2D_Color32(245, 95, 65, 255));
            C2D_DrawRectSolid(0, 145, 0.0f, 400, 45, C2D_Color32(255, 145, 55, 255));

            C3D_Mtx view;
            Mtx_LookAt(&view, FVec3_New(carX - 6.0f * sinf(carAngle), 3.0f, carZ - 6.0f * cosf(carAngle)), FVec3_New(carX, 0.8f, carZ), FVec3_New(0.0f, 1.0f, 0.0f), false);

            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, locProjection, &projection);

            drawBox(&view, 0.0f, -0.08f, carZ + 35.0f, 8.0f, 0.15f, 110.0f, 0x303038FF);
            drawBox(&view, -4.1f, 0.0f, carZ + 35.0f, 0.18f, 0.12f, 110.0f, 0xE8E8E8FF);
            drawBox(&view, 4.1f, 0.0f, carZ + 35.0f, 0.18f, 0.12f, 110.0f, 0xE8E8E8FF);

            float roadOffset = fmodf(carZ, 8.0f);
            if (roadOffset < 0.0f) roadOffset += 8.0f;
            for (int i = 0; i < 16; i++) {
                float markerZ = carZ + 8.0f + i * 8.0f - roadOffset;
                drawBox(&view, 0.0f, 0.025f, markerZ, 0.14f, 0.025f, 3.0f, 0xF0D060FF);
            }

            float palmOffset = fmodf(carZ, 16.0f);
            if (palmOffset < 0.0f) palmOffset += 16.0f;
            for (int i = 0; i < 8; i++) {
                float palmZ = carZ + 12.0f + i * 16.0f - palmOffset;
                float scale = 1.0f - (float)i * 0.055f;
                drawPalm(&view, -6.0f, palmZ, scale);
                drawPalm(&view,  6.0f, palmZ, scale);
            }

            drawSun(&view, carZ + 95.0f);
            drawCar(&view);
            drawBottomUI(activeButton);
        }

        C3D_FrameEnd(0);
    }

cleanup:
    if (vertexBuffer) linearFree(vertexBuffer);
    if (shaderDvlb) DVLB_Free(shaderDvlb);
    if (spriteSheetTop) C2D_SpriteSheetFree(spriteSheetTop);
    if (spriteSheetBottom) C2D_SpriteSheetFree(spriteSheetBottom);
    C2D_Fini();
    C3D_Fini();
    romfsExit();
    gfxExit();
    hidExit();
    return 0;
}

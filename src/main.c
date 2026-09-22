#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>

#include "just_pixels.h"
#include "drawing_tools.h"
#include "dialog.h"

#define INITIAL_WINDOW_WIDTH 1600
#define INITIAL_WINDOW_HEIGHT 900
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

#define CANVAS_INITIAL_WIDTH 64
#define CANVAS_INITIAL_HEIGHT 64
#define CANVAS_INITIAL_ZOOM 10
#define CANVAS_OFFSET_X 90
#define CANVAS_OFFSET_Y 80

#define TOP_WIDGET_HEIGHT 50
#define TOOLS_WIDGET_WIDTH 60
#define BOTTOM_WIDGET_HEIGHT 30
#define PALETTE_WIDGET_WIDTH 200

#define THEME_BG_COLOR 0x222222
#define THEME_CLEAR_COLOR 0xCCCCCC
#define THEME_FONT_COLOR 0xFFFFFF

static Jup_Window *window;
static PixelArray windowPixelArray;
static EditorCanvas editorCanvas;
static Palette palette;
static UIWidget topWidget;
static UIWidget toolsWidget;
static UIDialog dialog;

void drawCanvas(void);
void drawPaletteWidget(void);

void selectPenTool() {
    setDrawingTool(DRAWING_TOOL_PEN);
}

void selectLineTool() {
    setDrawingTool(DRAWING_TOOL_LINE);
}

void selectFillTool() {
    setDrawingTool(DRAWING_TOOL_FILL);
}

EditorCanvas createEditorCanvas() {
    int capacity = CANVAS_INITIAL_WIDTH * CANVAS_INITIAL_HEIGHT;
    uint32_t *canvasPixelData = (uint32_t*)malloc(capacity * sizeof(uint32_t));
    if (canvasPixelData == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory\n.");
        exit(1);
    }
    for (int i = 0; i < capacity; i++) {
        canvasPixelData[i] = 0xFFFFFF;
    }

    uint32_t *toolLayerPixelData = (uint32_t*)calloc(capacity, sizeof(uint32_t));
    if (toolLayerPixelData == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory.\n");
        exit(1);
    }

    Rectangle collider = {
        .x = 0,
        .y = 0,
        .width = CANVAS_INITIAL_WIDTH,
        .height = CANVAS_INITIAL_HEIGHT,
    };

    PixelArray workLayer = (PixelArray) {
        .width = CANVAS_INITIAL_WIDTH,
        .height = CANVAS_INITIAL_HEIGHT,
        .data = canvasPixelData
    };

    PixelArray toolLayer = (PixelArray) {
        .width = CANVAS_INITIAL_WIDTH,
        .height = CANVAS_INITIAL_HEIGHT,
        .data = toolLayerPixelData
    };

    return (EditorCanvas) {
        .capacity = capacity,
        .workLayer = workLayer,
        .toolLayer = toolLayer,
        .zoom = CANVAS_INITIAL_ZOOM,
        .collider = collider,
        .mouseX = -1,
        .mouseY = -1,
    };
}

void freeEditorCanvas(EditorCanvas editorCanvas) {
    free(editorCanvas.toolLayer.data);
    free(editorCanvas.workLayer.data);
}

void loadImage(char *filePath) {
    if (filePath == NULL) {
        fprintf(stderr, "Error: file must not be null.\n");
        exit(1);
    }

    FILE *file =fopen(filePath, "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open the file.\n");
        exit(1);
    }

    char header[3];
    fscanf(file, "%2s", header);
    if (strcmp(header, "P6")) {
        fprintf(stderr, "Error: Invalid file format.\n");
        fclose(file);
        exit(1);
    }

    int width = 0, height = 0;
    if (fscanf(file, "%i %i", &width, &height) != 2) {
        fprintf(stderr, "Error: Invalid file format.\nCould not parse width and height.");
        fclose(file);
        exit(1);
    }
    editorCanvas.collider.width = width;
    editorCanvas.collider.height = height;

    uint32_t *newPointer = (uint32_t*) realloc(editorCanvas.workLayer.data, width * height * sizeof(uint32_t));
    if (newPointer == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory.\n");
        fclose(file);
        exit(1);
    }
    editorCanvas.workLayer.data = newPointer;
    while (getc(file) != '\n');
    while (getc(file) != '\n');
    for (int i = 0; i < width * height; i++) {
        unsigned char rgb[3];
        fread(&rgb, 1, 3, file);
        uint32_t color = (rgb[0] << 16) | (rgb[1] << 8) | rgb[2];
        editorCanvas.workLayer.data[i] = color;

    }

    fclose(file);
}

void savePpm() {
    FILE *file = fopen("./piximage.ppm", "wb");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open image file.\n");
        exit(1);
    };

    fprintf(file, "P6\n%i %i\n255\n", editorCanvas.collider.width, editorCanvas.collider.height);
    for (int i = 0; i < editorCanvas.capacity; i++) {
        unsigned char color[3];
       	color[0] = (editorCanvas.workLayer.data[i] & 0xFF0000) >> 16;
       	color[1] = (editorCanvas.workLayer.data[i] & 0x00FF00) >> 8;
       	color[2] = (editorCanvas.workLayer.data[i] & 0x0000FF) >> 0;
        fwrite(color, 1, 3, file);
    }
    fclose(file);
}

void onMouseClick(float x, float y, int mouseBtn) {
    Dialog_TriggerMouseClicked(&dialog, mouseBtn, x, y);
    if (mouseBtn == 4) {
        editorCanvas.zoom ++;
        return;
    }

    if (mouseBtn == 5) {
        editorCanvas.zoom --;
        return;
    }

    if (mouseBtn != 1) {
        return;
    }

    for (int i = 0; i < topWidget.buttonCount; i ++) {
        if (inRectangle(x, y, topWidget.buttons[i].collider)) {
            // TODO: show dialog
            return;
        }
    }

    if (inRectangle(x, y, toolsWidget.collider)) {
        for (size_t i = 0; i < toolsWidget.buttonCount; i++) {
            if (inRectangle(x, y, toolsWidget.buttons[i].collider)) {
                toolsWidget.buttons[i].onClick();
            }
        }
    }

    for (int i = 0; i < 8; i++) {
        if (inRectangle(x, y, palette.btnColliders[i])) {
            editorCanvas.activeColor = palette.colors[i];
            drawPaletteWidget();
        }
    }
}

void drawCanvas() {
    int minY = TOP_WIDGET_HEIGHT, maxY = window->height - BOTTOM_WIDGET_HEIGHT;
    int minX = TOOLS_WIDGET_WIDTH, maxX = window->width - PALETTE_WIDGET_WIDTH - 1;

    drawRectangle(&windowPixelArray, minX, minY, maxX-minX, maxY-minY, THEME_CLEAR_COLOR);

    for (int y = 0; y < editorCanvas.collider.height; y++) {
        int convertedY = y*editorCanvas.zoom + CANVAS_OFFSET_Y;
        if (convertedY < minY || convertedY + editorCanvas.zoom > maxY)
            continue;

        for (int x = 0; x < editorCanvas.collider.width; x++) {
            uint32_t color, colorToolLayer;
            bool success = getPixel(&editorCanvas.toolLayer, x, y, &colorToolLayer);
            if (!success) {
                fprintf(stderr, "Error: Pixel out of bounds.\n");
                exit(1);
            }
            success = getPixel(&editorCanvas.workLayer, x, y, &color);
            if (!success) {
                fprintf(stderr, "Error: Pixel out of bounds.\n");
                exit(1);
            }
            uint8_t alpha = (colorToolLayer & 0xFF000000) >> 24;
            int convertedX = x*editorCanvas.zoom + CANVAS_OFFSET_X;
            if (convertedX < minX || convertedX + editorCanvas.zoom > maxX)
                continue;

            if (alpha > 0) {
                drawRectangle(
                    &windowPixelArray,
                    x*editorCanvas.zoom + CANVAS_OFFSET_X,
                    y*editorCanvas.zoom + CANVAS_OFFSET_Y,
                    editorCanvas.zoom,
                    editorCanvas.zoom, editorCanvas.activeColor);
            } else {
                drawRectangle(
                    &windowPixelArray,
                    x*editorCanvas.zoom + CANVAS_OFFSET_X,
                    y*editorCanvas.zoom + CANVAS_OFFSET_Y,
                    editorCanvas.zoom,
                    editorCanvas.zoom, color);
            }
        }
    }
}

void createTopWidget() {
    Rectangle collider = {
        .x = 0,
        .y = 0,
        .width = window->width,
        .height = TOP_WIDGET_HEIGHT
    };


    topWidget = (UIWidget) {
        .collider = collider,
        .buttonCount = 1,
    };

    int w = 30, h = 30;
    for (int i = 0; i < topWidget.buttonCount; i ++) {
        topWidget.buttons[0] = (UIButton) {
            .collider = (Rectangle) {
                .x = (i * w) + (4 * i),
                .y = 4,
                .width = w,
                .height = h
            },
        };
    }

    strcpy(topWidget.buttons[0].title,"Resize");
}

void drawTopWidget() {
    drawRectangleRec(&windowPixelArray, topWidget.collider, THEME_BG_COLOR);
    for (int i = 0; i < topWidget.buttonCount; i++) {
        drawRectangleRec(&windowPixelArray, topWidget.buttons[i].collider, 0);
    }
}

void createToolsWidget() {
    int buttonSize = 40;
    int padding = 10;

    toolsWidget.buttonCount = 3;
    for (int i = 0; i < toolsWidget.buttonCount; i++) {
        Rectangle collider = {
            .x = padding,
            .y = padding + TOP_WIDGET_HEIGHT + i*padding + i*buttonSize,
            .width = buttonSize,
            .height = buttonSize
        };
        toolsWidget.buttons[i] = (UIButton) {
            .collider = collider
        };
    }

    sprintf(toolsWidget.buttons[0].title, "Pen");
    toolsWidget.buttons[0].onClick = selectPenTool;
    sprintf(toolsWidget.buttons[1].title, "Line");
    toolsWidget.buttons[1].onClick = selectLineTool;
    sprintf(toolsWidget.buttons[2].title, "Fill");
    toolsWidget.buttons[2].onClick = selectFillTool;

    toolsWidget.collider = (Rectangle) {
        .x = 0,
        .y = TOP_WIDGET_HEIGHT,
        .width = TOOLS_WIDGET_WIDTH,
        .height = window->height - TOP_WIDGET_HEIGHT - BOTTOM_WIDGET_HEIGHT,
    };
}

void drawToolsWidget() {
    drawRectangleRec(&windowPixelArray,
        toolsWidget.collider, THEME_BG_COLOR);
    for (size_t i = 0; i < toolsWidget.buttonCount; i++) {
        UIButton button = toolsWidget.buttons[i];
        drawRectangleRec(&windowPixelArray, button.collider, 0);
        Jup_DrawText(&windowPixelArray, button.collider.x + 4, button.collider.y + 4, button.title, THEME_FONT_COLOR);
    }
}

void drawPaletteWidget() {
    drawRectangle(&windowPixelArray,
        window->width - PALETTE_WIDGET_WIDTH, TOP_WIDGET_HEIGHT,
        PALETTE_WIDGET_WIDTH, window->height - TOP_WIDGET_HEIGHT - BOTTOM_WIDGET_HEIGHT,
        THEME_BG_COLOR);
    int w = 20, h = 20, gap = 4;
    for (int i = 0; i < 8; i++) {
        int x = window->width - (gap + w)*(i+1);
        int y = TOP_WIDGET_HEIGHT + 2;
        if (palette.colors[i] == editorCanvas.activeColor) {
            drawRectangle(&windowPixelArray,
                x-1, y-1, w+2, h+2, 0xFFFFFF);
        }
        palette.btnColliders[i] = (Rectangle) {
            .x = x,
            .y = TOP_WIDGET_HEIGHT + 1,
            .width = w,
            .height = h
        };

        drawRectangleRec(&windowPixelArray, palette.btnColliders[i], palette.colors[i]);
    }
}

void drawCoordinates() {
    char text[32];
    sprintf(text, "x:%i y:%i", editorCanvas.mouseX, editorCanvas.mouseY);
    drawRectangle(&windowPixelArray,
        0, window->height - BOTTOM_WIDGET_HEIGHT, 120, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
    Jup_DrawText(&windowPixelArray, 10, window->height - 19, text, THEME_FONT_COLOR);
}

void drawBottomWidget() {
    drawRectangle(&windowPixelArray,
        0, window->height - BOTTOM_WIDGET_HEIGHT, window->width, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
    drawCoordinates();
}

void drawDialog() {
    Dialog_DrawDialog(&windowPixelArray, &dialog);
}

void onLeftMouseDown() {
    int canvasX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
    int canvasY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;

    drawingToolOnMouseDown(canvasX, canvasY);
}

void onMouseRelease(float, float, int mouseButton) {
    if (mouseButton != 1) {
        return;
    }
    int canvasX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
    int canvasY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;

    drawingToolOnMouseRelease(canvasX, canvasY);
}

void onKeyPressed(int keyCode) {
    Dialog_TriggerKeyPressed(&dialog, keyCode, window->shiftPressed);
}

int main (int argc, char *argv[]) {
    uint32_t *pixels = (uint32_t*)calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));

    Jup_CreateWindowArgs create_window_args = {
        .width = INITIAL_WINDOW_WIDTH,
        .height = INITIAL_WINDOW_HEIGHT,
        .windowTitle = (char *)"Pixelart Editor",
        .frameBuffer = pixels,
        .onClick = onMouseClick,
        .onMouseRelease = onMouseRelease,
        .onKeyPressed = onKeyPressed
    };
    window = Jup_CreateWindow(create_window_args);
    windowPixelArray = (PixelArray) {
        .width = INITIAL_WINDOW_WIDTH,
        .height = INITIAL_WINDOW_HEIGHT,
        .data = pixels
    };

    editorCanvas = createEditorCanvas();
    if (argc > 1) {
        loadImage(argv[1]);
    }

    palette.colors[0] = 0x000000;
    palette.colors[1] = 0xFF0000;
    palette.colors[2] = 0x00FF00;
    palette.colors[3] = 0x0000FF;
    palette.colors[4] = 0xFFFF00;
    palette.colors[5] = 0xFF00FF;
    palette.colors[6] = 0x00FFFF;
    palette.colors[7] = 0xFFFFFF;
    editorCanvas.activeColor = 0x000000;

    initializeDrawingTools(&editorCanvas);
    setDrawingTool(DRAWING_TOOL_PEN);


    dialog = Dialog_CreateDialog((char *) "Dialog Title foo", 2);
    Dialog_AddInput(&dialog, (UITextInput){
        .label = "Input 0",
        .value = "Hello"
    });
    Dialog_AddInput(&dialog, (UITextInput){
        .label = "Input 1",
        .value = ""
    });

    // full clear
    for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
        pixels[i] = THEME_CLEAR_COLOR;
    }

    createTopWidget();
    drawTopWidget();
    drawBottomWidget();

    createToolsWidget();
    drawToolsWidget();

    drawPaletteWidget();

    struct timespec requestedTime = { .tv_sec = 0, .tv_nsec = 16666667 };
    struct timespec remainingTime;

    while (!Jup_WindowShouldClose(window)) {
        if (window->mouseDown[1]) {
            onLeftMouseDown();
        }

        int convertedMouseX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
        int convertedMouseY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;
        if (convertedMouseX >= editorCanvas.collider.width) convertedMouseX = editorCanvas.collider.width - 1;
        if (convertedMouseY >= editorCanvas.collider.height) convertedMouseY = editorCanvas.collider.height - 1;
        if (convertedMouseX < 0) editorCanvas.mouseX = 0;
        if (convertedMouseY < 0) editorCanvas.mouseY = 0;
        editorCanvas.mouseX = convertedMouseX;
        editorCanvas.mouseY = convertedMouseY;

        drawCoordinates();
        drawCanvas();
        drawDialog();

        Jup_DrawPixels(window);

        nanosleep(&requestedTime, &remainingTime);
    }

    freeEditorCanvas(editorCanvas);
    Dialog_FreeDialog(&dialog);
    Jup_FreeAndClose(window);
    return 0;
}

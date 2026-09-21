#include <stdlib.h>
#include <stdio.h>
#include "drawing_tools.h"

typedef struct {
    void (*onMouseDown)(int x, int y);
    void (*onMouseRelease)(int x, int y);
    void (*onMouseClick)(int x, int y);
} ToolState;

typedef struct {
    ToolState idle;
    ToolState drawing;
    ToolState drawingOutside;
} DrawingTool;

static EditorCanvas *editorCanvas;
static DrawingTool penTool, lineTool, fillTool;
static ToolState toolState;
static int prevX = 0, prevY = 0;
static int endX = 0, endY = 0;

void setDrawingTool(enum DrawingToolType drawingTool) {
    if (drawingTool == DRAWING_TOOL_PEN) {
        toolState = penTool.idle;
    } else if(drawingTool == DRAWING_TOOL_LINE) {
        toolState = lineTool.idle;
    } else if (drawingTool == DRAWING_TOOL_FILL) {
        toolState = fillTool.idle;
    }
}

void drawingToolOnMouseDown(int x, int y) {
    if (toolState.onMouseDown) toolState.onMouseDown(x, y);
}

void drawingToolOnMouseRelease(int x, int y) {
    if (toolState.onMouseRelease) toolState.onMouseRelease(x, y);
}

void drawingToolOnMouseClick(int x, int y) {
    if (toolState.onMouseClick) toolState.onMouseClick(x, y);
}

// Pen Tool Idle
void penTool_Idle_OnMouseDown(int x, int y) {
    setPixel(&editorCanvas->workLayer, x, y, editorCanvas->activeColor);
    prevX = x; prevY = y;
    toolState = penTool.drawing;
}

// Pen Tool Drawing
void penTool_Drawing_OnMouseDown(int x, int y) {
    drawLine(&editorCanvas->workLayer, prevX, prevY, x, y, editorCanvas->activeColor);
    prevX = x; prevY = y;
}

void penTool_Drawing_OnMouseRelease(int, int) {
    toolState = penTool.idle;
}

// Line Tool Idle
void lineTool_Idle_OnMouseDown(int x, int y) {
    prevX = x; prevY = y;
    toolState = lineTool.drawing;
}

// Line Tool Drawing
void lineTool_Drawing_OnMouseDown(int x, int y) {
    for (int i = 0; i < editorCanvas->collider.width * editorCanvas->collider.height; i++) {
        editorCanvas->toolLayer.data[i] = 0;
    }

    drawLine(&editorCanvas->toolLayer, prevX, prevY, x, y, 0xFFFFFFFF);
    endX = x; endY = y;
}

void lineTool_Drawing_OnMouseRelease(int, int) {
    drawLine(&editorCanvas->workLayer, prevX, prevY, endX, endY, editorCanvas->activeColor);
    for (int i = 0; i < editorCanvas->collider.width * editorCanvas->collider.height; i++) {
        editorCanvas->toolLayer.data[i] = 0;
    }
    toolState = lineTool.idle;
}

void floodFillSelect(PixelArray *pixels, int x, int y, uint32_t color, bool *selection, size_t *selectionLength) {
    if (x < 0 || x >= (int)pixels->width || y < 0 || y >= (int)pixels->height)
        return;

    int index = x + y * pixels->width;
    if (selection[index])
        return;

    uint32_t c;
    bool success = getPixel(pixels, x, y, &c);
    if (!success)
        return;
    if (c != color)
        return;

    selection[index] = true;
    *selectionLength = *selectionLength + 1;
    floodFillSelect(pixels, x-1, y, color, selection, selectionLength);
    floodFillSelect(pixels, x+1, y, color, selection, selectionLength);
    floodFillSelect(pixels, x, y+1, color, selection, selectionLength);
    floodFillSelect(pixels, x, y-1, color, selection, selectionLength);
}

// Fill Tool Idle
void fillTool_Idle_OnMouseDown(int x, int y) {
    if (!inRectangle(x, y, editorCanvas->collider))
        return;

    bool *selection = (bool*)calloc(editorCanvas->collider.width * editorCanvas->collider.height, sizeof(bool));
    if (selection == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(1);
    }
    toolState = fillTool.drawing;
    size_t selectionLength = 0;
    uint32_t color;
    bool success = getPixel(&editorCanvas->workLayer, x, y, &color);
    if (!success) return;

    floodFillSelect(&editorCanvas->workLayer, x, y, color, selection, &selectionLength);

    for (int i = 0; i < editorCanvas->collider.width * editorCanvas->collider.height; i++) {
        if (selection[i]) {
            editorCanvas->workLayer.data[i] = editorCanvas->activeColor;
        }
    }

    free(selection);
}

void fillTool_Drawing_OnMouseDown(int, int) {}

void fillTool_Drawing_OnMouseRelease(int, int) {
    toolState = fillTool.idle;
}

void initializeDrawingTools(EditorCanvas *_editorCanvas) {
    editorCanvas = _editorCanvas;

    penTool = (DrawingTool) {
        .idle = {
            .onMouseDown = penTool_Idle_OnMouseDown
        },
        .drawing = {
            .onMouseDown = penTool_Drawing_OnMouseDown,
            .onMouseRelease = penTool_Drawing_OnMouseRelease
        }
    };

    lineTool = (DrawingTool) {
        .idle = {
            .onMouseDown = lineTool_Idle_OnMouseDown
        },
        .drawing = {
            .onMouseDown = lineTool_Drawing_OnMouseDown,
            .onMouseRelease = lineTool_Drawing_OnMouseRelease
        }
    };

    fillTool = (DrawingTool) {
        .idle = {
            .onMouseDown = fillTool_Idle_OnMouseDown
        },
        .drawing = {
            .onMouseDown = fillTool_Drawing_OnMouseDown,
            .onMouseRelease = fillTool_Drawing_OnMouseRelease
        }
    };
    toolState = penTool.idle;
}

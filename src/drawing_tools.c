#include "util.h"
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
	if (!inRectangle(x, y, editorCanvas->collider))
	    return;

	setPixel(&editorCanvas->workLayer, x, y, editorCanvas->activeColor);
	prevX = x; prevY = y;
	toolState = penTool.drawing;
}

// Pen Tool Drawing
void penTool_Drawing_OnMouseDown(int x, int y) {
	if (!inRectangle(x, y, editorCanvas->collider)) {
		drawLine(&editorCanvas->workLayer, prevX, prevY, x, y, editorCanvas->activeColor);
		prevX = x; prevY = y;
		toolState = penTool.drawingOutside;
		return;
	}

	drawLine(&editorCanvas->workLayer, prevX, prevY, x, y, editorCanvas->activeColor);
	prevX = x; prevY = y;
}

void penTool_Drawing_OnMouseRelease(int, int) {
	toolState = penTool.idle;
}

// Pen Tool Drawing Outside
void penTool_DrawingOutside_OnMouseDown(int x, int y) {
	if (inRectangle(x, y, editorCanvas->collider)) {
	        toolState = penTool.drawing;
			return;
	}
	prevX = x; prevY = y;
}

void penTool_DrawingOutside_OnMouseRelease(int, int) {
	toolState = penTool.idle;
}

// Line Tool Idle
void lineTool_Idle_OnMouseDown(int x, int y) {
	if (!inRectangle(x, y, editorCanvas->collider))
		return;

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

void initializeDrawingTools(EditorCanvas *_editorCanvas) {
	editorCanvas = _editorCanvas;

	penTool = (DrawingTool) {
		.idle = {
			.onMouseDown = penTool_Idle_OnMouseDown
		},
		.drawing = {
			.onMouseDown = penTool_Drawing_OnMouseDown,
			.onMouseRelease = penTool_Drawing_OnMouseRelease
		},
		.drawingOutside = {
			.onMouseDown = penTool_DrawingOutside_OnMouseDown,
			.onMouseRelease = penTool_DrawingOutside_OnMouseRelease
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

	/*
	fillTool = (DrawingTool) {
		.idle = {
			.onMouseDown = fillTool_Idle_OnMouseDown,
			.onMouseRelease = fillTool_Idle_OnMouseRelease
		},
		.drawing = {
			.onMouseDown = fillTool_Drawing_OnMouseDown,
			.onMouseRelease = fillTool_Drawing_OnMouseRelease
		},
		.drawingOutside = {
			.onMouseDown = fillTool_DrawingOutside_OnMouseDown,
			.onMouseRelease = fillTool_DrawingOutside_OnMouseRelease
		}
	}; */
	toolState = penTool.idle;
}

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>

#define JUST_PIXELS_IMPLEMENTATION
#include "just_pixels.h"
#include "util.h"
#include "drawing_tools.h"

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
static TopBar topBar;
static UIWidget toolsWidget;

void drawCanvas(void);
void drawPaletteWidget(void);

/*
void penToolOnMouseDown( int x, int y) {
	if ((editorCanvas.prevX < 0 || editorCanvas.prevY < 0) && inRectangle(x, y, editorCanvas.collider)) {
		// first point
		setPixel(editorCanvas.pixels, x, y, editorCanvas.activeColor);
		editorCanvas.prevX = x; editorCanvas.prevY = y;
		return;
	}

	if (!inRectangle(x, y, editorCanvas.collider)) {
		// drew outside
		int _x;
		if (x >= editorCanvas.collider.width) _x = editorCanvas.collider.width;
		if (x < 0) _x = 0;
		if (y >= editorCanvas.collider.
		drawLine(editorCanvas.pixels, editorCanvas.prevX, editorCanvas.prevY, x, y, editorCanvas.activeColor);
		editorCanvas.prevX = -1; editorCanvas.prevY = -1;
		return;
	}

	drawLine(editorCanvas.pixels, editorCanvas.prevX, editorCanvas.prevY, x, y, editorCanvas.activeColor);
	editorCanvas.prevX = x; editorCanvas.prevY = y;
}

void penToolOnMouseRelease(int, int) {
	editorCanvas.prevX = -1;
	editorCanvas.prevY = -1;
}

void lineToolOnMouseDown(int x, int y) {
	if (!inRectangle(x, y, editorCanvas.collider)) {
		drawLine(editorCanvas.toolLayer, editorCanvas.prevX, editorCanvas.prevY, x, y, 0xFFFFFFFF);
		editorCanvas.prevX = -1;
		editorCanvas.prevY = -1;
		return;
	}

	if (editorCanvas.prevX < 0 || editorCanvas.prevY < 0) {
		editorCanvas.prevX = x; editorCanvas.prevY = y;
		return;
	}

	for (int i = 0; i < editorCanvas.collider.width * editorCanvas.collider.height; i++) {
		editorCanvas.toolLayer[i] = 0;
	}

	drawLine(editorCanvas.toolLayer, editorCanvas.prevX, editorCanvas.prevY, x, y, 0xFFFFFFFF);
	editorCanvas.endX = x;
	editorCanvas.endY = y;
}

void lineToolOnMouseRelease(int x, int y) {
	if (editorCanvas.prevX < 0 || editorCanvas.prevY < 0) return;
	drawLine(editorCanvas.pixels, editorCanvas.prevX, editorCanvas.prevY, editorCanvas.endX, editorCanvas.endY, editorCanvas.activeColor);
	for (int i = 0; i < editorCanvas.collider.width * editorCanvas.collider.height; i++) {
		editorCanvas.toolLayer[i] = 0;
	}
	editorCanvas.prevX = -1; editorCanvas.prevY = -1;

}

void floodFillSelect(uint32_t *frameBuffer, Rectangle dimensions, int x, int y, uint32_t color, bool *selection, size_t *selectionLength) {
	if (x < 0 || x >= dimensions.width || y < 0 || y >= dimensions.height)
		return;

	int index = x + y * dimensions.width;
	if (selection[index])
		return;

	uint32_t c;
	bool success = getPixel(frameBuffer, x, y, &c);
	if (!success)
		return;
	if (c != color)
		return;

	selection[index] = true;
	*selectionLength = *selectionLength + 1;
	floodFillSelect(frameBuffer, dimensions, x-1, y, color, selection, selectionLength);
	floodFillSelect(frameBuffer, dimensions, x+1, y, color, selection, selectionLength);
	floodFillSelect(frameBuffer, dimensions, x, y+1, color, selection, selectionLength);
	floodFillSelect(frameBuffer, dimensions, x, y-1, color, selection, selectionLength);
}

void fillToolOnMouseDown(int x, int y) {
	if (editorCanvas.prevX >= 0 || editorCanvas.prevY >= 0) return;
	editorCanvas.prevX = x;
	editorCanvas.prevY = y;
	bool *selection = calloc(editorCanvas.collider.width * editorCanvas.collider.height, sizeof(bool));
	if (selection == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		exit(1);
	}
	size_t selectionLength = 0;
	uint32_t color;
	bool success = getPixel(editorCanvas.pixels, x, y, &color);
	if (!success) return;

	floodFillSelect(editorCanvas.pixels, editorCanvas.collider, x, y, color, selection, &selectionLength);

	for (int i = 0; i < editorCanvas.collider.width * editorCanvas.collider.height; i++) {
		if (selection[i]) {
			editorCanvas.pixels[i] = editorCanvas.activeColor;
		}
	}

	free(selection);
}

void fillToolOnMouseRelease() {
	editorCanvas.prevX = -1;
	editorCanvas.prevY = -1;
} */

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

void onMouseClick(float x, float y, int mouse_btn) {
	if (mouse_btn == 4) {
		editorCanvas.zoom ++;
		return;
	}

	if (mouse_btn == 5) {
		editorCanvas.zoom --;
		return;
	}

	if (mouse_btn != 1) {
		return;
	}

	if (inRectangle(x, y, topBar.boundary)) {
		savePpm();
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
		toolsWidget.buttons[i] = (Button) {
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
	drawRectangleRect(&windowPixelArray,
			toolsWidget.collider, THEME_BG_COLOR);
	for (size_t i = 0; i < toolsWidget.buttonCount; i++) {
		Button button = toolsWidget.buttons[i];
		drawRectangleRect(&windowPixelArray, button.collider, 0);
		Jup_DrawText(window, button.collider.x + 4, button.collider.y + 4, button.title, THEME_FONT_COLOR);
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

		drawRectangleRect(&windowPixelArray, palette.btnColliders[i], palette.colors[i]);
	}
}

void drawCoordinates() {
	char text[32];
	sprintf(text, "x:%i y:%i", editorCanvas.mouseX, editorCanvas.mouseY);
	drawRectangle(&windowPixelArray,
			0, window->height - BOTTOM_WIDGET_HEIGHT, 120, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
	Jup_DrawText(window, 10, window->height - 19, text, THEME_FONT_COLOR);
}

void drawBottomWidget() {
	drawRectangle(&windowPixelArray,
			0, window->height - BOTTOM_WIDGET_HEIGHT, window->width, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
	drawCoordinates();
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

int main (int argc, char *argv[]) {
	uint32_t *pixels = (uint32_t*)calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));

	Jup_CreateWindowArgs create_window_args = {
		.width = INITIAL_WINDOW_WIDTH,
		.height = INITIAL_WINDOW_HEIGHT,
		.windowTitle = "Pixelart Editor",
		.frameBuffer = pixels,
		.onClick = onMouseClick,
		.onMouseRelease = onMouseRelease
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

	topBar.boundary = (Rectangle) {
		.x = 0, .y = 0, .width = window->width, .height = TOP_WIDGET_HEIGHT
	};

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

	// full clear
	for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
		pixels[i] = THEME_CLEAR_COLOR;
	}

	drawRectangleRect(&windowPixelArray, topBar.boundary, 0x999999);
	createToolsWidget();

	drawBottomWidget();
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
		Jup_DrawPixels(window);

		nanosleep(&requestedTime, &remainingTime);
	}

	freeEditorCanvas(editorCanvas);
	Jup_FreeAndClose(window);
	return 0;
}

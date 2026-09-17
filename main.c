#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <inttypes.h>

#define JUST_PIXELS_IMPLEMENTATION
#include "just_pixels.h"

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
#define BOTTOM_WIDGET_HEIGHT 30

#define THEME_BG_COLOR 0x222222
#define THEME_FONT_COLOR 0xFFFFFF

typedef struct {
	int x;
	int y;
	int width;
	int height;
} Rectangle;

typedef struct {
	int capacity;
	uint32_t *pixels;
	uint32_t *toolLayer;
	int zoom;
	Rectangle collider;
	uint32_t activeColor;
	int mouseX;
	int mouseY;
	int prevX;
	int prevY;
	int endX;
	int endY;
	void (*drawToolOnMouseDown)(int x, int y);
	void (*drawToolOnMouseRelease)(int x, int y);
} EditorCanvas;

typedef struct {
	uint32_t colors[8];
	Rectangle btnColliders[8];
	Rectangle boundary;
} Palette;

typedef struct {
	Rectangle boundary;
} TopBar;

typedef struct {
	char title[16];
	Rectangle collider;
	void (*onClick)(void);
} Button;

typedef struct {
	Rectangle collider;
	size_t buttonCount;
	Button buttons[32];
} UIWidget;

static Jup_Window *window;
static EditorCanvas editorCanvas;
static Palette palette;
static TopBar topBar;
static UIWidget toolsWidget;

void drawLine(uint32_t *frameBuffer, int x0, int y0, int x1, int y1, uint32_t color);
void setEditorCanvasPixel(uint32_t *frameBuffer, int x, int y, uint32_t color);
bool getPixel(uint32_t *pixels, int x, int y, uint32_t *color);
void drawEditorCanvas(void);

bool inRectangle(int x, int y, Rectangle rectangle) {
	if (x > rectangle.x + rectangle.width ||
			y > rectangle.y + rectangle.height ||
			x < rectangle.x || y < rectangle.y) {
		return false;
	}

	return true;
}

void penToolOnMouseDown( int x, int y) {
	if (editorCanvas.prevX < 0 || editorCanvas.prevY < 0) {
		setEditorCanvasPixel(editorCanvas.pixels, x, y, editorCanvas.activeColor);
		editorCanvas.prevX = x; editorCanvas.prevY = y;
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
	drawEditorCanvas();
}

void fillToolOnMouseRelease() {
	editorCanvas.prevX = -1;
	editorCanvas.prevY = -1;
}

void selectPenTool() {
	editorCanvas.drawToolOnMouseDown = penToolOnMouseDown;
	editorCanvas.drawToolOnMouseRelease = penToolOnMouseRelease;
}

void selectLineTool() {
	editorCanvas.drawToolOnMouseDown = lineToolOnMouseDown;
	editorCanvas.drawToolOnMouseRelease = lineToolOnMouseRelease;
}

void selectFillTool() {
	editorCanvas.drawToolOnMouseDown = fillToolOnMouseDown;
	editorCanvas.drawToolOnMouseRelease = fillToolOnMouseRelease;
}

EditorCanvas createEditorCanvas() {
	int capacity = CANVAS_INITIAL_WIDTH * CANVAS_INITIAL_HEIGHT;
	uint32_t *canvasPixels = malloc(capacity * sizeof(uint32_t));
	if (canvasPixels == NULL) {
		fprintf(stderr, "Error: Failed to allocate memory\n.");
		exit(1);
	}
	for (int i = 0; i < capacity; i++) {
		canvasPixels[i] = 0xFFFFFF;
	}

	uint32_t *toolLayer = calloc(capacity, sizeof(uint32_t));
	if (toolLayer == NULL) {
		fprintf(stderr, "Error: Failed to allocate memory.\n");
		exit(1);
	}

	Rectangle collider = {
		.x = 0,
		.y = 0,
		.width = CANVAS_INITIAL_WIDTH,
		.height = CANVAS_INITIAL_HEIGHT,
	};

	return (EditorCanvas) {
		.collider = collider,
			.capacity = capacity,
			.pixels = canvasPixels,
			.toolLayer = toolLayer,
			.zoom = CANVAS_INITIAL_ZOOM,
			.mouseX = -1,
			.mouseY = -1,
			.prevX = -1,
			.prevY = -1,
			.drawToolOnMouseDown = lineToolOnMouseDown,
			.drawToolOnMouseRelease = lineToolOnMouseRelease
	};
}

void freeEditorCanvas(EditorCanvas editorCanvas) {
	free(editorCanvas.pixels);
}

bool getPixel(uint32_t *pixels, int x, int y, uint32_t *color) {
	if (x > editorCanvas.collider.width || x < 0 || y > editorCanvas.collider.height || y < 0) {
		return false;
	}
	*color = pixels[x + y * editorCanvas.collider.width];
	return true;
}

void setEditorCanvasPixel(uint32_t *frameBuffer, int x, int y, uint32_t color) {
	if (x >= editorCanvas.collider.width || y >= editorCanvas.collider.height || x < 0 || y < 0)
		return;
	frameBuffer[x + y * editorCanvas.collider.width] = color;
}

void drawRectangle(uint32_t *frameBuffer, int x, int y, int width, int height, uint32_t color) {
	// TODO: out of bounds check
	for (int _y = y; _y < y + height; _y++) {
		for (int _x = x; _x < x + width; _x++) {
			frameBuffer[_x + _y * window->width] = color;
		}
	}
}

void drawRectangleRect(uint32_t *frameBuffer, Rectangle rectangle, uint32_t color) {
	for (int y = rectangle.y; y < rectangle.y + rectangle.height; y++) {
		for (int x = rectangle.x; x < rectangle.x + rectangle.width; x++) {
			frameBuffer[x + y * window->width] = color;
		}
	}
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

	uint32_t *newPointer = realloc(editorCanvas.pixels, width * height * sizeof(uint32_t));
	if (newPointer == NULL) {
		fprintf(stderr, "Error: Failed to allocate memory.\n");
		fclose(file);
		exit(1);
	}
	editorCanvas.pixels = newPointer;
	while (getc(file) != '\n');
	while (getc(file) != '\n');
	for (int i = 0; i < width * height; i++) {
		unsigned char rgb[3];
		fread(&rgb, 1, 3, file);
		uint32_t color = (rgb[0] << 16) | (rgb[1] << 8) | rgb[2];
		editorCanvas.pixels[i] = color;

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
            	color[0] = (editorCanvas.pixels[i] & 0xFF0000) >> 16;
            	color[1] = (editorCanvas.pixels[i] & 0x00FF00) >> 8;
            	color[2] = (editorCanvas.pixels[i] & 0x0000FF) >> 0;
		fwrite(color, 1, 3, file);
	}
	fclose(file);
}

void onWindowClick(float x, float y, int mouse_btn) {
	if (mouse_btn != 1) {
		return;
	}

	if (inRectangle(x, y, topBar.boundary)) {
		savePpm();
	}

	if (inRectangle(x, y, toolsWidget.collider)) {
		for (int i = 0; i < toolsWidget.buttonCount; i++) {
			if (inRectangle(x, y, toolsWidget.buttons[i].collider)) {
				toolsWidget.buttons[i].onClick();
			}
		}
	}

	for (int i = 0; i < 8; i++) {
		if (inRectangle(x, y, palette.btnColliders[i])) {
			editorCanvas.activeColor = palette.colors[i];
		}
	}
	
}

void drawEditorCanvas() {
	for (int y = 0; y < editorCanvas.collider.height; y++) {
		for (int x = 0; x < editorCanvas.collider.width; x++) {
			uint32_t color, colorToolLayer;
			bool success = getPixel(editorCanvas.toolLayer, x, y, &colorToolLayer);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			success = getPixel(editorCanvas.pixels, x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			uint8_t alpha = (colorToolLayer & 0xFF000000) >> 24;
			if (alpha > 0) {
				drawRectangle(
						window->frameBuffer,
						x*editorCanvas.zoom + CANVAS_OFFSET_X,
						y*editorCanvas.zoom + CANVAS_OFFSET_Y,
						editorCanvas.zoom,
						editorCanvas.zoom, editorCanvas.activeColor);
			} else {
				drawRectangle(
						window->frameBuffer,
						x*editorCanvas.zoom + CANVAS_OFFSET_X,
						y*editorCanvas.zoom + CANVAS_OFFSET_Y,
						editorCanvas.zoom,
						editorCanvas.zoom, color);
			}
		}
	}
}

void createToolsWidget() {
	int width = 60;
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
		.width = width,
		.height = window->height - TOP_WIDGET_HEIGHT - BOTTOM_WIDGET_HEIGHT,
	};
}

void drawToolsWidget() {
	drawRectangleRect(window->frameBuffer,
			toolsWidget.collider, THEME_BG_COLOR);
	for (int i = 0; i < toolsWidget.buttonCount; i++) {
		Button button = toolsWidget.buttons[i];
		drawRectangleRect(window->frameBuffer, button.collider, 0);
		Jup_DrawText(window, button.collider.x + 4, button.collider.y + 4, button.title, THEME_FONT_COLOR);
	}
}

void swapPoints(int *x0, int *y0, int *x1, int *y1) {
	int temp = *x0;
	*x0 = *x1;
	*x1 = temp;
	temp = *y0;
	*y0 = *y1;
	*y1 = temp;
}

void drawLine(uint32_t *frameBuffer, int x0, int y0, int x1, int y1, uint32_t color) {
	if (abs(x1 - x0) >= abs(y1 - y0)) {
		if (x1 < x0) swapPoints(&x0, &y0, &x1, &y1);
		float slope = (x1-x0 == 0) ? 0 : (float)(y1 - y0) / (float)(x1 - x0);
		for (int x = 0; x <= (x1-x0); x++) {
			setEditorCanvasPixel(frameBuffer, x0 + x, y0 + roundf(x*slope), color);
		}
	} else {
		if (y1 < y0) swapPoints(&x0, &y0, &x1, &y1);
		float slope = (y1-y0 == 0) ? 0 : (float)(x1 - x0) / (float)(y1 - y0);
		for (int y = 0; y <= (y1-y0); y++) {
			setEditorCanvasPixel(frameBuffer, x0 + roundf(y*slope), y0 + y, color);
		}
	}
}

void drawCoordinates() {
	char text[32];
	sprintf(text, "x:%i y:%i", editorCanvas.mouseX, editorCanvas.mouseY);
	drawRectangle(window->frameBuffer,
			0, window->height - BOTTOM_WIDGET_HEIGHT, 120, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
	Jup_DrawText(window, 10, window->height - 19, text, THEME_FONT_COLOR);
}

void drawBottomWidget() {
	drawRectangle(window->frameBuffer,
			0, window->height - BOTTOM_WIDGET_HEIGHT, window->width, BOTTOM_WIDGET_HEIGHT, THEME_BG_COLOR);
	drawCoordinates();
}

void onLeftMouseDown() {
	int canvasX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
	int canvasY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;
	if (!inRectangle(canvasX, canvasY, editorCanvas.collider)) {
		return;
	}

	editorCanvas.drawToolOnMouseDown(canvasX, canvasY);
	drawEditorCanvas();
}

void onMouseRelease(float x, float y, int mouseButton) {
	if (mouseButton != 1) {
		return;
	}
	int canvasX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
	int canvasY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;
	editorCanvas.drawToolOnMouseRelease(canvasX, canvasY);
	editorCanvas.prevX = -1; editorCanvas.prevY = -1;
}

int main (int argc, char *argv[]) {
	uint32_t *frameBuffer = calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));
	for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
		frameBuffer[i] = 0xCCCCCC;
	}

	Jup_CreateWindowArgs create_window_args = {
		.width = INITIAL_WINDOW_WIDTH,
		.height = INITIAL_WINDOW_HEIGHT,
		.windowTitle = "Pixelart Editor",
		.frameBuffer = frameBuffer,
		.onClick = onWindowClick,
		.onMouseRelease = onMouseRelease
	};
	window = Jup_CreateWindow(create_window_args);

	editorCanvas = createEditorCanvas();
	if (argc > 1) {
		loadImage(argv[1]);
	}
	drawEditorCanvas();

	palette.colors[0] = 0x000000;
	palette.colors[1] = 0xFF0000;
	palette.colors[2] = 0x00FF00;
	palette.colors[3] = 0x0000FF;
	palette.colors[4] = 0xFFFF00;
	palette.colors[5] = 0xFF00FF;
	palette.colors[6] = 0x00FFFF;
	palette.colors[7] = 0xFFFFFF;
	editorCanvas.activeColor = 0x000000;

	for (int i = 0; i < 8; i++) {
		palette.btnColliders[i] = (Rectangle) {
			.x = window->width - (4 + 20)*(i+1),
				.y = TOP_WIDGET_HEIGHT,
				.width = 20,
				.height = 20
		};

		drawRectangleRect(window->frameBuffer, palette.btnColliders[i], palette.colors[i]);
	}

	topBar.boundary = (Rectangle) {
		.x = 0, .y = 0, .width = window->width, .height = TOP_WIDGET_HEIGHT
	};
	drawRectangleRect(window->frameBuffer, topBar.boundary, 0x999999);
	drawBottomWidget();
	createToolsWidget();

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
		drawToolsWidget();
		Jup_DrawPixels(window);

		nanosleep(&requestedTime, &remainingTime);
	}

	freeEditorCanvas(editorCanvas);
	Jup_FreeAndClose(window);
	return 0;
}

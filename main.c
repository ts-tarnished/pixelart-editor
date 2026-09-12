#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define JUST_PIXELS_IMPLEMENTATION
#include "just_pixels.h"

#define INITIAL_WINDOW_WIDTH 800
#define INITIAL_WINDOW_HEIGHT 600
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

#define CANVAS_INITIAL_WIDTH 30
#define CANVAS_INITIAL_HEIGHT 30
#define CANVAS_INITIAL_ZOOM 10

typedef struct {
	int x;
	int y;
	int width;
	int height;
} Rectangle;

typedef struct {
	int capacity;
	uint32_t *pixels;
	int zoom;
	Rectangle collider;
	uint32_t activeColor;
} EditorCanvas;

typedef struct {
	uint32_t colors[8];
	Rectangle colliders[8];
} Palette;

static Jup_Window *window;
static EditorCanvas editorCanvas;
static Palette palette;

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
			.zoom = CANVAS_INITIAL_ZOOM
	};
}

void freeEditorCanvas(EditorCanvas editorCanvas) {
	free(editorCanvas.pixels);
}

bool getEditorCanvasPixel(int x, int y, uint32_t *color) {
	if (x > editorCanvas.collider.width || x < 0 || y > editorCanvas.collider.height || y < 0) {
		return false;
	}
	*color = editorCanvas.pixels[x + y * editorCanvas.collider.width];
	return true;
}

void setEditorCanvasPixel(int x, int y, uint32_t color) {
	if (x >= editorCanvas.collider.width || y >= editorCanvas.collider.height || x < 0 || y < 0)
		return;
	editorCanvas.pixels[x + y * editorCanvas.collider.width] = color;
}

void drawRectangle(int x, int y, int width, int height, uint32_t color) {
	// TODO: out of bounds check
	for (int _y = y; _y < y + height; _y++) {
		for (int _x = x; _x < x + width; _x++) {
			window->frameBuffer[_x + _y * window->width] = color;
		}
	}
}

void drawRectangleRect(Rectangle rectangle, uint32_t color) {
	for (int y = rectangle.y; y < rectangle.y + rectangle.height; y++) {
		for (int x = rectangle.x; x < rectangle.x + rectangle.width; x++) {
			window->frameBuffer[x + y * window->width] = color;
		}
	}
}

bool inRectangle(int x, int y, Rectangle rectangle) {
	if (x > rectangle.x + rectangle.width ||
			y > rectangle.y + rectangle.height ||
			x < rectangle.x || y < rectangle.y) {
		return false;
	}

	return true;
}

void onWindowClick(float x, float y, int mouse_btn) {
	if (mouse_btn != 1) {
		return;
	}

	for (int i = 0; i < 8; i++) {
		if (inRectangle(x, y, palette.colliders[i])) {
			editorCanvas.activeColor = palette.colors[i];
		}
	}
	
}

void onLeftMouseDown() {
	int canvasX = window->mouseX / editorCanvas.zoom;
	int canvasY = window->mouseY / editorCanvas.zoom;
	setEditorCanvasPixel(canvasX, canvasY, editorCanvas.activeColor);
	for (int y = 0; y < editorCanvas.collider.height; y++) {
		for (int x = 0; x < editorCanvas.collider.width; x++) {
			uint32_t color;
			bool success = getEditorCanvasPixel(x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			drawRectangle(
					x*editorCanvas.zoom,
					y*editorCanvas.zoom,
					editorCanvas.zoom,
					editorCanvas.zoom, color);
		}
	}
}

int main (void) {
	uint32_t *frameBuffer = calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));
	for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
		frameBuffer[i] = 0xCCCCCC;
	}

	Jup_CreateWindowArgs create_window_args = {
		.width = INITIAL_WINDOW_WIDTH,
		.height = INITIAL_WINDOW_HEIGHT,
		.windowTitle = "Pixelart Editor",
		.frameBuffer = frameBuffer,
		.onClick = onWindowClick
	};
	window = Jup_CreateWindow(create_window_args);

	editorCanvas = createEditorCanvas();
	for (int y = 0; y < editorCanvas.collider.height; y++) {
		for (int x = 0; x < editorCanvas.collider.width; x++) {
			uint32_t color;
			bool success = getEditorCanvasPixel(x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			drawRectangle(
					x*editorCanvas.zoom,
					y*editorCanvas.zoom,
					editorCanvas.zoom,
					editorCanvas.zoom, color);
		}
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

	for (int i = 0; i < 8; i++) {
		palette.colliders[i] = (Rectangle) {
			.x = window->width - (4 + 20)*(i+1),
				.y = 0,
				.width = 20,
				.height = 20
		};

		drawRectangleRect(palette.colliders[i], palette.colors[i]);
	}

	struct timespec requestedTime = { .tv_sec = 0, .tv_nsec = 16666667 };
	struct timespec remainingTime;

	while (!Jup_WindowShouldClose(window)) {
		if (window->mouseDown[1]) {
			onLeftMouseDown();
		}
		Jup_DrawPixels(window);
		nanosleep(&requestedTime, &remainingTime);
	}

	freeEditorCanvas(editorCanvas);
	Jup_FreeAndClose(window);
	return 0;
}

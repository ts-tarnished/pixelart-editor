#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
#define CANVAS_OFFSET_X 0
#define CANVAS_OFFSET_Y 80
#define TOP_BAR_HEIGHT 50

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
	Rectangle btnColliders[8];
	Rectangle boundary;
} Palette;

typedef struct {
	Rectangle boundary;
} TopBar;

static Jup_Window *window;
static EditorCanvas editorCanvas;
static Palette palette;
static TopBar topBar;

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

	for (int i = 0; i < 8; i++) {
		if (inRectangle(x, y, palette.btnColliders[i])) {
			editorCanvas.activeColor = palette.colors[i];
		}
	}
	
}

void drawEditorCanvas() {
	for (int y = 0; y < editorCanvas.collider.height; y++) {
		for (int x = 0; x < editorCanvas.collider.width; x++) {
			uint32_t color;
			bool success = getEditorCanvasPixel(x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			drawRectangle(
					x*editorCanvas.zoom + CANVAS_OFFSET_X,
					y*editorCanvas.zoom + CANVAS_OFFSET_Y,
					editorCanvas.zoom,
					editorCanvas.zoom, color);
		}
	}
}

void onLeftMouseDown() {
	int canvasX = (window->mouseX - CANVAS_OFFSET_X) / editorCanvas.zoom;
	int canvasY = (window->mouseY - CANVAS_OFFSET_Y) / editorCanvas.zoom;
	setEditorCanvasPixel(canvasX, canvasY, editorCanvas.activeColor);
	drawEditorCanvas();
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
		.onClick = onWindowClick
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
				.y = TOP_BAR_HEIGHT,
				.width = 20,
				.height = 20
		};

		drawRectangleRect(palette.btnColliders[i], palette.colors[i]);
	}

	topBar.boundary = (Rectangle) {
		.x = 0, .y = 0, .width = window->width, .height = TOP_BAR_HEIGHT
	};
	drawRectangleRect(topBar.boundary, 0x999999);

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

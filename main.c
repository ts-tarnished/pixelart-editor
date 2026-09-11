#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

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
	int width;
	int height;
	int capacity;
	uint32_t *pixels;
	int zoom;
} EditorCanvas;

EditorCanvas create_editor_canvas() {
	int capacity = CANVAS_INITIAL_WIDTH * CANVAS_INITIAL_HEIGHT;
	uint32_t *canvas_pixels = malloc(capacity * sizeof(uint32_t));
	if (canvas_pixels == NULL) {
		fprintf(stderr, "Error: Failed to allocate memory\n.");
		exit(1);
	}
	for (int i = 0; i < capacity; i++) {
		canvas_pixels[i] = 0xFFFFFF;
	}
	return (EditorCanvas) {
		.width = CANVAS_INITIAL_WIDTH,
		.height = CANVAS_INITIAL_HEIGHT,
		.capacity = capacity,
		.pixels = canvas_pixels,
		.zoom = CANVAS_INITIAL_ZOOM
	};
}

void free_editor_canvas(EditorCanvas editor_canvas) {
	free(editor_canvas.pixels);
}

bool editor_canvas_get_pixel(EditorCanvas editor_canvas, int x, int y, uint32_t *color) {
	if (x > editor_canvas.width || x < 0 || y > editor_canvas.height || y < 0) {
		return false;
	}
	*color = editor_canvas.pixels[x + y * editor_canvas.width];
	return true;
}

void editor_canvas_set_pixel(EditorCanvas editor_canvas, int x, int y, uint32_t color) {
	if (x >= editor_canvas.width || y >= editor_canvas.height || x < 0 || y < 0) 
		return;
	editor_canvas.pixels[x + y * editor_canvas.width] = color;
}

void draw_rectangle(Jup_Window *window, uint32_t *frame_buffer, int x, int y, int width, int height, uint32_t color) {
	// TODO: out of bounds check
	for (int _y = y; _y <= y + height; _y++) {
		for (int _x = x; _x <= x + width; _x++) {
			frame_buffer[_x + _y * window->width] = color;
		}
	}
}


int main (void) {
	uint32_t *frame_buffer = calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));
	for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
		frame_buffer[i] = 0xCCCCCC;
	}
	Jup_Window *window = Jup_CreateWindow(
			INITIAL_WINDOW_WIDTH,
			INITIAL_WINDOW_HEIGHT,
			"Pixelart Editor",
			frame_buffer
			);

	EditorCanvas editor_canvas = create_editor_canvas();
	editor_canvas_set_pixel(editor_canvas, 1, 1, 0);
	for (int y = 0; y < editor_canvas.height; y++) {
		for (int x = 0; x < editor_canvas.width; x++) {
			uint32_t color;
			bool success = editor_canvas_get_pixel(editor_canvas, x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			draw_rectangle(window, frame_buffer,
					x*editor_canvas.zoom,
					y*editor_canvas.zoom, 
					editor_canvas.zoom, 
					editor_canvas.zoom, color);
		}
	}

	while (!Jup_WindowShouldClose(window)) {
		Jup_DrawPixels(window);
	}

	free_editor_canvas(editor_canvas);
	Jup_FreeAndClose(window);
	return 0;
}

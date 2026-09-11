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
	int width;
	int height;
	int capacity;
	uint32_t *pixels;
	int zoom;
} EditorCanvas;

static Jup_Window *window;
static EditorCanvas editor_canvas;

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

bool editor_canvas_get_pixel(int x, int y, uint32_t *color) {
	if (x > editor_canvas.width || x < 0 || y > editor_canvas.height || y < 0) {
		return false;
	}
	*color = editor_canvas.pixels[x + y * editor_canvas.width];
	return true;
}

void editor_canvas_set_pixel(int x, int y, uint32_t color) {
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

void on_window_click(float x, float y, int mouse_btn) {
	return;
	if (mouse_btn != 1) {
		return;
	}

	int canvas_x = x / editor_canvas.zoom;
	int canvas_y = y / editor_canvas.zoom;
	editor_canvas_set_pixel(canvas_x, canvas_y, 0);
	for (int y = 0; y < editor_canvas.height; y++) {
		for (int x = 0; x < editor_canvas.width; x++) {
			uint32_t color;
			bool success = editor_canvas_get_pixel(x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			draw_rectangle(window, window->frame_buffer,
					x*editor_canvas.zoom,
					y*editor_canvas.zoom, 
					editor_canvas.zoom, 
					editor_canvas.zoom, color);
		}
	}
}

void on_left_mouse_down() {
	int canvas_x = window->mouse_x / editor_canvas.zoom;
	int canvas_y = window->mouse_y / editor_canvas.zoom;
	editor_canvas_set_pixel(canvas_x, canvas_y, 0);
	for (int y = 0; y < editor_canvas.height; y++) {
		for (int x = 0; x < editor_canvas.width; x++) {
			uint32_t color;
			bool success = editor_canvas_get_pixel(x, y, &color);
			if (!success) {
				fprintf(stderr, "Error: Pixel out of bounds.\n");
				exit(1);
			}
			draw_rectangle(window, window->frame_buffer,
					x*editor_canvas.zoom,
					y*editor_canvas.zoom, 
					editor_canvas.zoom, 
					editor_canvas.zoom, color);
		}
	}
}

int main (void) {
	uint32_t *frame_buffer = calloc(MAX_WIDTH*MAX_HEIGHT, sizeof(uint32_t));
	for (int i = 0; i < MAX_WIDTH*MAX_HEIGHT; i++ ) {
		frame_buffer[i] = 0xCCCCCC;
	}

	Jup_CreateWindowArgs create_window_args = (Jup_CreateWindowArgs) {
		.width = INITIAL_WINDOW_WIDTH,
		.height = INITIAL_WINDOW_HEIGHT,
		.window_title = "Pixelart Editor",
		.frame_buffer = frame_buffer,
		.on_click = on_window_click
	};
	window = Jup_CreateWindow(create_window_args);

	editor_canvas = create_editor_canvas();
	editor_canvas_set_pixel(2, 2, 0);
	for (int y = 0; y < editor_canvas.height; y++) {
		for (int x = 0; x < editor_canvas.width; x++) {
			uint32_t color;
			bool success = editor_canvas_get_pixel(x, y, &color);
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
	
	struct timespec requested_t = { .tv_sec = 0, .tv_nsec = 16666667 };
	struct timespec remaining_t;
	while (!Jup_WindowShouldClose(window)) {
		if (window->mouse_down[1]) {
			on_left_mouse_down();
		}
		Jup_DrawPixels(window);
		nanosleep(&requested_t, &remaining_t);
	}

	free_editor_canvas(editor_canvas);
	Jup_FreeAndClose(window);
	return 0;
}

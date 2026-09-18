#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
	int x;
	int y;
	int width;
	int height;
} Rectangle;

typedef struct {
	size_t width;
	size_t height;
	uint32_t *data;
} PixelArray;

typedef struct {
	int capacity;
	PixelArray workLayer;
	PixelArray toolLayer;
	int zoom;
	Rectangle collider;
	uint32_t activeColor;
	int mouseX;
	int mouseY;
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

bool inRectangle(int x, int y, Rectangle rectangle);
void drawLine(PixelArray *pixels, int x0, int y0, int x1, int y1, uint32_t color);
void setPixel(PixelArray *pixels, int x, int y, uint32_t color);
bool getPixel(PixelArray *pixels, int x, int y, uint32_t *color);
void drawRectangle(PixelArray *pixels, int x, int y, int width, int height, uint32_t color);
void drawRectangleRect(PixelArray *pixels, Rectangle rectangle, uint32_t color);

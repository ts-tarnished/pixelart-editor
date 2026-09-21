#ifndef PIXELART_EDITOR_TYPEDEF
#define PIXELART_EDITOR_TYPEDEF

#include <stdbool.h>
#include "ts_sw_graphics.h"

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
} UIButton;

typedef struct {
	Rectangle collider;
	unsigned int buttonCount;
	UIButton buttons[32];
} UIWidget;

bool inRectangle(int x, int y, Rectangle rectangle);
void drawLine(PixelArray *pixels, int x0, int y0, int x1, int y1, uint32_t color);
void setPixel(PixelArray *pixels, int x, int y, uint32_t color);
bool getPixel(PixelArray *pixels, int x, int y, uint32_t *color);
void drawRectangle(PixelArray *pixels, int x, int y, int width, int height, uint32_t color);
void drawRectangleRec(PixelArray *pixels, Rectangle rectangle, uint32_t color);
void drawRectangleRecBordered(PixelArray *pixels, Rectangle rectangle, unsigned int borderWidth, uint32_t recColor, uint32_t borderColor);

#endif

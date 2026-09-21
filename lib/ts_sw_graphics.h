#ifndef TS_SW_GRAPHICS_H
#define TS_SW_GRAPHICS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	int x;
	int y;
	int width;
	int height;
} Rectangle;

typedef struct {
	unsigned int width;
	unsigned int height;
	uint32_t *data;
} PixelArray;

void swapPoints(int *x0, int *y0, int *x1, int *y1);
bool inRectangle(int x, int y, Rectangle rectangle);
void drawRectangle(PixelArray *pixels, int x, int y, int width, int height, uint32_t color);
void drawRectangleRec(PixelArray *pixels, Rectangle rectangle, uint32_t color);
void drawRectangleRecBordered(PixelArray *pixels, Rectangle rectangle, unsigned int borderWidth, uint32_t recColor, uint32_t borderColor);
bool getPixel(PixelArray *pixels, int x, int y, uint32_t *color);
void setPixel(PixelArray *pixels, int x, int y, uint32_t color);
void drawLine(PixelArray *pixels, int x0, int y0, int x1, int y1, uint32_t color);
void Jup_DrawText(PixelArray *pixels, int x, int y, char *text, uint32_t color);

#endif

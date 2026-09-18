#include <stdlib.h>
#include <math.h>
#include "util.h"

void swapPoints(int *x0, int *y0, int *x1, int *y1) {
	int temp = *x0;
	*x0 = *x1;
	*x1 = temp;
	temp = *y0;
	*y0 = *y1;
	*y1 = temp;
}

bool inRectangle(int x, int y, Rectangle rectangle) {
	if (x >= rectangle.x + rectangle.width ||
			y >= rectangle.y + rectangle.height ||
			x < rectangle.x || y < rectangle.y) {
		return false;
	}

	return true;
}

void drawRectangle(PixelArray *pixels, int x, int y, int width, int height, uint32_t color) {
	// TODO: out of bounds check
	for (int _y = y; _y < y + height; _y++) {
		for (int _x = x; _x < x + width; _x++) {
			pixels->data[_x + _y * pixels->width] = color;
		}
	}
}

void drawRectangleRect(PixelArray *pixels, Rectangle rectangle, uint32_t color) {
	for (int y = rectangle.y; y < rectangle.y + rectangle.height; y++) {
		for (int x = rectangle.x; x < rectangle.x + rectangle.width; x++) {
			pixels->data[x + y * pixels->width] = color;
		}
	}
}

bool getPixel(PixelArray *pixels, int x, int y, uint32_t *color) {
	if (x >= pixels->width || x < 0 || y >= pixels->height || y < 0) {
		return false;
	}
	*color = pixels->data[x + y * pixels->width];
	return true;
}

void setPixel(PixelArray *pixels, int x, int y, uint32_t color) {
	if (x >= pixels->width || x < 0 || y >= pixels->height || y < 0) {
		return;
	}
	pixels->data[x + y * pixels->width] = color;
}

void drawLine(PixelArray *pixels, int x0, int y0, int x1, int y1, uint32_t color) {
	if (abs(x1 - x0) >= abs(y1 - y0)) {
		if (x1 < x0) swapPoints(&x0, &y0, &x1, &y1);
		float slope = (x1-x0 == 0) ? 0 : (float)(y1 - y0) / (float)(x1 - x0);
		for (int x = 0; x <= (x1-x0); x++) {
			setPixel(pixels, x0 + x, y0 + roundf(x*slope), color);
		}
	} else {
		if (y1 < y0) swapPoints(&x0, &y0, &x1, &y1);
		float slope = (y1-y0 == 0) ? 0 : (float)(x1 - x0) / (float)(y1 - y0);
		for (int y = 0; y <= (y1-y0); y++) {
			setPixel(pixels, x0 + roundf(y*slope), y0 + y, color);
		}
	}
}

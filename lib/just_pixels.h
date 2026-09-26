#include <stdint.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdbool.h>

#define JUP_MOUSE_BUTTONS_CAPACITY 32

typedef struct {
	Display *display;
	Window window;
	XImage *frameBufferImage;
	Atom deleteWindowMessage;
	GC graphicsContext;
} Jup_X11Context;

typedef struct {
	int width;
	int height;
	float mouseX;
	float mouseY;
	bool mouseDown[JUP_MOUSE_BUTTONS_CAPACITY];
	uint32_t *frameBuffer;
	void (*onClick)(float x, float y, int mouseBtn);
	void (*onMouseRelease)(float x, float y, int mouseBtn);
	void (*onKeyPressed)(int keysym, char c);
	void (*onKeyReleased)(int keyCode);
	Jup_X11Context context;
} Jup_Window;

typedef struct {
	int width;
	int height;
	char *windowTitle;
	uint32_t *frameBuffer;
	void (*onClick)(float x, float y, int mouseBtn);
	void (*onMouseRelease)(float x, float y, int mouseBtn);
	void (*onKeyPressed)(int keysym, char c);
	void (*onKeyReleased)(int keysym);
} Jup_CreateWindowArgs;

Jup_Window* Jup_CreateWindow(Jup_CreateWindowArgs args);
bool Jup_WindowShouldClose(Jup_Window *jupWindow);
void Jup_Update(Jup_Window *jupWindow, int x, int y, int width, int height);
void Jup_DrawPixels(Jup_Window *jupWindow);
void Jup_FreeAndClose(Jup_Window *window);

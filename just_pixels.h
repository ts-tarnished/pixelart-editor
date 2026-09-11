#ifndef JUST_PIXELS_H
#define JUST_PIXELS_H

#include <stdbool.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define JUP_MOUSE_BUTTONS_CAPACITY 32

typedef struct {
	Display *display;
	Window window;
	XImage *frame_buffer_image;
	Atom delete_window_message;
	GC graphics_context;
} Jup_X11Context;

typedef struct {
	int width;
	int height;
	float mouse_x;
	float mouse_y;
	bool mouse_down[JUP_MOUSE_BUTTONS_CAPACITY];
	uint32_t *frame_buffer;
	void (*on_click)(float x, float y, int mouse_btn);
	Jup_X11Context context;
} Jup_Window;

typedef struct {
	int width;
	int height;
	char *window_title;
	uint32_t *frame_buffer;
	void (*on_click)(float x, float y, int mouse_btn);
} Jup_CreateWindowArgs;


Jup_Window* Jup_CreateWindow(Jup_CreateWindowArgs args);
bool Jup_WindowShouldClose(Jup_Window *jup_window);
void Jup_Update(Jup_Window *jup_window, int x, int y, int width, int height);
void Jup_DrawPixels(Jup_Window *jup_window);
void Jup_FreeAndClose(Jup_Window *window);

#ifdef JUST_PIXELS_IMPLEMENTATION

Jup_X11Context Jup_X11_Init(Jup_CreateWindowArgs args) {
	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Failed to open X display\n");
		exit(1);
	}

	int screen = DefaultScreen(display);
	Window window = XCreateSimpleWindow(
			display,
			RootWindow(display, screen),
			100, 100,
			args.width,
			args.height,
			0,
			BlackPixel(display, screen),
			BlackPixel(display, screen)
			);

	XSelectInput(display,
             window,
             PointerMotionMask |
             ButtonPressMask |
             ButtonReleaseMask |
             ExposureMask |
             StructureNotifyMask);

	XStoreName(display, window, args.window_title);

	Atom delte_window_message =
		XInternAtom(display, "WM_DELETE_WINDOW", False);

	XSetWMProtocols(
			display,
			window,
			&delte_window_message,
			1
		       );

	XMapWindow(display, window);

	XImage *frame_buffer_image = XCreateImage(
			display,
			DefaultVisual(display, screen),
			24,
			ZPixmap,
			0,
			(char *)args.frame_buffer,
			args.width,
			args.height,
			32,
			args.width * sizeof(uint32_t)
			);

	GC graphics_context = DefaultGC(display, screen);

	Jup_X11Context x11 = (Jup_X11Context) {
		.display = display,
			.window = window,
			.frame_buffer_image = frame_buffer_image,
			.graphics_context = graphics_context,
			.delete_window_message = delte_window_message
	};

	return x11;
}

Jup_Window* Jup_CreateWindow(Jup_CreateWindowArgs args) {
	Jup_Window *window = malloc(sizeof(Jup_Window));
	window->width = args.width;
	window->height = args.height;
	window->context = Jup_X11_Init(args);
	window->on_click = args.on_click;
	window->frame_buffer = args.frame_buffer;
	for (int i = 0; i < JUP_MOUSE_BUTTONS_CAPACITY; i++) {
		window->mouse_down[i] = false;
	}

	return window;
}

bool Jup_WindowShouldClose(Jup_Window *jup_window) {
	Jup_X11Context x11 = jup_window->context;
	XEvent event;

	while (XPending(x11.display) > 0) {
		XNextEvent(x11.display, &event);
		if (event.type == ClientMessage &&
				(Atom)event.xclient.data.l[0] == x11.delete_window_message) {
			return 1;
		}

		if (event.type == MotionNotify) {
			XMotionEvent *motion = &event.xmotion;
			jup_window->mouse_x = motion->x;
			jup_window->mouse_y = motion->y;
		}

		if (event.type == ButtonPress) {
			if (event.xbutton.button >= JUP_MOUSE_BUTTONS_CAPACITY) {
				fprintf(stderr, "Error: Unknown mouse button: %i\n", event.xbutton.button);
			}

			jup_window->mouse_down[event.xbutton.button] = true;
			jup_window->on_click(
					event.xbutton.x,
					event.xbutton.y,
					event.xbutton.button
					);
		}

		if (event.type == ButtonRelease) {
			if (event.xbutton.button >= JUP_MOUSE_BUTTONS_CAPACITY) {
				fprintf(stderr, "Error: Unknown mouse button: %i\n", event.xbutton.button);
			}

			jup_window->mouse_down[event.xbutton.button] = false;
		}

	}

	return 0;
}

void Jup_X11Update(Jup_Window *jup_window, int x, int y, int width, int height) {
	Jup_X11Context x11 = jup_window->context;
	XPutImage(
			x11.display,
			x11.window,
			x11.graphics_context,
			x11.frame_buffer_image,
			x, y,
			x, y,
			width,height // update region
		 );
	XFlush(x11.display);
}


void Jup_Update(Jup_Window *jup_window, int x, int y, int width, int height) {
	Jup_X11Update(jup_window, x, y, width, height);
}

void Jup_X11DrawPixels(Jup_Window *jup_window) {
	Jup_X11Context x11 = jup_window->context;
	XPutImage(
			x11.display,
			x11.window,
			x11.graphics_context,
			x11.frame_buffer_image,
			0, 0,
			0, 0,
			jup_window->width,
			jup_window->height
		 );
	XFlush(x11.display);
}

void Jup_DrawPixels(Jup_Window *jup_window) {
	Jup_X11DrawPixels(jup_window);
}

void Jup_X11FreeAndClose(Jup_Window * window) {
	Jup_X11Context x11 = window->context;
	// XDestroyImage also frees the frame_buffer that was passed in Jup_CreateWindow()
	XDestroyImage(x11.frame_buffer_image);
	free(window);
	XCloseDisplay(x11.display);
}

void Jup_FreeAndClose(Jup_Window *window) {
	Jup_X11FreeAndClose(window);
}

#endif
#endif

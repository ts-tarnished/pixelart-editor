#ifndef JUST_PIXELS_H
#define JUST_PIXELS_H

#include <stdbool.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct {
	Display *display;
	Window window;
	XImage *frame_buffer_image;
	Atom delete_window_message;
	GC graphics_context;
	bool force_next_render;
} Jup_X11Context;

typedef struct {
	int width;
	int height;
	Jup_X11Context context;
} Jup_Window;


Jup_Window* Jup_CreateWindow(int width, int height, char* window_title, uint32_t *frame_buffer);
bool Jup_WindowShouldClose(Jup_Window *jup_window);
void Jup_Update(Jup_Window *jup_window, int x, int y, int width, int height);
void Jup_DrawPixels(Jup_Window *jup_window);
void Jup_FreeAndClose(Jup_Window *window);

#ifdef JUST_PIXELS_IMPLEMENTATION

Jup_X11Context Jup_X11_Init(int width, int height, char *window_title, uint32_t *frame_buffer) {
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
			width,
			height,
			0,
			BlackPixel(display, screen),
			BlackPixel(display, screen)
			);

	XSelectInput(display, window, ExposureMask | KeyPressMask);

	XStoreName(display, window, window_title);

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
			(char *)frame_buffer,
			width,
			height,
			32,
			width * sizeof(uint32_t)
			);

	GC graphics_context = DefaultGC(display, screen);

	Jup_X11Context x11 = (Jup_X11Context) {
		.display = display,
			.window = window,
			.frame_buffer_image = frame_buffer_image,
			.graphics_context = graphics_context,
			.delete_window_message = delte_window_message,
			.force_next_render = 0
	};

	return x11;
}

Jup_Window* Jup_CreateWindow(int width, int height, char* window_title, uint32_t *frame_buffer) {
	Jup_Window *window = malloc(sizeof(Jup_Window));
	window->width = width;
	window->height = height;
	window->context = Jup_X11_Init(width, height, window_title, frame_buffer);


	return window;
}

bool Jup_WindowShouldClose(Jup_Window *jup_window) {
	Jup_X11Context x11 = jup_window->context;
	XEvent event;

	if (!x11.force_next_render) {
		// XNextEvent is blocking
		XNextEvent(x11.display, &event);

		if (event.type == ClientMessage &&
				(Atom)event.xclient.data.l[0] == x11.delete_window_message) {
			return 1;
		}

		return 0;
	}

	printf("hi\n");
	while (XPending(x11.display) > 0) {
		XNextEvent(x11.display, &event);
		if (event.type == ClientMessage &&
				(Atom)event.xclient.data.l[0] == x11.delete_window_message) {
			return 1;
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

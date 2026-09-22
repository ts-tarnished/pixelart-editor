#include <X11/X.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "just_pixels.h"

Jup_X11Context Jup_X11_Init(Jup_CreateWindowArgs args) {
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
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
        StructureNotifyMask |
        KeyPressMask |
        KeyReleaseMask);

    XStoreName(display, window, args.windowTitle);

    Atom deleteWindowMessage =
        XInternAtom(display, "WM_DELETE_WINDOW", False);

    XSetWMProtocols(
        display,
        window,
        &deleteWindowMessage,
        1
    );

    XMapWindow(display, window);

    XImage *frameBufferImage = XCreateImage(
        display,
        DefaultVisual(display, screen),
        24,
        ZPixmap,
        0,
        (char *)args.frameBuffer,
        args.width,
        args.height,
        32,
        args.width * sizeof(uint32_t)
    );

    GC graphicsContext = DefaultGC(display, screen);

    Jup_X11Context x11 = (Jup_X11Context) {
        .display = display,
        .window = window,
        .frameBufferImage = frameBufferImage,
        .deleteWindowMessage = deleteWindowMessage,
        .graphicsContext = graphicsContext
    };

    return x11;
}

Jup_Window* Jup_CreateWindow(Jup_CreateWindowArgs args) {
    Jup_Window *window = (Jup_Window*)malloc(sizeof(Jup_Window));
    window->width = args.width;
    window->height = args.height;
    window->context = Jup_X11_Init(args);
    window->onClick = args.onClick;
    window->onMouseRelease = args.onMouseRelease;
    window->onKeyPressed = args.onKeyPressed;
    window->onKeyReleased = args.onKeyReleased;
    window->frameBuffer = args.frameBuffer;
    window->mouseX = 9999;
    window->mouseY = 9999;
    window->shiftPressed = false;
    for (int i = 0; i < JUP_MOUSE_BUTTONS_CAPACITY; i++) {
        window->mouseDown[i] = false;
    }

    return window;
}

bool Jup_WindowShouldClose(Jup_Window *jupWindow) {
    Jup_X11Context x11 = jupWindow->context;
    XEvent event;

    while (XPending(x11.display) > 0) {
        XNextEvent(x11.display, &event);
        if (event.type == ClientMessage &&
            (Atom)event.xclient.data.l[0] == x11.deleteWindowMessage) {
                return 1;
            }

        if (event.type == MotionNotify) {
            XMotionEvent *motion = &event.xmotion;
            jupWindow->mouseX = motion->x;
            jupWindow->mouseY = motion->y;
        }

        if (event.type == ButtonPress) {
            if (event.xbutton.button >= JUP_MOUSE_BUTTONS_CAPACITY) {
                fprintf(stderr, "Error: Unknown mouse button: %i\n", event.xbutton.button);
            }

            jupWindow->mouseDown[event.xbutton.button] = true;
            jupWindow->onClick(
                event.xbutton.x,
                event.xbutton.y,
                event.xbutton.button
            );
        }

        if (event.type == ButtonRelease) {
            if (event.xbutton.button >= JUP_MOUSE_BUTTONS_CAPACITY) {
                fprintf(stderr, "Error: Unknown mouse button: %i\n", event.xbutton.button);
            }

            jupWindow->mouseDown[event.xbutton.button] = false;
            jupWindow->onMouseRelease(
                event.xbutton.x,
                event.xbutton.y,
                event.xbutton.button
            );
        }

        if (event.type == KeyPress) {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            char* str = XKeysymToString(keysym);
            if (keysym == 0xffe1 || keysym == 0xffe2)
                jupWindow->shiftPressed = true;
            jupWindow->onKeyPressed(keysym);
        }

        if (event.type == KeyRelease) {
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            if (keysym == 0xffe1 || keysym == 0xffe2)
                jupWindow->shiftPressed = false;
        }

    }

    return 0;
}

void Jup_X11Update(Jup_Window *jup_window, int x, int y, int width, int height) {
    Jup_X11Context x11 = jup_window->context;
    XPutImage(
        x11.display,
        x11.window,
        x11.graphicsContext,
        x11.frameBufferImage,
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
        x11.graphicsContext,
        x11.frameBufferImage,
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
    XDestroyImage(x11.frameBufferImage);
    free(window);
    XCloseDisplay(x11.display);
}

void Jup_FreeAndClose(Jup_Window *window) {
    Jup_X11FreeAndClose(window);
}

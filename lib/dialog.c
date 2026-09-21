#include <stdbool.h>
#include "dialog.h"
#include "just_pixels.h"
#include "ts_sw_graphics.h"
#include "pixelart_editor_typedef.h"

static Jup_Window jupWindow;
static int inputCount = 0;
static UITextInput *textInputs;
static int focusedInput = -1;
static UIButton submitButton;
static void (*onSubmit)(void);

void Dialog_Open(Jup_Window w, void (*onSubmitCallback)(void)) {
    jupWindow = w;
    onSubmit = onSubmitCallback;
}

static void Dialog_OnClick(int x, int y) {
    if (inRectangle(x, y, submitButton.collider)) {
        onSubmit();
        return;
    }

    focusedInput = -1;
    for (int i = 0; i < inputCount; i++) {
        if (inRectangle(x, y, textInputs[i].collider)) {
            focusedInput = i;
        }
    }
}

void Dialog_DrawDialog(PixelArray *window, UIDialog *dialog) {
    int inputHeight = 30;
    int titleHeight = 30;
    int border = 12;
    int padding = 20;
    int popupWidth = 600;
    int footerHeight = 70;
    int popupHeight = titleHeight + 2 * padding + border
        + dialog->inputCount * inputHeight
        + footerHeight;

    int x = ((int)window->width - popupWidth) / 2;
    int y = ((int)window->height - popupHeight) / 2;
    Rectangle r = {
        .x = x, .y = y,
        .width = popupWidth,
        .height = popupHeight
    };

    // the dialog frame
    drawRectangleRecBordered(window, r, 12, 0xFFFFFF, 0);
    drawRectangle(window, x, y, popupWidth, titleHeight, 0);

    // inputs
    for (int i = 0; i < dialog->inputCount; i++) {
        int labelX = x + border + padding;
        int labelY = y + titleHeight + padding + (i * inputHeight);
        int labelHeight = 20;

        Rectangle r = {
            .x = labelX,
            .y = labelY + labelHeight,
            .width = popupWidth - 2 * (border + padding),
            .height = inputHeight
        };
        Jup_DrawText(window, labelX, labelY, (char *)"label", 0);
        uint32_t border_c  = i == focusedInput ? 0xFF0000 : 0;
        drawRectangleRecBordered(window, r, 1, 0xFFFFFF, border_c);
    }

    drawLine(window,
        x + padding + border, y + popupHeight - footerHeight - border,
        x + popupWidth - padding - border, y + popupHeight - footerHeight - border,
        0);

    int btnWidth = 60;
    int btnHeight = 40;
    drawRectangle(window,
        x + popupWidth - btnWidth - padding - border,
        y + (popupHeight - footerHeight - border) + (btnHeight / 2),
        btnWidth, btnHeight, 0);
}

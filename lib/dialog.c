#include "dialog.h"
#include "just_pixels.h"
#include "pixelart_editor_typedef.h"
#include "ts_sw_graphics.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Jup_Window jupWindow;
static UIButton submitButton;
static void (*onSubmit)(void);

void Dialog_Open(Jup_Window w, void (*onSubmitCallback)(void)) {
    jupWindow = w;
    onSubmit = onSubmitCallback;
}

void Dialog_TriggerKeyPressed(UIDialog *dialog, int keyCode, bool shift) {
    if (dialog->focusedInput < 0) {
        return;
    }

    UITextInput *input = &dialog->inputs[dialog->focusedInput];

    printf("Dialog: %i\n", keyCode);
    if (keyCode == 65293) {
        // Enter
    }
    if (keyCode == 65307) {
        // Escape
    }

    size_t len = strlen(input->value);
    if (keyCode == 65288) {
        // Backspace
        if (len > 0) {
            input->value[len - 1] = '\0';
        }
        return;
    }

    if (isalnum(keyCode) == 0)
        return;

    if (len >= 367)
        return;

    if (shift) {
        if(isalnum(keyCode)) {
            sprintf(input->value, "%s%c", input->value, toupper(keyCode));
            return;
        }
    }

    sprintf(input->value, "%s%c", input->value, (char)keyCode);
}

void Dialog_TriggerMouseClicked(UIDialog *dialog, int mouseButton, int x,
    int y) {
        if (mouseButton != 1)
            return;

        if (inRectangle(x, y, submitButton.collider)) {
            onSubmit();
            return;
        }

        dialog->focusedInput = -1;
        for (int i = 0; i < dialog->inputCount; i++) {
            if (inRectangle(x, y, dialog->inputs[i].collider)) {
                printf("Dialog clicked.\n");
                dialog->focusedInput = i;
            }
        }
    }

    UIDialog Dialog_CreateDialog(char dialogTitle[], int inputCapacity) {
        UIDialog dialog = {
            .inputCapacity = inputCapacity, .inputCount = 0, .focusedInput = -1};
        if (inputCapacity > 0) {
            dialog.inputs = (UITextInput *)malloc(sizeof(UITextInput) * inputCapacity);
            if (dialog.inputs == NULL) {
                fprintf(stderr, "Error: Failed to allocate memory.\n");
                exit(1);
            }
        } else {
            dialog.inputs = NULL;
        }

        strncpy(dialog.title, dialogTitle, 32);
        return dialog;
    }

    void Dialog_AddInput(UIDialog *dialog, UITextInput input) {
        if (dialog->inputCount >= dialog->inputCapacity) {
            fprintf(stderr, "Error: Capacity exceeded.\n");
            exit(1);
        }
        dialog->inputs[dialog->inputCount] = input;
        dialog->inputCount += 1;
    }

    void Dialog_DrawDialog(PixelArray *window, UIDialog *dialog) {
        int titleHeight = 30;
        int border = 12;
        int padding = 20;
        int popupWidth = 600;
        int labelHeight = 14;
        int inputHeight = 30;
        int labelMargin = 5;
        int fieldsetHeight = inputHeight + labelHeight + labelMargin;
        int fieldsetMargin = 20;
        int buttonHeight = 30;

        int popupHeight = titleHeight + 2 * padding + border +
            dialog->inputCount * (fieldsetHeight) + padding +
            buttonHeight + padding;

        int dialogX = ((int)window->width - popupWidth) / 2;
        int dialogY = ((int)window->height - popupHeight) / 2;
        Rectangle r = {
            .x = dialogX, .y = dialogY, .width = popupWidth, .height = popupHeight};

        // the dialog frame
        drawRectangleRecBordered(window, r, 12, 0xFFFFFF, 0);
        drawRectangle(window, dialogX, dialogY, popupWidth, titleHeight, 0);
        Jup_DrawText(window, dialogX + border, dialogY + 8, dialog->title, 0xFFFFFF);

        int y = dialogY + titleHeight + padding;
        int innerX = dialogX + border + padding;

        // inputs
        for (int i = 0; i < dialog->inputCount; i++) {
            // label
            Jup_DrawText(window, innerX, y, (char *)dialog->inputs[i].label, 0);
            y += labelHeight + labelMargin;
            // input
            Rectangle r = {.x = innerX,
                .y = y,
                .width = popupWidth - 2 * (border + padding),
                .height = inputHeight};
            dialog->inputs[i].collider = r;
            if (dialog->focusedInput == i) {
                drawRectangleRecBordered(window, r, 2, 0xFFFFFF, 0xFF0000);
            } else {
                drawRectangleRecBordered(window, r, 1, 0xFFFFFF, 0);
            }

            Jup_DrawText(window, innerX + 5, y + 8, dialog->inputs[i].value, 0);

            y += inputHeight;
            y = (i == dialog->inputCount - 1) ? y : y + fieldsetMargin;
        }

        y += padding;
        int btnWidth = 60;
        drawRectangle(window, dialogX + popupWidth - btnWidth - padding - border, y,
            btnWidth, buttonHeight, 0);
    }

    void Dialog_FreeDialog(UIDialog *dialog) { free(dialog->inputs); }

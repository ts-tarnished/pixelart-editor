#include <stdbool.h>
#include "ts_sw_graphics.h"

typedef struct {
    char label[32];
    Rectangle collider;
} UITextInput;

typedef struct {
    char title[32];
    UITextInput inputs[16];
    unsigned int inputCount;
    unsigned int inputsPerRow;
} UIDialog;

void Dialog_DrawDialog(PixelArray *window, UIDialog *dialog);

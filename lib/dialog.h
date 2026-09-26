#include <stdbool.h>
#include "ts_sw_graphics.h"

typedef struct {
    bool set;
    char label[32];
    char value[368];
    Rectangle collider;
} UITextInput;

typedef struct {
    char title[32];
    int inputCapacity;
    int inputCount;
    int focusedInput;
    UITextInput* inputs;
} UIDialog;

UIDialog Dialog_CreateDialog(char dialogTitle[], int inputCount);
void Dialog_AddInput(UIDialog* dialog, UITextInput input);
void Dialog_DrawDialog(PixelArray* window, UIDialog* dialog);
void Dialog_TriggerKeyPressed(UIDialog* dialog, int keysym, char c);
void Dialog_TriggerMouseClicked(UIDialog* dialog, int mouseButton, int x, int y);
void Dialog_FreeDialog(UIDialog* dialog);

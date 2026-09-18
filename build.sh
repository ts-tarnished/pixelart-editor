# Dependencies: libx11-dev
# gcc main.c -o pixelart-editor -lX11 -lm
gcc src/*.c -Iinclude -Wall -Wextra -fsanitize=address -o pixelart-editor -lX11 -lm

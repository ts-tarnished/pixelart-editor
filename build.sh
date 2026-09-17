# Dependencies: libx11-dev
# gcc main.c -o pixelart-editor -lX11 -lm
gcc main.c -Wall -Wextra -fsanitize=address -o pixelart-editor -lX11 -lm

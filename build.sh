# Dependencies: libx11-dev
# gcc main.c -o pixelart-editor -lX11 -lm
gcc src/*.c lib/*.c -Ilib -Isrc -Wall -Wextra -fsanitize=address -o pixelart-editor -lX11 -lm

/*
  Just a test program for the WatcomGL graphics modes.
*/
/*
  MIT License

  Copyright (c) 2026 Stefan Mader

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
// WatcomGL can be found here: https://github.com/kosmonautdnb/WatcomGL
// Some additional Libraries for WatcomC can be found here: https://github.com/kosmonautdnb/WatcomLibs
#include "GL.H"
#include "GLEGA.HPP"
#include <stdio.h>
#include <conio.h>

unsigned int getColor(int i, int w, int h) {
  int r = (i % w)*255/w;
  int g = (i / w)*255/h;
  int b = (r+g)/2;
  return r | (g<<8) | (b<<16) | 0xff000000;
}

void displayAndWait() {
  for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) glFrameBuffer[i] = getColor(i,glFrameBufferWidth,glFrameBufferHeight);
  glRefresh(); getch(); glDone();
  printf("If you have seen a reasonable pattern all is good here.\n");
}

int main(int argc, char *argv[]) {

  printf("Press a key to test glVGA() (HCR mode, 320*200);\n"); getch();
  if (glVGA()) {
    displayAndWait();
  } else {
    printf("glVGA is not available.\n");
  }

  printf("Press a key to test glVesa() 640x480 with 32 bits;\n"); getch();
  if (glVesa(640,480,32)) {
    displayAndWait();
  } else {
    printf("glVesa() 640x480 with 32 bits is not available.\n");
  }

  printf("Press a key to test glVesa() 640x480 with 24 bits;\n"); getch();
  if (glVesa(640,480,24)) {
    displayAndWait();
  } else {
    printf("glVesa() 640x480 with 24 bits is not available.\n");
  }

  printf("Press a key to test glVesa() 640x480 with 16 bits;\n"); getch();
  if (glVesa(640,480,16)) {
    displayAndWait();
  } else {
    printf("glVesa() 640x480 with 16 bits is not available.\n");
  }

  printf("Press a key to test glEGA() 640x480;\n"); getch();
  if (glEGA(640,480)) {
    displayAndWait();
  } else {
    printf("glEGA() 640x480 is not available.\n");
  }

  return 0;
}

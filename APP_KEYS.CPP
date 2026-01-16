/*
  Just a simple keypress logger.
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
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>
#ifdef __WATCOMC__
#include <i86.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#endif // __DJGPP__

#ifdef __WATCOMC__
GLuint int16_1(); // we need the zero flag it's not well exposed by the watcom api sorry that's why some asm here
#pragma aux int16_1="mov ah,0x01""int 0x16""jz do""mov eax,0""jmp dont""do:""mov eax,1""dont:" value[eax] modify[eax ebx ecx edx]
#endif
#ifdef __DJGPP__
GLuint int16_1() {
  GLuint ret;
  __asm__ __volatile__("\tmovb $1,%%ah\n""\tint $0x16\n""\tjz 1f\n""\tmovl $0,%%eax\n""\tjmp 2f\n""1:\n""\tmovl $1,%%eax\n""2:\n" : "=a" (ret));
  return ret;
}
#endif

int main(int argc, char *argv[]) {
  int key = 0;
  printf("Press a key:\n");
  while(1) {
    key = glNextKey();
    GLboolean shift,ctrl,alt;
    glSpecialKeys(&shift,&ctrl,&alt);
    if (shift != 0x00) printf("shift;");
    if (ctrl != 0x00) printf("ctrl;");
    if (alt != 0x00) printf("alt;");
    if (key != 0) break;
  }
  printf("WatcomGL keycode:%d ($%04x)\n", key, key);

  while(1) {
    key = (unsigned char)inp(0x60);
    if (key & 0x80) break;
  }

  unsigned char followUp[6];
  printf("Press another key:\n");
  while(1) {
    key = (unsigned char)inp(0x60);
    if ((key & 0x80) == 0) {
      printf("Scancode: $%02x(%d)\n", key, key);
      break;
    }
  }

  while(glNextKey() !=0 ) {;}

  return 0;
}

#include "GLEGA.HPP"
#define WATCOMGLAPI
#include "GL.H"
#include <stdlib.h>

#ifdef __WATCOMC__
#include <i86.h>
#include <conio.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#endif // __DJGPP__

bool glIsEga = false;

static unsigned int palette[16];
static GLubyte *glEgaIndexBuffer = NULL;
static unsigned short glEgaGraphicsModeToRestore;

#ifdef __WATCOMC__
#define DJGPPVGAMEMON
#define DJGPPVGAMEMOFF
#define VGARAMBASE (0xa0000)
#endif //__WATCOMC__

#ifdef __DJGPP__
#define VGARAMBASE (0xa0000+__djgpp_conventional_base)
#define DJGPPVGAMEMON if (useNearPointers) __djgpp_nearptr_enable();
#define DJGPPVGAMEMOFF if (useNearPointers) __djgpp_nearptr_disable();
#endif // __DJGPP__

#ifdef __WATCOMC__
#define INLINE static __inline
#endif // __WATCOMC__

#ifdef __DJGPP__
#define INLINE static  inline
#endif // __DJGPP__

#ifndef INLINE
#define INLINE static inline
#endif // INLINE

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


extern GLint glFrameBufferWidth0;
extern GLint glFrameBufferHeight0;
extern GLuint *glFrameBuffer0;
extern GLfloat *glDepthBuffer0;
extern GLubyte *glStencilBuffer0;

#ifdef __cplusplus
}
#endif // __cplusplus


#define EGA_COLOR_BLACK         0x00
#define EGA_COLOR_BLUE          0x01
#define EGA_COLOR_GREEN         0x02
#define EGA_COLOR_CYAN          0x03
#define EGA_COLOR_RED           0x04
#define EGA_COLOR_VIOLET        0x05
#define EGA_COLOR_BROWN         0x06
#define EGA_COLOR_GREY_LIGHT    0x07
#define EGA_COLOR_GREY_DARK     0x08
#define EGA_COLOR_BLUE_LIGHT    0x09
#define EGA_COLOR_GREEN_LIGHT   0x0a
#define EGA_COLOR_CYAN_LIGHT    0x0b
#define EGA_COLOR_RED_LIGHT     0x0c
#define EGA_COLOR_VIOLET_LIGHT  0x0d
#define EGA_COLOR_YELLOW        0x0e
#define EGA_COLOR_WHITE         0x0f

#define _EGA_COLOR_BLACK         0xff000000
#define _EGA_COLOR_BLUE          0xffaa0000
#define _EGA_COLOR_GREEN         0xff00aa00
#define _EGA_COLOR_CYAN          0xffaaaa00
#define _EGA_COLOR_RED           0xff0000aa
#define _EGA_COLOR_VIOLET        0xffaa00aa
#define _EGA_COLOR_BROWN         0xff0055aa
#define _EGA_COLOR_GREY_LIGHT    0xffaaaaaa
#define _EGA_COLOR_GREY_DARK     0xff555555
#define _EGA_COLOR_BLUE_LIGHT    0xffff5555
#define _EGA_COLOR_GREEN_LIGHT   0xff55ff55
#define _EGA_COLOR_CYAN_LIGHT    0xffffff55
#define _EGA_COLOR_RED_LIGHT     0xff5555ff
#define _EGA_COLOR_VIOLET_LIGHT  0xffff55ff
#define _EGA_COLOR_YELLOW        0xff55ffff
#define _EGA_COLOR_WHITE         0xffffffff

void egaPalette() {
  int startColor = 0;
  palette[EGA_COLOR_BLACK+startColor] = _EGA_COLOR_BLACK;
  palette[EGA_COLOR_BLUE+startColor] = _EGA_COLOR_BLUE;
  palette[EGA_COLOR_GREEN+startColor] = _EGA_COLOR_GREEN;
  palette[EGA_COLOR_CYAN+startColor] = _EGA_COLOR_CYAN;
  palette[EGA_COLOR_RED+startColor] = _EGA_COLOR_RED;
  palette[EGA_COLOR_VIOLET+startColor] = _EGA_COLOR_VIOLET;
  palette[EGA_COLOR_BROWN+startColor] = _EGA_COLOR_BROWN;
  palette[EGA_COLOR_GREY_LIGHT+startColor] = _EGA_COLOR_GREY_LIGHT;
  palette[EGA_COLOR_GREY_DARK+startColor] = _EGA_COLOR_GREY_DARK;
  palette[EGA_COLOR_BLUE_LIGHT+startColor] = _EGA_COLOR_BLUE_LIGHT;
  palette[EGA_COLOR_GREEN_LIGHT+startColor] = _EGA_COLOR_GREEN_LIGHT;
  palette[EGA_COLOR_CYAN_LIGHT+startColor] = _EGA_COLOR_CYAN_LIGHT;
  palette[EGA_COLOR_RED_LIGHT+startColor] = _EGA_COLOR_RED_LIGHT;
  palette[EGA_COLOR_VIOLET_LIGHT+startColor] = _EGA_COLOR_VIOLET_LIGHT;
  palette[EGA_COLOR_YELLOW+startColor] = _EGA_COLOR_YELLOW;
  palette[EGA_COLOR_WHITE+startColor] = _EGA_COLOR_WHITE;
}

static void setBiosGraphicsMode(unsigned short mode) {
  union REGS regs;
  regs.w.ax = mode;
  int386(0x10, &regs, &regs);
}
 
static unsigned short getBiosGraphicsMode() {
  union REGS regs;
  regs.w.ax = 0x0f00;
  int386 (0x10, &regs, &regs);
  return regs.h.al;
}

INLINE unsigned char glGetPaletteIndex16(unsigned int rgba) {
  const int r = rgba & 255;rgba>>=8;
  const int g = rgba & 255;rgba>>=8;
  const int b = rgba & 255;rgba>>=8;
  unsigned int ld = 256*256*4;
  unsigned char ri = 0;
  for (unsigned char i = 0; i < 16; i++) {
    unsigned int pal = palette[i];
    const int r2 = (pal & 255)-r;pal >>= 8; unsigned int d = r2*r2;
    const int g2 = (pal & 255)-g;pal >>= 8; d += g2*g2;
    const int b2 = (pal & 255)-b;pal >>= 8; d += b2*b2;
    if (d < ld) {
      ld = d;
      ri = i;
    }
  }
  return ri;
}

static void glFrameBufferToScreen16Colors() {
  int x,y,i,j,b;
  unsigned char *read;
  unsigned int *write;
  unsigned int *frameBuffer = glFrameBuffer0;
  int width = glFrameBufferWidth0;
  int height = glFrameBufferHeight0;
  GLubyte *buffer = glEgaIndexBuffer;
  GLubyte *b2 = buffer;
  unsigned int *r2 = frameBuffer;

  for (y = 0; y < height; y++) {
    for (x = 0; x < width; x++) {
      *b2++ = glGetPaletteIndex16(*r2++);
    }
  }

  DJGPPVGAMEMON
  for (y = 0; y < height; y++) {
    for (j = 0; j < 4; j++) {
      outp(0x3c4, 2);
      outp(0x3c5, 1<<j);
      read = &buffer[y * width];
      write = (unsigned int *)(VGARAMBASE + (width/8)*y);
      int xi = 0;
      for (x = 0; x < width/32; x++) {
        unsigned int b3 = 0;
        for (int k = 0; k < 4; k++) {
          b = 0;
          for (i = 0; i < 8; i++) {
            b |= (((read[xi])>>j) & 1)<<(7-i);
            xi++;
          }
          b3 |= b << (k*8);
        }
        *write++ = b3;
      }
    }
  }
  DJGPPVGAMEMOFF
}

static GLvoid glRefreshEga() {
  if (glIsEga) {
    glFrameBufferToScreen16Colors();
  }
}

static GLvoid glDoneEga() {
  if (glIsEga) {
    setBiosGraphicsMode(glEgaGraphicsModeToRestore);
    glSetRefreshNotification(NULL);
    glSetDoneNotification(NULL);
    glIsEga = false;
    if (glFrameBuffer0 != NULL) {
      glFree(glFrameBuffer0); glFrameBuffer0 = NULL; glFrameBuffer = NULL;
    }
    
    if (glDepthBuffer0 != NULL) {
      glFree(glDepthBuffer0); glDepthBuffer0 = NULL; glDepthBuffer = NULL;
    }
    
    if (glStencilBuffer0 != NULL) {
      glFree(glStencilBuffer0); glStencilBuffer0 = NULL; glStencilBuffer = NULL;
    }
  
    if (glEgaIndexBuffer != NULL) {
      glFree(glEgaIndexBuffer); glEgaIndexBuffer = NULL;
    }
  }
}

bool glEGA(int xRes, int yRes) {
  glIsEga = false;
  int mode = -1;
  if (xRes == 320 && yRes == 200) mode = 0x0d;
  if (xRes == 640 && yRes == 200) mode = 0x0e;
  if (xRes == 640 && yRes == 350) mode = 0x10;
  if (xRes == 640 && yRes == 480) mode = 0x12;
  if (mode == -1) return false;
  glEgaGraphicsModeToRestore = getBiosGraphicsMode();
  setBiosGraphicsMode((unsigned short)mode);
  egaPalette();
  glIsEga = true;
  GLuint *frameBuffer = (GLuint*)glMalloc(xRes*yRes*sizeof(GLuint));
  GLfloat *depthBuffer = (GLfloat*)glMalloc(xRes*yRes*sizeof(GLfloat));
  GLubyte *stencilBuffer = (GLubyte*)glMalloc(xRes*yRes*sizeof(GLubyte));
  glEgaIndexBuffer = (GLubyte*)glMalloc(xRes*yRes*sizeof(GLubyte));
  glDirect(frameBuffer, depthBuffer, stencilBuffer, xRes, yRes);
  glSetRefreshNotification(glRefreshEga);
  glSetDoneNotification(glDoneEga);
  return true;
}


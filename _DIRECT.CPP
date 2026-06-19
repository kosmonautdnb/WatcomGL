//render into a self defined framebuffer
//without any of the DOS functions/functionality at all (in WatcomGL)
//has /D__GLDISABLEDOSFUNCTIONS__=1 set with GLIMPL.CPP for compiling
//----
#include "GL.H"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>

GLfloat xrot;
GLfloat yrot;
GLfloat zrot;
GLuint texture[1];

// this is just to have some visual output on DOS, you can of course use your own (non DOS) frameworks for that
#ifdef __DJGPP__
#include <sys/nearptr.h>
#endif // __DJGPP__
#include <dos.h>
#include <string.h>
void setDosBiosGraphicsMode(unsigned short mode) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = mode;
  int386(0x10, &regs, &regs);
}

unsigned short getDosBiosGraphicsMode() {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x0f00;
  int386 (0x10, &regs, &regs);
  return regs.h.al;
}

int main() {

  int k = 1;
//#define ANTIALIAS 1
#ifdef ANTIALIAS
   k = 2; glConfigureAntiAlias();
#endif

  unsigned int *frameBuffer = (unsigned int*)malloc(320*200*k*sizeof(unsigned int));
  float *depthBuffer = (float*)malloc(320*200*k*sizeof(float));
  unsigned char *stencilBuffer = NULL; //(unsigned char*)malloc(320*200*k*sizeof(unsigned char));

  glDirect(frameBuffer, depthBuffer, stencilBuffer, 320, 200);

  unsigned short mode = getDosBiosGraphicsMode();
  setDosBiosGraphicsMode(0x13);

  unsigned char *textureData = (unsigned char *)malloc(256*256*3);
  for (int y = 0; y < 256; y++)
  for (int x = 0; x < 256; x++) {
    double d = sqrt((x-128)*(x-128)+(y-128)*(y-128));
    d = d * d / 128;
    double a = atan2(y-128,x-128);
    int r = (int)((sin(d/10.0+a*6)+sin(a*4+d/7.0))*63+128);
    a += 0.125;
    int g = (int)((sin(d/10.0+a*6)+sin(a*4+d/7.0))*63+128);
    a += 0.125;
    int b = (int)((sin(d/10.0+a*6)+sin(a*4+d/7.0))*63+128);
    int k = ((x/8) ^ (y/8)) & 3;
    r = r * k / 4;
    g = g * k / 4;
    b = b * k / 4;
    textureData[(x+y*256)*3+0] = (unsigned char)r;
    textureData[(x+y*256)*3+1] = (unsigned char)g;
    textureData[(x+y*256)*3+2] = (unsigned char)b;
    textureData[(x+y*256)*3+3] = 255;
  }

  glGenTextures(1, &texture[0]);
  glBindTexture(GL_TEXTURE_2D, texture[0]);
  glTexImage2D(GL_TEXTURE_2D,0,3,256,256,0,GL_RGB,GL_UNSIGNED_BYTE,textureData);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(textureData);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0f,(GLfloat)16.0/9.0,0.1f,100.f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);  

  while(true)
  {    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glEnable(GL_TEXTURE_2D);
    glTranslatef(0.f,0.f,-5.f);
    glRotatef(xrot,1.0f,0.0f,0.0f);
    glRotatef(yrot,0.0f,1.0f,0.0f);
    glRotatef(zrot,0.0f,0.0f,1.0f);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
  
    glColor4f(1.0f,1.0f,1.0f,0.5f);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
  
    glBegin(GL_QUADS);
  
    glNormal3f(0.0f,0.0f,1.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,1.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,1.0f);
  
    glNormal3f(0.0f,0.0f,-1.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,-1.0f);
  
    glNormal3f(0.0f,1.0f,0.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,1.0f,1.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,1.0f,1.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);
  
    glNormal3f(0.0f,-1.0f,0.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,-1.0f,-1.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);
  
    glNormal3f(1.0f,0.0f,0.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,-1.0f,-1.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,1.0f,1.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);
  
    glNormal3f(-1.0f,0.0f,0.0f);
    glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
    glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);
    glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,1.0f,1.0f);
    glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);
  
    glEnd();
  
    xrot += 0.3f;
    yrot += 0.2f;
    zrot += 0.4f;

    glRefresh(); // not needed

#ifdef __DJGPP__
    __djgpp_nearptr_enable();
    for (int i = 0; i < 320*200; i++) ((unsigned char*)(0xa0000+__djgpp_conventional_base))[i] = ((frameBuffer[i] & 255) / 16)+16;
    __djgpp_nearptr_disable();
#endif // __DJGPP__

#ifdef __WATCOMC__
    for (int i = 0; i < 320*200; i++) ((unsigned char*)0xa0000)[i] = ((frameBuffer[i] & 255) / 16)+16;
#endif // __WATCOMC__

    if (glNextKey() == 'a') break; // test if DOS is active in the WatcomGL obj
    if (kbhit()) if (getch() != 'a') break;
  }
  glDone();

  if (frameBuffer != NULL) free(frameBuffer);
  if (depthBuffer != NULL) free(depthBuffer);
  if (stencilBuffer != NULL) free(stencilBuffer);

  setDosBiosGraphicsMode(mode);

  return 0;
}

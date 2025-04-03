#include "gl.h"
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define XRES 640
#define YRES 480

typedef unsigned char Uint8;

int counter;
char quit=0;   

extern char biisi[];
extern void analyysi(signed short *);
extern int osa(int);

void pakki(void *udata, Uint8 *stream, int len)
{
    //syna_play((short *)stream,len/4);
    analyysi((signed short*)stream);
}

int SDL_GetTicks() {
  static int i = 0; i+=100;
  counter = i * (44100/1000);
  return i;
}

int main(int argc,char *argv[])
{    
  glVesa(XRES,YRES,32);
  glViewport(0,0,XRES,YRES);
  glSetMonitorAspectRatio(16.0/9.0);

  
  int orig=SDL_GetTicks();
  
  while(!quit)
  {
      if(osa(SDL_GetTicks()-orig)==0)
        glRefresh();
      if ((inp(0x60)&0x80)==0) 
        quit = 1;
  }
  glDone();
  return 0;
}

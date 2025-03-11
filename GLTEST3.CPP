#include "gl.h"	
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "png\png.h"
#include <time.h>

#define XRES 640
#define YRES 480
#define NEARPLANE 15.f
#define FARPLANE 1000.f
#define FOV 90.f

#define LINEAR GL_LINEAR

float frand() {
  return (float)rand()/RAND_MAX;
}

float globalTime;

bool keyPressed() {
  return (inp(0x60)&0x80)==0x00;
}

int main(int argc, char **argv) {
  glVesa(XRES,YRES,32);
  glViewport(0,0,XRES,YRES);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  unsigned int startClock = clock();
  do {
    globalTime = (float)(clock() - startClock) / CLOCKS_PER_SEC;
    glClearColor(0.2f,0.4f,0.6f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, (double)XRES/(double)YRES, NEARPLANE, FARPLANE);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glBegin(GL_LINE_STRIP);
    glColor3f(1,0,0);
    glVertex3f(-10.5f,10.5f,-20.f);
    glColor3f(0,1,0);
    glVertex3f(10.5f,10.5f,-20.f);
    glColor3f(0,0,1);
    glVertex3f(10.5,-10.5,-20.f);
    glColor3f(0,1,1);
    glVertex3f(-10.5f,-10.5,-10.f+globalTime);
    glColor3f(1,0,0);
    glVertex3f(-10.5f,10.5f,-20.f);
    glEnd();

    glRefresh();
  } while(!keyPressed());
  glDone();
  return 0;
}
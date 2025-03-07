#include "gl.h"	
#include <conio.h>
#include <stdlib.h>
#include <math.h>

float frand() {
  return (float)rand()/RAND_MAX;
}

float time = 0.f;

float h(int x, int y) {
  return sin((x+time*3.f)/5.f+sin(x*0.75)*0.1)*cos((y-time*2.f)/10.f+sin(y*0.75)*0.1)*0.5+0.5;
}

float h2(int x, int y) {
  return (h(x+1,y)-h(x-1,y))*20+(h(x,y+1)-h(x,y-1))*20+2.f;
}

int main(int argc, char **argv) {
  glVesa(640,480,32);
  glEnable(GL_TEXTURE_2D);
  unsigned int tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
#define texx 32
#define texy 32
  unsigned int *t = new unsigned int[texx*texy];
  for (int y = 0; y < texy; y++) {
    for (int x = 0; x < texx; x++) {
      float d = sqrt(x*x/100+y*y/100); 
      t[x+y*texx] = 0x80+d*200*200*100+d*200+d*200*200;
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texx, texy, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);

  do
  {		
    glClearColor(0.2f,0.4f,0.6f,1.f);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(120.0, (double)640/(double)480, 2.0, 50000.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0,600,-800,0,0,0,0,1,0);
    glViewport(0,0,640,480);
    glBegin(GL_QUADS);
    const int SEGX = 40;
    const int SEGY = 40;
    for (int x = -SEGX; x < SEGX; x++) {
      for (int y = -SEGY; y < SEGY; y++) {
        float xp[4],yp[4],zp[4],cp[4], tx[4], ty[4];
        for (int y2 = 0; y2 < 2; y2++) {
          for (int x2 = 0; x2 < 2; x2++) {
            cp[x2+y2*2] = h(x+x2,y+y2);
            xp[x2+y2*2] = (x+x2)*80;
            yp[x2+y2*2] = cp[x2+y2*2]*400-200;
            zp[x2+y2*2] = (y+y2)*40+time*10.f;
            tx[x2+y2*2] = (x+x2)/10.f;
            ty[x2+y2*2] = (y+y2)/10.f;
          }
        }
        float c; 
        c = h2(x,y)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[0],ty[0]);
        glVertex3f(xp[0],yp[0],zp[0]);
        c = h2(x+1,y)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[1],ty[1]);
        glVertex3f(xp[1],yp[1],zp[1]);
        c = h2(x+1,y+1)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[3],ty[3]);
        glVertex3f(xp[3],yp[3],zp[3]);
        c = h2(x,y+1)*0.5+0.5;
        glColor3f(c*0.4,c*0.5,c*0.6);
        glTexCoord2f(tx[2],ty[2]);
        glVertex3f(xp[2],yp[2],zp[2]);
      }
    }
    glEnd();      
    time += 0.25f;
    glRefresh();
  } while((inp(0x60)&0x80)==0x80);
  glDone();
  return 0;
}
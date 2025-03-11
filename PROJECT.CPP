#include "gl.h"	
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "png\png.h"
#include <time.h>

#define XRES 320
#define YRES 200
#define NEARPLANE 0.01f
#define FARPLANE 1000.f
#define FOV 120.f

float globalTime;
GLuint skyBox[6];

float frand() {
  return (float)rand()/RAND_MAX;
}

bool keyPressed() {
  return (inp(0x60)&0x80)==0x00;
}

typedef struct Image {
  unsigned int width;
  unsigned int height;
  unsigned char *data;
} Image;

Image loadTGA24BGR(const char *fileName, int width, int height) {
  Image r;
  memset(&r,0,sizeof(Image));
  FILE *in = fopen(fileName, "rb");
  if (in == NULL) return r;
  fseek(in, 0, SEEK_END);
  int fileSize = ftell(in);
  fseek(in, 0, SEEK_SET);
  unsigned char basicHeader[0x12];
  fread(basicHeader,1,0x12,in);
  r.width = basicHeader[0x0c]+(int)basicHeader[0x0d]*0x100;
  r.height = basicHeader[0x0e]+(int)basicHeader[0x0f]*0x100;
  r.data = new unsigned char[r.width * r.height * 3];    
  fread(r.data, 1, r.width * r.height * 3, in);
  if (r.width != width || r.height != height) {
    unsigned char *data2 = new unsigned char[width*height*3];
    for (int y = 0; y < height; y++) {
      int ry = y * r.height / height;
      for (int x = 0; x < width; x++) {
        int rx = x * r.width / width;
        unsigned char *a = &r.data[(rx + ry * r.width) * 3];
        unsigned char *b = &data2[(x+y*width) * 3];
        b[0] = a[0];
        b[1] = a[1];
        b[2] = a[2];
      }
    }
    delete[] r.data;
    r.data = data2;
    r.width = width;
    r.height = height;
  }
  fclose(in);
  return r;
}

void glLoadSkyBox(const char *maskString, const char *left,const char *right,const char *front,const char *back,const char *top,const char *bottom) {
  glGenTextures(6, skyBox);
  const char *names[6] = {
    left,
    right,
    front,
    back,
    top,
    bottom
  };
  for (int i = 0; i < 6; i++) {
    char buffer[0x100];
    sprintf(buffer, maskString, names[i]);
    Image img = loadTGA24BGR(buffer, 256, 256);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyBox[i]);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i,0,GL_RGBA,img.width,img.height,0,GL_BGR,GL_UNSIGNED_BYTE,img.data);
    delete[] img.data;
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
}

void cube(float x, float y, float z, float sx, float sy, float sz) {
  glBegin(GL_QUADS);
  glNormal3f(0,0,1);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,y-sy,z+sz);

  glNormal3f(0,1,0);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,y+sy,z+sz);

  glNormal3f(1,0,0);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,x-sx,z+sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,x+sx,z+sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,x+sx,z-sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,x-sx,z-sz);

  glNormal3f(0,0,-1);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,y+sy,z-sz);

  glNormal3f(0,-1,0);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,y-sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,y-sy,z-sz);

  glNormal3f(-1,0,0);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,x-sx,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,x+sx,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,x+sx,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,x-sx,z+sz);
  glEnd();
}

void skyCube(float x, float y, float z, float sx, float sy, float sz) {

  int k = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_Z-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,0,1);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,y-sy,z+sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_Y-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,1,0);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,y+sy,z+sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_X-k]);
  glBegin(GL_QUADS);
  glNormal3f(1,0,0);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,x-sx,z+sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,x+sx,z+sz);
  glTexCoord2f(1,1);
  glVertex3f(x+sx,x+sx,z-sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,x-sx,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,0,-1);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,y+sy,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,-1,0);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,y-sy,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);
  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,y-sy,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_X-k]);
  glBegin(GL_QUADS);
  glNormal3f(-1,0,0);
  glTexCoord2f(0,0);
  glVertex3f(x-sx,x-sx,z-sz);
  glTexCoord2f(0,1);
  glVertex3f(x-sx,x+sx,z-sz);
  glTexCoord2f(1,1);
  glVertex3f(x-sx,x+sx,z+sz);
  glTexCoord2f(1,0);
  glVertex3f(x-sx,x-sx,z+sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[0]);
}

int main(int argc, char **argv) {
  glVesa(XRES,YRES,32);
  glViewport(0,0,XRES,YRES);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glEnable(GL_DEPTH_TEST);
  glLoadSkyBox("TEXTURES\\%s.TGA","sky0002","sky0005","sky0003","sky0006","sky0001","sky0004");
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
    gluLookAt(0,0,-1, 0,0,0, 0,1,0);
    glPushMatrix();
    glRotatef(globalTime*10*0.5,0,1,0);
    glRotatef(globalTime*3*0.5,1,0,0);
    glRotatef(globalTime*5*0.5,0,0,1);

    glCullFace(GL_FRONT);
    skyCube(0,0,0,10.f,10.f,10.f);
      
    glPopMatrix();
    glTranslatef(0,0,4);
    glRotatef(globalTime*23,0,1,0);
    glRotatef(globalTime*17,1,0,0);
    glRotatef(globalTime*19,0,0,1);
    glCullFace(GL_BACK);
    skyCube(0,0,0,0.5f,0.5f,0.5f);

    float t[][3] = { 
      {1,0,0},{-1,0,0},
      {0,1,0},{0,-1,0},
      {0,0,1},{0,0,-1},
      {1,1,0},{-1,-1,0},
      {0,1,1},{0,-1,-1},
      {1,0,1},{-1,0,-1}
    };
    for (int i = 0; i < 12; i++) {
      glPushMatrix();
      for (int j = 0; j < 10; j++) {
        glTranslatef(t[i][0]*0.5,t[i][1]*0.5,t[i][2]*0.5);
        float k = sin(-globalTime*1+j*1.5)*0.4+0.6;
        glDisable(GL_TEXTURE_2D);
        if (((int)(j-globalTime*3) & 3)==0)
          cube(0,0,0,0.5f*k,0.5f*k,0.5f*k);
        else
          skyCube(0,0,0,0.5f*k,0.5f*k,0.5f*k);
      }
      glPopMatrix();
    }


    glRefresh();
  } while(!keyPressed());
  glDone();
  return 0;
}
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
#define NEARPLANE 40.f
#define FARPLANE 10000.f
#define FOV 90.f

#define LINEAR GL_LINEAR

float frand() {
  return (float)rand()/RAND_MAX;
}

float globalTime;

bool keyPressed() {
  return (inp(0x60)&0x80)==0x00;
}

void getVertexFromScreen(float screenX, float screenY, float screenZ, double *dest) {
  GLdouble modelView[16];
  GLdouble projection[16];
  GLint viewport[4];
  glGetDoublev(GL_MODELVIEW_MATRIX,modelView);
  glGetDoublev(GL_PROJECTION_MATRIX,projection);
  glGetIntegerv(GL_VIEWPORT,viewport);
  gluUnProject(screenX, screenY, screenZ, modelView, projection, viewport, &dest[0], &dest[1], &dest[2]);
}


void projectVertexToScreen(float objX, float objY, float objZ, double *dest) {
  GLdouble modelView[16];
  GLdouble projection[16];
  GLint viewport[4];
  glGetDoublev(GL_MODELVIEW_MATRIX,modelView);
  glGetDoublev(GL_PROJECTION_MATRIX,projection);
  glGetIntegerv(GL_VIEWPORT,viewport);
  gluProject(objX, objY, objZ, modelView, projection, viewport, &dest[0], &dest[1], &dest[2]);
}

float cameraPosX = 0;
float cameraPosY = 0;
float cameraPosZ = 0;
float cameraRotation = 0;
float lookDirX = 0;
float lookDirY = 0;
float lookDirZ = 1;
GLuint cloudTexture;
GLuint groundTexture;
GLuint pseudoShadowTexture;

void generateCloudTexture() {
  unsigned int tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
  unsigned int *t = new unsigned int[256*256];
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      float xi = x / 256.f * 2.f * 3.14159f*10.f;
      float yi = y / 256.f * 2.f * 3.14159f*7.f;
      float c1 = sin(xi*0.1+yi*0.3+sin(xi))*sin(yi*-0.3+xi*0.2+sin(yi))*0.5+0.5;
      float c2 = sin(xi*0.1+yi*0.3+sin(xi)+1)*sin(yi*-0.3+xi*0.2+sin(yi))*0.5+0.5;
      int r = c2*64.f+192.f;
      int r2 = c1 * 255.f;
      t[x+y*256] = r|(r<<8)|(r<<16)|(r2<<24);
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);
  cloudTexture = tex;
}

void generatePseudoShadowTexture() {
  unsigned int tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
  unsigned int *t = new unsigned int[256*256];
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      float xi = x / 256.f * 2.f * 3.14159f*10.f;
      float yi = y / 256.f * 2.f * 3.14159f*7.f;
      float c2 = sin(xi*0.1+yi*0.3+sin(xi)+1)*sin(yi*-0.3+xi*0.2+sin(yi))*0.5+0.5;
      int r = c2*255.f;
      t[x+y*256] = r|(r<<8)|(r<<16);
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);
  pseudoShadowTexture = tex;
}

void generateGroundTexture() {
  unsigned int tex;
  glGenTextures(1,&tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,LINEAR);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_MIRRORED_REPEAT);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_MIRRORED_REPEAT);
  unsigned int *t = new unsigned int[256*256];
  for (int y = 0; y < 256; y++) {
    for (int x = 0; x < 256; x++) {
      float xi = x / 256.f * 2.f * 3.14159f*10.f;
      float yi = y / 256.f * 2.f * 3.14159f*7.f;
      float c2 = sin(xi*0.1+yi*0.3+sin(xi)+1)*sin(yi*-0.3+xi*0.2+sin(yi))*0.5+0.5;
      c2 += frand()*4;
      int r = c2*100.f+155.f;
      int g = r * 0.7;
      int b = r * 0.5;
      if (r > 255) r = 255;
      if (g > 255) g = 255;
      if (b > 255) b = 255;
      float k = sin(xi+yi*0.3)+sin(yi+xi*2);
      if (fabs(k)<0.5) {r*=0.4;g*=0.4;b*=0.4;}
      t[x+y*256] = r|(g<<8)|(b<<16);
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, t);
  groundTexture = tex;
}

void drawSky_Gradient() {

  glEnable(GL_BLEND);
  glBlendFunc(GL_ONE,GL_ONE);
  glDisable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  const int SKYDOMESEGS = 32;
  const float SKYDOMERADIUS = FARPLANE*2/3;
  const float DOMEHEIGHT = SKYDOMERADIUS;
  float colorRed = 0.5f;
  float colorGreen = 0.5f;
  float colorBlue = 0.5f;
  for (int x = 0; x < SKYDOMESEGS; x++) {
    float xi0 = (float)x / SKYDOMESEGS * 2.f * 3.1415927f;
    float xi1 = (float)(x+1) / SKYDOMESEGS * 2.f * 3.1415927f;
    float xp0 = sin(xi0) * SKYDOMERADIUS + cameraPosX;
    float zp0 = cos(xi0) * SKYDOMERADIUS + cameraPosZ;
    float xp1 = sin(xi1) * SKYDOMERADIUS + cameraPosX;
    float zp1 = cos(xi1) * SKYDOMERADIUS + cameraPosZ;
    glColor3f(colorRed,colorGreen,colorBlue);
    glVertex3f(xp0,cameraPosY,zp0);
    glVertex3f(xp1,cameraPosY,zp1);
    glColor3f(0,0,0);
    glVertex3f(xp1,cameraPosY+DOMEHEIGHT,zp1);
    glVertex3f(xp0,cameraPosY+DOMEHEIGHT,zp0);
  }
  glEnd();
}

float cloud(float x, float z) {
  return sin(x+z+globalTime)*cos(x*0.1-z*0.2)*0.5 + 0.5;
}

void drawSky_CloudLayer(float h) {
  glEnable(GL_BLEND);
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, cloudTexture);
  glBegin(GL_QUADS);
  float xSteps = 0.5f;
  float ySteps = 0.5f;
  float cloudY = 200.f+h*200.f;
  float cloudLayerSize = 1500.f;
  for (float y = -1.f; y < 1.f; y += ySteps) {
    for (float x = -1.f; x <= 1.f; x += xSteps) {
      float ix[4]={0,1,1,0};
      float iz[4]={0,0,1,1};
      for (int i = 0; i < 4; i++) {
        float l = 1.f-sqrt((y+iz[i]*ySteps)*(y+iz[i]*ySteps)+(x+ix[i]*xSteps)*(x+ix[i]*xSteps)); if (l < 0) l = 0;
        float xp = (ix[i]*xSteps+x)*cloudLayerSize+cameraPosX;
        float zp = (iz[i]*ySteps+y)*cloudLayerSize+cameraPosZ;
        float tx = (x+ix[i]*xSteps)*(0.5-h)+cameraPosX/cloudLayerSize+0.3;
        float ty = (y+iz[i]*ySteps)*0.5+cameraPosZ/cloudLayerSize+globalTime*0.1f*(h+1)-0.3;
        tx += sin(ty)+2.f+h;
        ty += sin(tx)*0.5;
        glColor4f(1,1,1,l);
        glTexCoord2f(tx,ty);
        glVertex3f(xp,cloudY+sin(l*5)*10.f,zp);
      }
    }
  }
  glEnd();
  glEnable(GL_DEPTH_TEST);
}

float sinp(float f) {
  return (pow(sin(f)*0.5+0.5,10.0)-0.5)*2;
}

float getHeight(float x, float y) {
  x *= 0.003;
  y *= 0.003; 
  return (sinp(x+y*0.3)*cos(x*0.2+y*0.7)*0.4+0.6)*550.f-500.f+sinp(x*4.f+y*0.3+sinp(y*3.1+x*0.5)+sinp(y*5.3-x*0.7))*10.f;
}

float sign(float v) {
  return v < 0 ? -1 : v > 0 ? 1 : 0;
}

void drawGround(int pass) {
  if (pass == 0) {
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE,GL_ONE);
    glBindTexture(GL_TEXTURE_2D, groundTexture);
  } else {
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, pseudoShadowTexture);
    glBlendFunc(GL_ZERO,GL_SRC_COLOR);
    glColor3f(1,1,1);
  }
  glEnable(GL_TEXTURE_2D);
  glBegin(GL_QUADS);
  float xSteps = 0.02f;
  float ySteps = 0.02f;
  float groundLayerSize = 4000.f;
  int stepX = floor(cameraPosX/(xSteps*groundLayerSize));
  int stepY = floor(cameraPosZ/(ySteps*groundLayerSize));
  for (float y = -1.f; y < 1.f; y += ySteps) {
    for (float x = -1.f; x <= 1.f; x += xSteps) {
      float ix[4]={0,1,1,0};
      float iz[4]={0,0,1,1};
      for (int i = 0; i < 4; i++) {
        float xa = ix[i]*xSteps+x;
        float ya = iz[i]*ySteps+y;
        float xp = xa*groundLayerSize+stepX*(xSteps*groundLayerSize);
        float zp = ya*groundLayerSize+stepY*(ySteps*groundLayerSize);
        xp += sin(zp*0.4)*20.f;
        zp += sin(xp*0.4)*20.f;
        float height = getHeight(xp,zp);
        if (pass == 0) {
          float d = (getHeight(xp,zp)-getHeight(xp+1.,zp))*2+0.9;
          float tx = (x+ix[i]*xSteps+stepX*xSteps)*1;
          float ty = (y+iz[i]*ySteps+stepY*ySteps)*1;
          tx += sin(ty*2.f);
          ty += sin(tx*2.f);
          glColor3f(d*0.5,d,d*0.1);
          glTexCoord2f(tx,ty);
          glVertex3f(xp,height,zp);
        } else {
          float s = 0.0001;
          glTexCoord2f(xp*s+globalTime*0.1,zp*s);
          glVertex3f(xp,height,zp);
        }
      }
    }
  }
  glEnd();
}

void drawSeaLevel() {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_COLOR,GL_SRC_COLOR);
  float xSteps = 0.1f;
  float ySteps = 0.1f;
  float seaLayerSize = 4000.f;
  float height = -350.f;
  int stepX = floor(cameraPosX/(xSteps*seaLayerSize));
  int stepY = floor(cameraPosZ/(ySteps*seaLayerSize));
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, cloudTexture);
  glBegin(GL_QUADS);
  for (float y = -1.f; y < 1.f; y += ySteps) {
    for (float x = -1.f; x <= 1.f; x += xSteps) {
      float ix[4]={0,1,1,0};
      float iz[4]={0,0,1,1};
      for (int i = 0; i < 4; i++) {
        float xa = ix[i]*xSteps+x;
        float ya = iz[i]*ySteps+y;
        float xp = xa*seaLayerSize+stepX*(xSteps*seaLayerSize);
        float zp = ya*seaLayerSize+stepY*(ySteps*seaLayerSize);
        glColor3f(0.5,0.5,1);
        glTexCoord2f(xp*0.001,zp*0.001);
        glVertex3f(xp,height,zp);
      }
    }
  }
  glEnd();
}


int main(int argc, char **argv) {
  glVesa(XRES,YRES,32);
  glViewport(0,0,XRES,YRES);
  generateCloudTexture();
  generateGroundTexture();
  generatePseudoShadowTexture();
  glDepthFunc(GL_LEQUAL);
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
    cameraPosX = cos(globalTime*0.01) * 10000.f;
    cameraPosZ = sin(globalTime*0.01) * 10000.f;
    cameraPosY = getHeight(cameraPosX,cameraPosZ)+50;
    cameraRotation = sin(globalTime * 0.5f)*0.02;
    lookDirY = 0.15;
    lookDirX = cos(globalTime*1.)*0.5;

    gluLookAt(cameraPosX,cameraPosY+20,cameraPosZ,cameraPosX+lookDirX,getHeight(cameraPosX+lookDirX*0.05,cameraPosZ+lookDirZ*0.05)+lookDirY+50+20.2,cameraPosZ+lookDirZ,sin(cameraRotation),cos(cameraRotation),0);
    drawGround(0);
    drawGround(1);
    drawSeaLevel();
    drawSky_Gradient();
    drawSky_CloudLayer(0.f);
    drawSky_CloudLayer(1.f);
    glRefresh();
  } while(!keyPressed());
  glDone();
  return 0;
}
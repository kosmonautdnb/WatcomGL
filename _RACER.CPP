#include "gl.h"	
#include <conio.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dos.h>

#define XRES 320
#define YRES 200
#define NEARPLANE 0.1f
#define FARPLANE 1500.f
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

Image loadTGA32BGRA(const char *fileName, int width, int height) {
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
  r.data = new unsigned char[r.width * r.height * 4];    
  fread(r.data, 1, r.width * r.height * 4, in);
  if (r.width != width || r.height != height) {
    unsigned char *data2 = new unsigned char[width*height*4];
    for (int y = 0; y < height; y++) {
      int ry = y * r.height / height;
      for (int x = 0; x < width; x++) {
        int rx = x * r.width / width;
        unsigned char *a = &r.data[(rx + ry * r.width) * 4];
        unsigned char *b = &data2[(x+y*width) * 4];
        b[0] = a[0];
        b[1] = a[1];
        b[2] = a[2];
        b[3] = a[3];
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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }
}

void randomColor() {
  float w = (float)rand() / RAND_MAX;
  w = w * 0.2 + 0.8;
  glColor3f(w,w,w);
}

void cube(float x, float y, float z, float sx, float sy, float sz) {
  glBegin(GL_QUADS);
  glNormal3f(0,0,1);
  randomColor();
  glTexCoord2f(0,0);  glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,0);  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,1); glVertex3f(x-sx,y-sy,z+sz);
  glNormal3f(0,1,0);
  randomColor();
  glTexCoord2f(0,1);  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,0);  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x-sx,y+sy,z+sz);
  glNormal3f(1,0,0);
  randomColor();
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);   glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);    glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);   glVertex3f(x+sx,y-sy,z-sz);
  glNormal3f(0,0,-1);
  randomColor();
  glTexCoord2f(1,1);   glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,0);   glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,0);    glVertex3f(x-sx,y+sy,z-sz);
  glNormal3f(0,-1,0);
  randomColor();
  glTexCoord2f(1,0);  glVertex3f(x-sx,y-sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x-sx,y-sy,z-sz);
  glNormal3f(-1,0,0);
  randomColor();
  glTexCoord2f(0,1);  glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(1,0);   glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,1);   glVertex3f(x-sx,y-sy,z+sz);
  glEnd();
}

void skyCube(float x, float y, float z, float sx, float sy, float sz) {

  int k = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_Z-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,0,1);
  glTexCoord2f(0,0);  glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,0);  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,1); glVertex3f(x-sx,y-sy,z+sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_Y-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,1,0);
  glTexCoord2f(0,1);  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,0);  glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x-sx,y+sy,z+sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_POSITIVE_X-k]);
  glBegin(GL_QUADS);
  glNormal3f(1,0,0);
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,0);   glVertex3f(x+sx,y+sy,z+sz);
  glTexCoord2f(1,0);    glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,1);   glVertex3f(x+sx,y-sy,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_Z-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,0,-1);

  glTexCoord2f(1,1);   glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,0);   glVertex3f(x+sx,y+sy,z-sz);
  glTexCoord2f(1,0);    glVertex3f(x-sx,y+sy,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_Y-k]);
  glBegin(GL_QUADS);
  glNormal3f(0,-1,0);

  glTexCoord2f(1,0);  glVertex3f(x-sx,y-sy,z+sz);
  glTexCoord2f(1,1);  glVertex3f(x+sx,y-sy,z+sz);
  glTexCoord2f(0,1);  glVertex3f(x+sx,y-sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x-sx,y-sy,z-sz);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[GL_TEXTURE_CUBE_MAP_NEGATIVE_X-k]);
  glBegin(GL_QUADS);
  glNormal3f(-1,0,0);
  
  glTexCoord2f(0,1);  glVertex3f(x-sx,y-sy,z-sz);
  glTexCoord2f(0,0);  glVertex3f(x-sx,y+sy,z-sz);
  glTexCoord2f(1,0);   glVertex3f(x-sx,y+sy,z+sz);
  glTexCoord2f(1,1);   glVertex3f(x-sx,y-sy,z+sz);
  
  glEnd();

  glBindTexture(GL_TEXTURE_2D, skyBox[0]);
}

class TrackSegment {
public:
  float matrix[16];
};

#define TRACKSEGMENTS 1000
#define TRACKSCALE 4.f

TrackSegment segments[TRACKSEGMENTS];

void buildTrack() {
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  for(int i = 0; i < TRACKSEGMENTS; i++) {
    float fi = (float)i / TRACKSEGMENTS * 20.f;
    glGetFloatv(GL_MODELVIEW_MATRIX, segments[i].matrix);
    glTranslatef(0,0,-TRACKSCALE/2);
    glRotatef(1*sin(fi*1.7+sin(fi*5.3)),0,0,1);
    glRotatef(1*sin(fi*3.5+sin(fi*12.6)+0.5f*sin(fi*9.0)),1,0,0);
    glRotatef(1*sin(fi*3.3),0,1,0);
  }
}

void trackVectorf(float *f,float trackSegment, float x, float y) {
  x *= TRACKSCALE;
  y *= TRACKSCALE;
  float *matrix1 = segments[(int)(floor(trackSegment)) % TRACKSEGMENTS].matrix;
  float *matrix2 = segments[(int)(floor(trackSegment)+1) % TRACKSEGMENTS].matrix;
  float iz = fmod(trackSegment,1.f); float z = 0;
  float x1 = matrix1[0+4*0]*x+matrix1[0+4*1]*y+matrix1[0+4*2]*z+matrix1[0+4*3];
  float y1 = matrix1[1+4*0]*x+matrix1[1+4*1]*y+matrix1[1+4*2]*z+matrix1[1+4*3];
  float z1 = matrix1[2+4*0]*x+matrix1[2+4*1]*y+matrix1[2+4*2]*z+matrix1[2+4*3];
  float x2 = matrix2[0+4*0]*x+matrix2[0+4*1]*y+matrix2[0+4*2]*z+matrix2[0+4*3];
  float y2 = matrix2[1+4*0]*x+matrix2[1+4*1]*y+matrix2[1+4*2]*z+matrix2[1+4*3];
  float z2 = matrix2[2+4*0]*x+matrix2[2+4*1]*y+matrix2[2+4*2]*z+matrix2[2+4*3];
  f[0] = (x2 - x1) * iz + x1;
  f[1] = (y2 - y1) * iz + y1;
  f[2] = (z2 - z1) * iz + z1;
}

void trackVector(float trackSegment, float x, float y) {
  float f[3];
  trackVectorf(f, trackSegment, x, y);
  glVertex3f(f[0],f[1],f[2]);
}

void getInterpolatedTrackMatrix(float *m, float step) {
  float *matrix1 = segments[(int)(floor(step)) % TRACKSEGMENTS].matrix;
  float *matrix2 = segments[(int)(floor(step)+1) % TRACKSEGMENTS].matrix;
  float iz = fmod(step,1.f);
  for (int i = 0; i < 16; i++) m[i] = ((matrix2[i]-matrix1[i])*iz+matrix1[i]);
}

void randomColor(float f) {
  float k = sin(f*10.0f)*0.1+0.9;
  glColor3f(k,k,k);
}

void drawHouse(float step, float x, float y) {
  float f[3];
  trackVectorf(f, step, x, y);
  glDisable(GL_TEXTURE_2D);
  glColor3f(1.f,1.f,1.f);
  for (int i = -8; i < 40; i++) {
    cube( f[0],f[1]+i*5,f[2], 25,1,25);
  }
}

GLuint street[10];

int main(int argc, char **argv) {

  const bool pureVGA = true;
  if (pureVGA) {
    glVGA();
  } else {
    if (!glVesa(XRES,YRES,32)) {
      printf("No Vesa %dx%dx32 found.\n", XRES, YRES);
      sleep(1);
      if (!glVGA())
      {
        printf("No VGA device found.\n");
        exit(0);
      }
    }
  }

  glDepthFunc(GL_LEQUAL);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glLoadSkyBox("TEXTURES\\%s.TGA","sky0002","sky0005","sky0003","sky0006","sky0001","sky0004");
  buildTrack();
   
  char *textureNames[] = {"TEXTURES\\s1.TGA","TEXTURES\\s2.TGA","TEXTURES\\s3.TGA"};
  for (int t = 0; t < 3; ++t) {
    glGenTextures(1, &street[t]);
    Image img = loadTGA32BGRA(textureNames[t], 64, 64);
    glBindTexture(GL_TEXTURE_2D, street[t]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,img.width,img.height,0,GL_BGRA,GL_UNSIGNED_BYTE,img.data);
    delete[] img.data;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
  }

  unsigned int startClock = clock();
  do {
    globalTime = (float)(clock() - startClock) / CLOCKS_PER_SEC;

    for (int r = 1; r < 2; r++) {
      if (r == 0) {
        glSetRenderTarget(glGetTexturePointer(street[0]),(float*)glGetTexturePointer(street[1]),glGetTextureWidth(street[0]),glGetTextureHeight(street[0]));
        glViewport(0,0,glGetTextureWidth(street[0]),glGetTextureHeight(street[0]));
      } else {
        glSetRenderTarget(NULL,NULL,0,0);
        glViewport(0,0,glFrameBufferWidth,glFrameBufferHeight);
      }

      glClearColor(0.2f,0.4f,0.6f,1.0f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrontFace(GL_CW); // sorry
      gluPerspective(FOV, (double)glFrameBufferWidth/(double)glFrameBufferHeight, NEARPLANE, FARPLANE);
      glMatrixMode(GL_MODELVIEW);
/*
    glLoadIdentity();
    gluLookAt(0,0,-1, 0,0,0, 0,1,0);
    glPushMatrix();
    glRotatef(globalTime*10*0.5,0,1,0);
    glRotatef(globalTime*3*0.5,1,0,0);
    glRotatef(globalTime*5*0.5,0,0,1);

    glCullFace(GL_FRONT);
    skyCube(0,0,0,100.f,100.f,100.f);
    glPopMatrix();
    glCullFace(GL_BACK);

    glTranslatef(0,0,4);
    glRotatef(globalTime*23,0,1,0);
    glRotatef(globalTime*17,1,0,0);
    glRotatef(globalTime*19,0,0,1);
    skyCube(0,0,0,0.5f,0.5f,0.5f);

    float t[][3] = { 
      {1,0,0},{-1,0,0},
      {0,1,0},{0,-1,0},
      {0,0,1},{0,0,-1},
      {1,1,0},{-1,-1,0},
      {0,1,1},{0,-1,-1},
      {1,0,1},{-1,0,-1}
    };
    int i;
    for (i = 0; i < 12; i++) {
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
*/
  
      glEnable(GL_TEXTURE_2D);
      glLoadIdentity(); 
      gluLookAt(0,2,1, 0,2,0, 0,1,0);
  
      float m[16];
      getInterpolatedTrackMatrix(m,globalTime*4.f);
      gluInvertMatrixf(m,m);
      glMultMatrixf(m);
  
  
      int i;
      for (i = 0; i < TRACKSEGMENTS/TRACKSCALE-1; i++) {
        glBegin(GL_QUADS);
        glColor3f(1,1,1);
  
        float tr = 0.45f;
        float th = 0.25f;
  
        float i0 = i;
        float i1 = (i+1);
        glBindTexture(GL_TEXTURE_2D, street[2]);
        randomColor(i+0);
        glTexCoord2f(0,1); trackVector(i1,-1-tr-tr,th);
        glTexCoord2f(1,1); trackVector(i1,-1-tr,th);
        glTexCoord2f(1,0); trackVector(i0,-1-tr,th);
        glTexCoord2f(0,0); trackVector(i0,-1-tr-tr,th);
  
        randomColor(i+100);
        glTexCoord2f(0,1); trackVector(i1,-1-tr,th);
        glTexCoord2f(1,1); trackVector(i1,-1,0);
        glTexCoord2f(1,0); trackVector(i0,-1,0);
        glTexCoord2f(0,0); trackVector(i0,-1-tr,th);
  
        glBindTexture(GL_TEXTURE_2D, street[0]);
        randomColor(i+200);
        glTexCoord2f(0,1); trackVector(i1,-1,0);
        glTexCoord2f(1,1); trackVector(i1,0,0);
        glTexCoord2f(1,0); trackVector(i0,0,0);
        glTexCoord2f(0,0); trackVector(i0,-1,0);
  
        randomColor(i+400);
        glTexCoord2f(0,1); trackVector(i1,0,0);
        glTexCoord2f(1,1); trackVector(i1,1,0);
        glTexCoord2f(1,0); trackVector(i0,1,0);
        glTexCoord2f(0,0); trackVector(i0,0,0);
  
        glBindTexture(GL_TEXTURE_2D, street[2]);
        randomColor(i+500);
        glTexCoord2f(0,1); trackVector(i1,1,0);
        glTexCoord2f(1,1); trackVector(i1,1+tr,th);
        glTexCoord2f(1,0); trackVector(i0,1+tr,th);
        glTexCoord2f(0,0); trackVector(i0,1,0);
  
        randomColor(i+600);
        glTexCoord2f(0,1); trackVector(i1,1+tr,th);
        glTexCoord2f(1,1); trackVector(i1,1+tr+tr,th);
        glTexCoord2f(1,0); trackVector(i0,1+tr+tr,th);
        glTexCoord2f(0,0); trackVector(i0,1+tr,th);
  
        glBindTexture(GL_TEXTURE_2D, street[1]);
  
        float th20 = pow(sin(i)*sin(i*0.1)*0.5+0.5,10.f)*20.f+th;
        float th21 = pow(sin(i+1)*sin((i+1)*0.1)*0.5+0.5,10.f)*20.f+th;
        randomColor(i+700);
        glTexCoord2f(0,1); trackVector(i1,1+tr+tr,th);
        glTexCoord2f(1,1); trackVector(i1,1+tr+tr+125,th21);
        glTexCoord2f(1,0); trackVector(i0,1+tr+tr+125,th20);
        glTexCoord2f(0,0); trackVector(i0,1+tr+tr,th);
  
        randomColor(i+800);
        glTexCoord2f(0,1); trackVector(i1,-1-tr-tr-125,th21);
        glTexCoord2f(1,1); trackVector(i1,-1-tr-tr,th);
        glTexCoord2f(1,0); trackVector(i0,-1-tr-tr,th);
        glTexCoord2f(0,0); trackVector(i0,-1-tr-tr-125,th20);
  
        glEnd();
      }


      glDisable(GL_TEXTURE_2D);
      srand(0);
      for (i = 0; i < 1000; i++) {
        float z = rand() % TRACKSEGMENTS;
        float f[3];
        trackVectorf(f, (rand() % (TRACKSEGMENTS*100))/100.f, (rand()%50)-25, (rand()%50)-25);
        cube( f[0],f[1],f[2], 5,1,5);
      }
  
      glCullFace(GL_FRONT);
      skyCube(-m[3*4+0],-m[3*4+1],-m[3*4+2],FARPLANE/sqrt(3.f),FARPLANE/sqrt(3.f),FARPLANE/sqrt(3.f));
      glCullFace(GL_BACK);
    }

    glRefresh();
  } while(!keyPressed());
  glDone();
  return 0;
}
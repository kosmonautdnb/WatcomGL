// Tests similar to the ones at nehe.gamedev.net in the internet
//----
#include "GL.H"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#define XRES 640 // 320
#define YRES 480 // 200

GLfloat xrot;
GLfloat yrot;
GLfloat zrot;
GLuint texture[1];

GLfloat LightAmbient[] = {0.5f, 0.5f, 0.5f, 1.0f};
GLfloat LightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
GLfloat LightPosition[] = {0.f, 0.f, 2.0f, 1.0f};

void NEHE2_YourFirstPolygon() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(-1.5f,0.0f,-6.0);
  glBegin(GL_TRIANGLES);
  glVertex3f(0.0f,1.0f,0.0f);
  glVertex3f(-1.0f,-1.0f,0.0f);
  glVertex3f(1.0f,-1.0f,0.0f);
  glEnd();
  glTranslatef(3.0f,0.0f,0.0f);
  glBegin(GL_QUADS);
  glVertex3f(-1.0f,1.0f,0.0f);
  glVertex3f(1.0f,1.0f,0.0f);
  glVertex3f(1.0f,-1.0f,0.0f);
  glVertex3f(-1.0f,-1.0f,0.0f);
  glEnd();
}

void NEHE3_AddingColor() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(-1.5f,0.0f,-6.0);
  glBegin(GL_TRIANGLES);
  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(-1.0f, -1.0f, 0.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(1.0f, -1.0f, 0.0f);
  glEnd();
  glTranslatef(3.0f,0.0f,0.0f);
  glColor3f(0.5f,0.5f,1.0f);
  glBegin(GL_QUADS);
  glVertex3f(-1.0f,1.0f,0.0f);
  glVertex3f(1.0f,1.0f,0.0f);
  glVertex3f(1.0f,-1.0f,0.0f);
  glVertex3f(-1.0f,-1.0f,0.0f);
  glEnd();
}

void NEHE4_Rotation() {
  static double rtri = 0.0;
  static double rquad = 0.0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(-1.5f,0.0f,-6.0);
  glRotatef(rtri, 0.0f, 1.0f, 0.0f);

  //    0
  //  1   2
  glBegin(GL_TRIANGLES);
  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(-1.0f, -1.0f, 0.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(1.0f, -1.0f, 0.0f);
  glEnd();

  glLoadIdentity();
  glTranslatef(1.5f,0.0f,-6.0f);
  glRotatef(rquad,1.0f,0.0f,0.0f);

  //  0  1
  //  3  2
  glColor3f(0.5f,0.5f,1.0f);
  glBegin(GL_QUADS);
  glVertex3f(-1.0f,1.0f,0.0f);
  glVertex3f(1.0f,1.0f,0.0f);
  glVertex3f(1.0f,-1.0f,0.0f);
  glVertex3f(-1.0f,-1.0f,0.0f);
  glEnd();

  rtri += 0.2f;
  rquad -=0.15f;
}

void NEHE5_3DShapes() {
  static double rtri = 0.0;
  static double rquad = 0.0;

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glTranslatef(-1.5f,0.0f,-6.0);
  glRotatef(rtri, 0.0f, 1.0f, 0.0f);

  //    0
  //  1   2
  glBegin(GL_TRIANGLES);

  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(-1.0f, -1.0f, 1.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);

  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(1.0f, -1.0f, 1.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(1.0f, -1.0f, -1.0f);

  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(1.0f, -1.0f, -1.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(-1.0f, -1.0f, -1.0f);

  glColor3f(1.0f,0.0f,0.0f);
  glVertex3f(0.0f, 1.0f, 0.0f);
  glColor3f(0.0f,0.0f,1.0f);
  glVertex3f(-1.0f, -1.0f, -1.0f);
  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(-1.0f, -1.0f, 1.0f);

  glEnd();

  glLoadIdentity();
  glTranslatef(1.5f,0.0f,-6.0f);
  glRotatef(rquad,1.0f,0.0f,0.0f);

  //  0  1
  //  3  2
  glBegin(GL_QUADS);

  glColor3f(0.0f,1.0f,0.0f);
  glVertex3f(1.0f,1.0f,-1.0f);
  glVertex3f(-1.0f,1.0f,-1.0f);
  glVertex3f(-1.0f,1.0f,1.0f);
  glVertex3f(1.0f,1.0f,1.0f);

  glColor3f(1.0f,0.5f,0.0f);
  glVertex3f(1.0f,-1.0f,1.0f);
  glVertex3f(-1.0f,-1.0f,1.0f);
  glVertex3f(-1.0f,-1.0f,-1.0f);
  glVertex3f(1.0f,-1.0f,-1.0f);

  glColor3f(1.0f,0.f,0.0f);
  glVertex3f(1.0f,1.0f,1.0f);
  glVertex3f(-1.0f,1.0f,1.0f);
  glVertex3f(-1.0f,-1.0f,1.0f);
  glVertex3f(1.0f,-1.0f,1.0f);

  glColor3f(1.0f,1.f,0.0f);
  glVertex3f(1.0f,-1.0f,-1.0f);
  glVertex3f(-1.0f,-1.0f,-1.0f);
  glVertex3f(-1.0f,1.0f,-1.0f);
  glVertex3f(1.0f,1.0f,-1.0f);

  glColor3f(0.0f,0.f,1.0f);
  glVertex3f(-1.0f,1.0f,1.0f);
  glVertex3f(-1.0f,1.0f,-1.0f);
  glVertex3f(-1.0f,-1.0f,-1.0f);
  glVertex3f(-1.0f,-1.0f,1.0f);

  glColor3f(1.0f,0.f,1.0f);
  glVertex3f(1.0f,1.0f,-1.0f);
  glVertex3f(1.0f,1.0f,1.0f);
  glVertex3f(1.0f,-1.0f,1.0f);
  glVertex3f(1.0f,-1.0f,-1.0f);

  glEnd();

  rtri += 0.2f;
  rquad -=0.15f;
}

void NEHE6_TextureMapping() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glTranslatef(0.f,0.f,-5.f);
  glRotatef(xrot,1.0f,0.0f,0.0f);
  glRotatef(yrot,0.0f,1.0f,0.0f);
  glRotatef(zrot,0.0f,0.0f,1.0f);
  glBindTexture(GL_TEXTURE_2D, texture[0]);

  glBegin(GL_QUADS);

  glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);
  glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);
  glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,1.0f);
  glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,1.0f);

  glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
  glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);
  glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);
  glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,-1.0f);

  glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);
  glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,1.0f,1.0f);
  glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,1.0f,1.0f);
  glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);

  glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
  glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,-1.0f,-1.0f);
  glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);
  glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);

  glTexCoord2f(1.0f,0.0f); glVertex3f(1.0f,-1.0f,-1.0f);
  glTexCoord2f(1.0f,1.0f); glVertex3f(1.0f,1.0f,-1.0f);
  glTexCoord2f(0.0f,1.0f); glVertex3f(1.0f,1.0f,1.0f);
  glTexCoord2f(0.0f,0.0f); glVertex3f(1.0f,-1.0f,1.0f);

  glTexCoord2f(0.0f,0.0f); glVertex3f(-1.0f,-1.0f,-1.0f);
  glTexCoord2f(1.0f,0.0f); glVertex3f(-1.0f,-1.0f,1.0f);
  glTexCoord2f(1.0f,1.0f); glVertex3f(-1.0f,1.0f,1.0f);
  glTexCoord2f(0.0f,1.0f); glVertex3f(-1.0f,1.0f,-1.0f);

  glEnd();

  xrot += 0.3f;
  yrot += 0.2f;
  zrot += 0.4f;
}

void NEHE7_Lighting() {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  glTranslatef(0.f,0.f,-5.f);
  glRotatef(xrot,1.0f,0.0f,0.0f);
  glRotatef(yrot,0.0f,1.0f,0.0f);
  glRotatef(zrot,0.0f,0.0f,1.0f);
  glBindTexture(GL_TEXTURE_2D, texture[0]);

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
}

void NEHE8_Blending() {

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
}

int main() {
  
  //glWatcomPrecisionTimer(GL_TRUE);
  glConfigureAntiAlias();

  const bool pureVGA = false;
  if (pureVGA) {
    glVGA();
  } else {
    if (!glVesa(XRES,YRES,32)) {
      printf("No Vesa %dx%dx32 found.\n", XRES, YRES);
      if (!glVGA())
      {
        printf("No VGA device found.\n");
        exit(0);
      }
    }
  }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(45.0f,(GLfloat)16.0/9.0,0.1f,100.f);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);  

  glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
  glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);

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

  while(true)
  {    
      int lesson = 8;
      switch(lesson) {
        case 2: NEHE2_YourFirstPolygon(); break;
        case 3: NEHE3_AddingColor(); break;
        case 4: NEHE4_Rotation(); break;
        case 5: NEHE5_3DShapes(); break;
        case 6: NEHE6_TextureMapping(); break;
        case 7: NEHE7_Lighting(); break;
        case 8: NEHE8_Blending(); break;
      }

      glRefresh();
      if (glNextKey()!=0)
        break;
  }
  glDone();
  return 0;
}

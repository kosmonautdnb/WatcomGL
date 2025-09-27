// --
// WatcomGL
// OpenGL like software rendering for WatcomC++/Dos.
// --
// License: MIT License
// Github: https://github.com/kosmonautdnb/WatcomGL
// by Stefan Mader in 2025
// --
// Issues:
// - In ultra rare cases polygons may have seams. (DJGPP no -Ofast,-ffast-math), maybe add 0.001 to the coordinates
// - Better take public domain PModeW for 256MB of memory.
// --
#define WATCOMGLAPI
#include "gl.h"
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PI
#  define PI 3.14159265358979323846
#endif

//#define __GLDISABLEDOSFUNCTIONS__ 1 // you may use only glDirect(unsigned int *frameBuffer, float *depthBuffer, unsigned int width, unsigned int height);

#define FLOOR(__v__) (__v__) // not needed since conversion to int truncates
#define FABSF fabs
#define DIVE255 /= 255 // >>= 8
#define DIVQ255 / 255 // >> 8

#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]
#define FLIPRB(__v__) (((__v__)&0xff00ff00)|(((__v__)>>16) & 0x000000ff)|(((__v__)<<16) & 0x00ff0000))
#define __MATRIX_STACK_SIZE__ 16
#define __ATTRIB_STACK_SIZE__ 4

#ifdef __WATCOMC__
#  define INLINE __inline
#  define __FASTTEXTURING__ 1
#endif

#ifdef __DJGPP__
#  define INLINE inline
#  define __FASTTEXTURING__ 1
#endif

#ifndef INLINE
#define INLINE
#  define __FASTTEXTURING__ 1
#endif

#define SEAMPREVENT -0.0000001 // landing directly on 0.5 screen (real screen) pixels caused seams on DJGPP
double glPixelCenterX = -0.5+SEAMPREVENT;
double glPixelCenterY = -0.5+SEAMPREVENT;
double glTexelCenterX = -0.5;
double glTexelCenterY = -0.5;
unsigned int *glFrameBufferDedicated = NULL; // linear framebuffer
int glFrameBufferWidth = 0;
int glFrameBufferHeight = 0;
int glFrameBufferBytesPerPixel = 4;
unsigned int *glFrameBuffer = NULL;
float *glDepthBuffer = NULL;
bool useNearPointers = true;
bool glUseHalveVector = false; // OSMesa seems to use the halvevector, instead of the "real" phong described in the docs of OpenGL
bool glFastTexturing = false; // only with #define __FASTTEXTURING__
int glFastTextureSpanWidth = 16;
int glFrameBufferWidth0 = 0;
int glFrameBufferHeight0 = 0;
unsigned int *glFrameBuffer0 = NULL;
float *glDepthBuffer0 = NULL;
double identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};
bool currentBackFacing = false;
_GLContext glContext;
_GLContext glAttribStack[__ATTRIB_STACK_SIZE__];
int glAttribStackPos = 0;
double glMatrixStack[__MATRIX_STACK_SIZE__][4*4];
int glMatrixStackPos = 0;
glVertex glVertices[4];
int glCurrentVertexElement = 0;
double additionalPointSpriteXStretch = 1.0;
TriangleDrawer glDrawTriangle = glDrawTrianglePrecise;

glTexture glTextures[GLMAXTEXTURES]={{0,0,0,NULL,0,0,GL_LINEAR,1000,1000,GL_NEAREST,0,GL_REPEAT,GL_REPEAT,GL_REPEAT,1.f,0.f,1.f,1.f,GL_MODULATE}};


INLINE float glClampf(float a, float n, float x) {
  return a < n ? n : (a > x ? x : a);
}

INLINE int glClampi(int a, int n, int x) {
  return a < n ? n : (a > x ? x : a);
}

INLINE int glMini(int a, int n) {
  return a < n ? a : n;
}

INLINE int glMaxi(int a, int x) {
  return a > x ? a : x;
}

void initTexture(glTexture *t) {
  t->name=0;
  t->width=0;
  t->height=0;
  t->data=NULL;
  t->baseLevel=0;
  t->lodBias=0;
  t->magFilter=GL_LINEAR;
  t->maxLevel=1000;
  t->maxLod=1000;
  t->minFilter=GL_NEAREST;
  t->minLod=0;
  t->wrapS=GL_REPEAT;
  t->wrapT=GL_REPEAT;
  t->wrapR=GL_REPEAT;
  t->borderColorRed=1.f;
  t->borderColorGreen=0.f;
  t->borderColorBlue=1.f;
  t->borderColorAlpha=1.f;
  t->texEnvMode=GL_MODULATE;
}

void _GLContext_init() {
  int i;
  glContext.error = 0;
  glContext.viewportX0 = 0;
  glContext.viewportY0 = 0;
  glContext.viewportX1 = 0;
  glContext.viewportY1 = 0;
  glContext.clearRed = 0;
  glContext.clearGreen = 0;
  glContext.clearBlue = 0;
  glContext.clearAlpha = 0;
  glContext.clearDepth = 1.f;
  glContext.clearStencil = 0;
  glContext.activeTexture = 0;
  glContext.alphaFunc = 0;
  glContext.alphaFuncRef = 0;
  glContext.blendFuncSFactor = GL_ONE;
  glContext.blendFuncDFactor = GL_ZERO;
  glContext.cullFaceMode = GL_BACK;
  glContext.depthFunc = GL_LESS;
  glContext.depthMask = true;
  glContext.depthRangeZNear = 0;
  glContext.depthRangeZFar = 1;
  glContext.pointSize = 0;
  glContext.polygonOffsetFactor = 0;
  glContext.polygonOffsetUnits = 0;
  memcpy(glContext.matrixForMode[0],identityMatrix,4*4*sizeof(double));
  memcpy(glContext.matrixForMode[1],identityMatrix,4*4*sizeof(double));
  memcpy(glContext.inverseMatrixForMode[0],identityMatrix,4*4*sizeof(double));
  memcpy(glContext.inverseMatrixForMode[1],identityMatrix,4*4*sizeof(double));
  memcpy(glContext.matrix,identityMatrix,4*4*sizeof(double));
  glContext.matrixModeNr = 0;
  glContext.lineStippleFactor = 0;
  glContext.lineStipplePattern = 0;
  glContext.lineWidth = 1;
  glContext.scissorX0 = 0;
  glContext.scissorY0 = 0;
  glContext.scissorX1 = 0;
  glContext.scissorY1 = 0;
  glContext.shadeMode = 0;
  glContext.stencilFunc = 0;
  glContext.stencilFuncRef = 0;
  glContext.stencilFuncMask = 0;
  glContext.stencilMask = 0;
  glContext.stencilOpFail = 0;
  glContext.stencilOpZFail = 0;
  glContext.stencilOpZPass = 0;
  glContext.colorRed = 1.f;
  glContext.colorGreen = 1.f;
  glContext.colorBlue = 1.f;
  glContext.colorAlpha = 1.f;
  glContext.normalX = 0.0;
  glContext.normalY = 0.0;
  glContext.normalZ = 0.0;
  glContext.vertexX = 1.0;
  glContext.vertexY = 1.0;
  glContext.vertexZ = 1.0;
  glContext.vertexW = 1.0;
  glContext.textureX = 0.0;
  glContext.textureY = 0.0;
  glContext.textureZ = 0.0;
  glContext.textureW = 0.0;
  glContext.beginMode = 0;

  glContext.colorMaterialFace = GL_FRONT;
  for (i = 0; i < 2; i++) {
    glContext.colorMaterial[i] = GL_AMBIENT_AND_DIFFUSE;
    // ambient
    glContext.materialRed[i][0] = 0;
    glContext.materialGreen[i][0] = 0;
    glContext.materialBlue[i][0] = 0;
    glContext.materialAlpha[i][0] = 1;
    // diffuse
    glContext.materialRed[i][1] = 1;
    glContext.materialGreen[i][1] = 1;
    glContext.materialBlue[i][1] = 1;
    glContext.materialAlpha[i][1] = 1;
    // specular
    glContext.materialRed[i][2] = 0;
    glContext.materialGreen[i][2] = 0;
    glContext.materialBlue[i][2] = 0;
    glContext.materialAlpha[i][2] = 1;
    // emission
    glContext.materialRed[i][3] = 0;
    glContext.materialGreen[i][3] = 0;
    glContext.materialBlue[i][3] = 0;
    glContext.materialAlpha[i][3] = 1;
    // shininess
    glContext.materialRed[i][4] = 1;
    glContext.materialGreen[i][4] = 1;
    glContext.materialBlue[i][4] = 1;
    glContext.materialAlpha[i][4] = 1;
    glContext.wireframe[i] = false;
  }

  for (i = 0;  i < GLMAXLIGHTS; i++) {
    // ambient
    glContext.lightRed[0][i] = 0;
    glContext.lightGreen[0][i] = 0;
    glContext.lightBlue[0][i] = 0;
    glContext.lightAlpha[0][i] = 0;
    // diffuse
    glContext.lightRed[1][i] = 1;
    glContext.lightGreen[1][i] = 1;
    glContext.lightBlue[1][i] = 1;
    glContext.lightAlpha[1][i] = 1;
    // specular
    glContext.lightRed[2][i] = 0;
    glContext.lightGreen[2][i] = 0;
    glContext.lightBlue[2][i] = 0;
    glContext.lightAlpha[2][i] = 0;
    // position
    glContext.lightRed[3][i] = 0;
    glContext.lightGreen[3][i] = 0;
    glContext.lightBlue[3][i] = 1;
    glContext.lightAlpha[3][i] = 0;
    // spot direction
    glContext.lightRed[4][i] = 0;
    glContext.lightGreen[4][i] = 0;
    glContext.lightBlue[4][i] = 1;
    glContext.lightAlpha[4][i] = 0;
    // attenuation
    glContext.constantAttenuation[i] = 1;
    glContext.linearAttenuation[i] = 0;
    glContext.quadraticAttenuation[i] = 0;
    glContext.spotExponent[i] = 0;
    glContext.spotCutOff[i] = (float)cos(180.0*PI/180.0);
  }
  for (i = 0; i < GLMAXTEXTUREUNITS; i++) {
    glContext.boundTextures[i] = 0;
  }
  glContext.blendColorRed = 0;
  glContext.blendColorGreen = 0;
  glContext.blendColorBlue = 0;
  glContext.blendColorAlpha = 1;
  glContext.blendEquation = GL_FUNC_ADD;
  glContext.frontFace = GL_CCW;
  glContext.maskRed = true;
  glContext.maskGreen = true;
  glContext.maskBlue = true;
  glContext.maskAlpha = true;
  glContext.forceNoCull = 0;
  glContext.zoomX = 1.0;
  glContext.zoomY = 1.0;
  glContext.pushAttribBitsHere = 0;
  glContext.explicitAlpha = 0.f;
  glContext.useExplicitAlpha = false;
  glContext.separateSpecular = false;
  glContext.texGenS = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  glContext.texGenT = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  glContext.needNewInverseModelView = false;
  glContext.twoSidedLighting = false;

  glContext.fogStart = 0.0;
  glContext.fogEnd = 1.0;
  glContext.fogColor[0] = 0;
  glContext.fogColor[1] = 0;
  glContext.fogColor[2] = 0;
  glContext.fogColor[3] = 0;
  glContext.fogDensity = 1.0;
  glContext.fogMode = GL_EXP;

  for (i = 0; i < GLMAXTEXTURES; i++) {
    glTexture *t=&glTextures[i];
    initTexture(t);
  }
}

void _GLContext_enable(GLenum prop, bool enable) {
  glContext.enabledCaps[prop&255]=enable;
}

GLboolean _GLContext_isEnabled(GLenum prop) {
  return glContext.enabledCaps[prop&255];
}

INLINE GLboolean glIsEnabled2(_GLContext *context, GLenum prop) {
  return context->enabledCaps[prop&255];
}

#ifdef __cplusplus
// in C++ the context is already constructed at program start
}
  class GLConstructor {
  public:
    GLConstructor() {_GLContext_init();}
  };
  GLConstructor glConstruct;
extern "C" {
void construtGL() {;}
#endif
#ifndef __cplusplus
// in C glVesa,glVGA,glDirect firstly construct the context
void construtGL() {
  _GLContext_init();
}
#endif

void glSetTriangleDrawer(TriangleDrawer drawer) {
  glDrawTriangle = drawer;
}

void glDrawQuad(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3);

GLuint glNewTexture() {
  for (int i = 1; i < GLMAXTEXTURES; i++) {
    if (glTextures[i].name == 0x00) {
      initTexture(&glTextures[i]);
      glTextures[i].name = i;
      glTextures[i].width = 0;
      glTextures[i].height = 0;
      glTextures[i].data = NULL;
      return i;
    }
  }
  return 0;
}

void glDeleteTexture(GLuint i) {
  if (i > 0 && glTextures[i].name != 0) {
    glTextures[i].name = 0;
    if (glTextures[i].data != NULL) {
      free(glTextures[i].data);
      glTextures[i].data = NULL;
    }
  }
}

double *glGetInverseModelView(_GLContext *context) {
  if (context->needNewInverseModelView) {
    context->needNewInverseModelView = false;
    memcpy(glContext.inverseMatrixForMode[GL_MODELVIEW & 1],glContext.matrixForMode[GL_MODELVIEW & 1],4*4*sizeof(double));
    gluInvertMatrix(glContext.inverseMatrixForMode[GL_MODELVIEW & 1],glContext.inverseMatrixForMode[GL_MODELVIEW & 1]);
  }
  return glContext.inverseMatrixForMode[GL_MODELVIEW & 1];
}

void combineIntoWindow(GLint *x0, GLint *y0, GLint *x1, GLint *y1,GLint nx0, GLint ny0, GLint nx1, GLint ny1) {
  *x0 = nx0<*x0?*x0:nx0;
  *y0 = ny0<*y0?*y0:ny0;
  *x1 = nx1>*x1?*x1:nx1;
  *y1 = ny1>*y1?*y1:ny1;
}

void glMatMulf(const GLfloat *ma, const GLfloat *mb, GLfloat *r) {
  GLfloat result[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float a = 0.f;
      for (int k = 0; k < 4; k++) {
        a += ma[i+k*4] * mb[j*4+k];
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(float));
}

void glMatMulf2(const GLdouble *ma, const GLfloat *mb, GLdouble *r) { // double output
  GLdouble result[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double a = 0.0;
      for (int k = 0; k < 4; k++) {
        a += ((double)ma[i+k*4] * mb[j*4+k]);
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(double));
}

void glMatMul(const GLdouble *ma, const GLdouble *mb, GLdouble *r) {
  GLdouble result[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      double a = 0.0;
      for (int k = 0; k < 4; k++) {
        a += ma[i+k*4] * mb[j*4+k];
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(double));
}

void glUpdateMatrix() {
  glMatMul(glContext.matrixForMode[GL_PROJECTION & 1],glContext.matrixForMode[GL_MODELVIEW & 1],glContext.matrix);
  glContext.needNewInverseModelView = true;
}

INLINE int glTransformVertex(_GLContext *context, glVertex *v, bool clip) {
  const double *cm = context->matrix;
  const double vx = v->vertexX;
  const double vy = v->vertexY;
  const double vz = v->vertexZ;
  const double vw = v->vertexW;
  const double x = vx * cm[0*4+0] + vy * cm[1*4+0] + vz * cm[2*4+0] + vw * cm[3*4+0];
  const double y = vx * cm[0*4+1] + vy * cm[1*4+1] + vz * cm[2*4+1] + vw * cm[3*4+1];
  const double z = vx * cm[0*4+2] + vy * cm[1*4+2] + vz * cm[2*4+2] + vw * cm[3*4+2];
  const double w = vx * cm[0*4+3] + vy * cm[1*4+3] + vz * cm[2*4+3] + vw * cm[3*4+3];

  if (clip) {
    if (z < -w) {
      return 1;
    }
  }
  if (w == 0) {
    return 1; // should never happen
  }
     
  v->sx = x/w;
  v->sy = y/w;
  v->sz = z/w;
  v->sw = w; // not 1

  v->sx *= (context->viewportX1-context->viewportX0)*0.5*context->zoomX;
  v->sy *= (context->viewportY1-context->viewportY0)*-0.5*context->zoomY;
  v->sx += (context->viewportX0+context->viewportX1)*0.5+glPixelCenterX;
  v->sy += (context->viewportY0+context->viewportY1)*0.5+glPixelCenterY;
  return 0;
}

void fixSphereMapUV(_GLContext *context, float *tx0, float *ty0, float *tx1, float *ty1) {
  if (glIsEnabled2(context,GL_TEXTURE_GEN_S)&&(context->texGenS == GL_SPHERE_MAP_DUAL_PARABOLOID||context->texGenS == GL_SPHERE_MAP_ATAN2||context->texGenS == GL_SPHERE_MAP)) {
    if (*tx1-*tx0>0.5) *tx1-=1.0;
    if (*tx1-*tx0<-0.5) *tx1+=1.0;
  }
  if (glIsEnabled2(context,GL_TEXTURE_GEN_T)&&(context->texGenT == GL_SPHERE_MAP_DUAL_PARABOLOID||context->texGenT == GL_SPHERE_MAP_ATAN2||context->texGenT == GL_SPHERE_MAP)) {
    if (*ty1-*ty0>0.5) *ty1-=1.0;
    if (*ty1-*ty0<-0.5) *ty1+=1.0;
  }
}

void glTexGen_(_GLContext *context, glVertex *v, double x, double y, double z, double normalX, double normalY, double normalZ) {
  // glTexGen is done per vertex what most likely is wrong
  double vx = x;
  double vy = y;
  double vz = z;
  const double l = sqrt(vx*vx+vy*vy+vz*vz);
  if (l > 0.0) {                               
    vx /= l;
    vy /= l;
    vz /= l;
  }
  const double l2 = sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
  if (l2 > 0.0) {
    normalX /= l2;
    normalY /= l2;
    normalZ /= l2;
  }

  const double dot2NI = 2.0*(vx*normalX+vy*normalY+vz*normalZ);
  double reflectionX = vx - dot2NI * normalX;
  double reflectionY = vy - dot2NI * normalY;
  double reflectionZ = vz - dot2NI * normalZ;

  switch(context->texGenS) {
  case GL_SPHERE_MAP: {v->textureX = reflectionX*0.5+0.5;} break;
  case GL_SPHERE_MAP_ATAN2: {v->textureX = atan2(reflectionZ,reflectionX)/(2.0*PI)+0.5;} break;
  case GL_SPHERE_MAP_DUAL_PARABOLOID: {v->textureX = reflectionX/(fabs(reflectionZ)+1.0)*0.5+0.5;} break;
  }
  switch(context->texGenT) {
  case GL_SPHERE_MAP: {v->textureY = -reflectionY*0.5+0.5;} break;
  case GL_SPHERE_MAP_ATAN2: {v->textureY = -acos(-reflectionY)/PI;} break;
  case GL_SPHERE_MAP_DUAL_PARABOLOID: {v->textureY = -reflectionY/(fabs(reflectionZ)+1.0)*0.5+0.5;} break;
  }
}

void glLightVertex(_GLContext *context, glVertex *v) {
  if (glIsEnabled2(context,GL_LIGHTING)) {
    const double *matrix = context->matrixForMode[GL_MODELVIEW & 1];
    double x = v->vertexX * matrix[0*4+0] + v->vertexY * matrix[1*4+0] + v->vertexZ * matrix[2*4+0] + v->vertexW * matrix[3*4+0];
    double y = v->vertexX * matrix[0*4+1] + v->vertexY * matrix[1*4+1] + v->vertexZ * matrix[2*4+1] + v->vertexW * matrix[3*4+1];
    double z = v->vertexX * matrix[0*4+2] + v->vertexY * matrix[1*4+2] + v->vertexZ * matrix[2*4+2] + v->vertexW * matrix[3*4+2];
    double w = v->vertexX * matrix[0*4+3] + v->vertexY * matrix[1*4+3] + v->vertexZ * matrix[2*4+3] + v->vertexW * matrix[3*4+3];
    if (w != 0) { // should never happen
      x /= w;
      y /= w;
      z /= w;
    }
    // rotate normal (inversetranspose would be better, or?)
    const double vnx = v->vertexX+v->normalX;
    const double vny = v->vertexY+v->normalY;
    const double vnz = v->vertexZ+v->normalZ;
    const double vnw = v->vertexW;
    double nx = vnx * matrix[0*4+0] + vny * matrix[1*4+0] + vnz * matrix[2*4+0] + vnw * matrix[3*4+0];
    double ny = vnx * matrix[0*4+1] + vny * matrix[1*4+1] + vnz * matrix[2*4+1] + vnw * matrix[3*4+1];
    double nz = vnx * matrix[0*4+2] + vny * matrix[1*4+2] + vnz * matrix[2*4+2] + vnw * matrix[3*4+2];
    const double nw = vnx * matrix[0*4+3] + vny * matrix[1*4+3] + vnz * matrix[2*4+3] + vnw * matrix[3*4+3];
    if (nw != 0) { // should never happen
      nx /= nw;
      ny /= nw;
      nz /= nw;
    }
    nx -= x;
    ny -= y;
    nz -= z;

    float ambientRed = 0;
    float ambientGreen = 0;
    float ambientBlue = 0;
    float diffuseRed = 0;
    float diffuseGreen = 0;
    float diffuseBlue = 0;
    float diffuseAlpha = 0;
    float specularRed = 0;
    float specularGreen = 0;
    float specularBlue = 0;

    int face = glContext.twoSidedLighting ? (currentBackFacing ? 1:0) : 0;
    float materialAmbientRed = context->materialRed[face][0];
    float materialAmbientGreen = context->materialGreen[face][0];
    float materialAmbientBlue = context->materialBlue[face][0];
    float materialDiffuseRed = context->materialRed[face][1];
    float materialDiffuseGreen = context->materialGreen[face][1];
    float materialDiffuseBlue = context->materialBlue[face][1];
    float materialDiffuseAlpha = context->materialAlpha[face][1];
    float materialSpecularRed = context->materialRed[face][2];
    float materialSpecularGreen = context->materialGreen[face][2];
    float materialSpecularBlue = context->materialBlue[face][2];
    float materialEmissionRed = context->materialRed[face][3];
    float materialEmissionGreen = context->materialGreen[face][3];
    float materialEmissionBlue = context->materialBlue[face][3];
    float materialShininessRed = context->materialRed[face][4];
    float materialShininessGreen = context->materialGreen[face][4];
    float materialShininessBlue = context->materialBlue[face][4];


    if (glIsEnabled2(context,GL_COLOR_MATERIAL)) {
      switch(glContext.colorMaterial[face]) {
        case GL_AMBIENT: {
          materialAmbientRed = v->colorRed;
          materialAmbientGreen = v->colorGreen;
          materialAmbientBlue = v->colorBlue;
        } break;
        case GL_DIFFUSE: {
          materialDiffuseRed = v->colorRed;
          materialDiffuseGreen = v->colorGreen;
          materialDiffuseBlue = v->colorBlue;
          materialDiffuseAlpha = v->colorAlpha;
        } break;
        case GL_SPECULAR: {
          materialSpecularRed = v->colorRed;
          materialSpecularGreen = v->colorGreen;
          materialSpecularBlue = v->colorBlue;
        } break;
        case GL_EMISSION: {
          materialEmissionRed = v->colorRed;
          materialEmissionGreen = v->colorGreen;
          materialEmissionBlue = v->colorBlue;
        } break;
        case GL_SHININESS: {
          materialShininessRed = v->colorRed;
          materialShininessGreen = v->colorGreen;
          materialShininessBlue = v->colorBlue;
        } break;
        case GL_AMBIENT_AND_DIFFUSE: {
          materialAmbientRed = v->colorRed;
          materialAmbientGreen = v->colorGreen;
          materialAmbientBlue = v->colorBlue;
          materialDiffuseRed = v->colorRed;
          materialDiffuseGreen = v->colorGreen;
          materialDiffuseBlue = v->colorBlue;
          materialDiffuseAlpha = v->colorAlpha;
        } break;
      }
    }

    double normalX = nx;
    double normalY = ny;
    double normalZ = nz;

    float l = (float)sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
    if (l>0.f) {
      normalX/=l;
      normalY/=l;
      normalZ/=l;
    }

    for (int i = 0; i < GLMAXLIGHTS; i++) {
      if (glIsEnabled2(context,GL_LIGHT0+i)) {
        const float lightAmbientRed = (float)context->lightRed[0][i];
        const float lightAmbientGreen = (float)context->lightGreen[0][i];
        const float lightAmbientBlue = (float)context->lightBlue[0][i];
        const float lightDiffuseRed = (float)context->lightRed[1][i];
        const float lightDiffuseGreen = (float)context->lightGreen[1][i];
        const float lightDiffuseBlue = (float)context->lightBlue[1][i];
        const float lightDiffuseAlpha = (float)context->lightAlpha[1][i];
        const float lightSpecularRed = (float)context->lightRed[2][i];
        const float lightSpecularGreen = (float)context->lightGreen[2][i];
        const float lightSpecularBlue = (float)context->lightBlue[2][i];
        const double lightPositionX = context->lightRed[3][i];
        const double lightPositionY = context->lightGreen[3][i];
        const double lightPositionZ =  context->lightBlue[3][i];
  
        const double lightPositionW =  context->lightAlpha[3][i];
        double lVecX = lightPositionX;
        double lVecY = lightPositionY;
        double lVecZ = lightPositionZ;
        double attenuation = context->constantAttenuation[i]; // directionals have no distance based attenuation
        if (fabs(lightPositionW)>0.0) {
          lVecX-=x;
          lVecY-=y;
          lVecZ-=z;
          const double d = sqrt(lVecX*lVecX+lVecY*lVecY+lVecZ*lVecZ);
          attenuation = context->constantAttenuation[i]+context->linearAttenuation[i]*d+context->quadraticAttenuation[i]*d*d;
        }
        if (attenuation > 0.0) 
          attenuation = 1.0/attenuation; 
        else 
          attenuation = 0.0;
        double l = sqrt(lVecX*lVecX+lVecY*lVecY+lVecZ*lVecZ);
        if (l>0.0) {lVecX/=l;  lVecY/=l; lVecZ/=l;}
        double diffuse = lVecX * normalX + lVecY * normalY + lVecZ * normalZ;
        if (context->twoSidedLighting) {
          diffuse = fabs(diffuse);
        }
        if (diffuse<0) diffuse = 0;
        double viewX = x;
        double viewY = y;
        double viewZ = z;
        l = sqrt(viewX*viewX+viewY*viewY+viewZ*viewZ);
        if (l>0.f) {viewX/=l;viewY/=l;viewZ/=l;}
        double specular;
        const bool halveVector = glUseHalveVector;
        if (halveVector) {
          double reflectionX = -viewX + lVecX; // halveVector
          double reflectionY = -viewY + lVecY;
          double reflectionZ = -viewZ + lVecZ;
          l = sqrt(reflectionX*reflectionX+reflectionY*reflectionY+reflectionZ*reflectionZ);
          if (l>0.f) {reflectionX/=l; reflectionY /= l; reflectionZ /= l;}
          specular = (normalX * reflectionX + normalY * reflectionY + normalZ * reflectionZ);
        } else {
          double dot2NI = 2.0*(-lVecX*normalX+-lVecY*normalY+-lVecZ*normalZ);
          double reflectionX = -lVecX - dot2NI * normalX;
          double reflectionY = -lVecY - dot2NI * normalY;
          double reflectionZ = -lVecZ - dot2NI * normalZ;
          l = sqrt(reflectionX*reflectionX+reflectionY*reflectionY+reflectionZ*reflectionZ);
          if (l>0.f) {reflectionX/=l; reflectionY /= l; reflectionZ /= l;}
          specular = -viewX * reflectionX + -viewY * reflectionY + -viewZ * reflectionZ;
        }
        if (specular<0.0) specular = 0.0;

        // spot calculations
        const double spotCutOff = context->spotCutOff[i];
        if (spotCutOff>-0.9999) { // is it a spot light?
          // light position needs w other than 0, denoting no directional
          double spotDirectionX = context->lightRed[4][i];
          double spotDirectionY = context->lightGreen[4][i];
          double spotDirectionZ =  context->lightBlue[4][i];
          double spotAngleDot = lVecX*spotDirectionX+lVecY*spotDirectionY+lVecZ*spotDirectionZ;
          if (spotAngleDot>spotCutOff) { // spotCutOff goes from 1(0degrees) to -1(180degrees)
            const double spotExponent = context->spotExponent[i];
            const double d = pow(spotAngleDot < 0 ? 0 : spotAngleDot,spotExponent);
            diffuse *= d;
            specular *= d;
          } else {
            // cutoff
            diffuse = 0;
            specular = 0;
          }
        }

        ambientRed += materialAmbientRed*lightAmbientRed;
        ambientGreen += materialAmbientGreen*lightAmbientGreen;
        ambientBlue += materialAmbientBlue*lightAmbientBlue;
        diffuse *= attenuation;
        diffuseRed += (float)(materialDiffuseRed*lightDiffuseRed*diffuse);
        diffuseGreen += (float)(materialDiffuseGreen*lightDiffuseGreen*diffuse);
        diffuseBlue += (float)(materialDiffuseBlue*lightDiffuseBlue*diffuse);
        double shininess = materialShininessRed;
        if (shininess < 0.001) shininess = 0.001;
        specular = pow(specular,shininess) * attenuation;
        specularRed += (float)(materialSpecularRed*lightSpecularRed*specular);
        specularGreen += (float)(materialSpecularGreen*lightSpecularGreen*specular);
        specularBlue += (float)(materialSpecularBlue*lightSpecularBlue*specular);
      }
    }
    if (context->separateSpecular) {
      v->additionalSpecularColorRed = specularRed;
      v->additionalSpecularColorGreen = specularGreen;
      v->additionalSpecularColorBlue = specularBlue;
      specularRed = 0;
      specularGreen = 0;
      specularBlue = 0;
    }
    v->colorRed = materialEmissionRed + ambientRed + diffuseRed + specularRed;
    v->colorGreen = materialEmissionGreen + ambientGreen + diffuseGreen + specularGreen;
    v->colorBlue = materialEmissionBlue + ambientBlue + diffuseBlue + specularBlue;
    v->colorAlpha = materialDiffuseAlpha;
  }
  if (glIsEnabled2(context,GL_TEXTURE_GEN_S)||glIsEnabled2(context,GL_TEXTURE_GEN_T)) {
    const double *matrix2 = glGetInverseModelView(context);
    glTexGen_(context,v,v->vertexX-matrix2[3*4+0],v->vertexY-matrix2[3*4+1],v->vertexZ-matrix2[3*4+2],v->normalX,v->normalY,v->normalZ);
  }
}

INLINE int glClipVertex(_GLContext *context, glVertex *v) {
  int clipFlags = (v->sx < context->viewportX0) ? 1 : 0;
  clipFlags |= (v->sy < context->viewportY0) ? 2 : 0;
  clipFlags |= (v->sx > context->viewportX1) ? 4 : 0;
  clipFlags |= (v->sy > context->viewportY1) ? 8 : 0;
  return clipFlags;
}

void interpolateVertex(glVertex *dest, glVertex *v0, glVertex *v1, double f) {
  dest->colorRed = v0->colorRed + (float)(f * (v1->colorRed-v0->colorRed));
  dest->colorGreen = v0->colorGreen + (float)(f * (v1->colorGreen-v0->colorGreen));
  dest->colorBlue = v0->colorBlue + (float)(f * (v1->colorBlue-v0->colorBlue));
  dest->colorAlpha = v0->colorAlpha + (float)(f * (v1->colorAlpha-v0->colorAlpha));

  if (glContext.separateSpecular) {
    dest->additionalSpecularColorRed = v0->additionalSpecularColorRed + (float)(f * (v1->additionalSpecularColorRed-v0->additionalSpecularColorRed));
    dest->additionalSpecularColorGreen = v0->additionalSpecularColorGreen + (float)(f * (v1->additionalSpecularColorGreen-v0->additionalSpecularColorGreen));
    dest->additionalSpecularColorBlue = v0->additionalSpecularColorBlue + (float)(f * (v1->additionalSpecularColorBlue-v0->additionalSpecularColorBlue));
   }

  dest->normalX = v0->normalX + f * (v1->normalX-v0->normalX);
  dest->normalY = v0->normalY + f * (v1->normalY-v0->normalY);
  dest->normalZ = v0->normalZ + f * (v1->normalZ-v0->normalZ);

  dest->vertexX = v0->vertexX + f * (v1->vertexX-v0->vertexX);
  dest->vertexY = v0->vertexY + f * (v1->vertexY-v0->vertexY);
  dest->vertexZ = v0->vertexZ + f * (v1->vertexZ-v0->vertexZ);
  dest->vertexW = v0->vertexW + f * (v1->vertexW-v0->vertexW);

  dest->textureX = v0->textureX + f * (v1->textureX-v0->textureX);
  dest->textureY = v0->textureY + f * (v1->textureY-v0->textureY);
  dest->textureZ = v0->textureZ + f * (v1->textureZ-v0->textureZ);
  dest->textureW = v0->textureW + f * (v1->textureW-v0->textureW);
}

void drawClippedNgon(_GLContext *context, glVertex *vertices[], int vertexCount) {
  double *matrix = context->matrix;
  glVertex poly[8];
  int count = 0;
  int clipFlags = 1|2|4|8;
  for(int i = 0; i < vertexCount; i++) {
    glVertex *v0 = vertices[i];
    glVertex *v1 = vertices[(i+1) % vertexCount];
    double z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
    double w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
    double z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
    double w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
    bool currentInside = z0 >= -w0;
    bool nextInside = z1 >= -w1;
    if (currentInside&&nextInside) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],false);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else
    if(currentInside && (!nextInside)) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],false);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
      double f = (w0+z0) / ((z0-z1)+(w0-w1));
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],false);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else 
    if((!currentInside) && nextInside) {
      double f = (w0+z0) / ((z0-z1)+(w0-w1));
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],false);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    }
  }
  if (clipFlags == 0) for(int j = 1; j < count-1; j++) glDrawTriangle(context, &poly[0],&poly[j],&poly[j+1]);
}

void drawClippedQuad(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2, glVertex *v3) {
  glVertex *vertices[4];
  vertices[0] = v0;
  vertices[1] = v1;
  vertices[2] = v2;
  vertices[3] = v3;
  drawClippedNgon(context, vertices, 4);
}

void drawClippedTriangle(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2) {
  glVertex *vertices[3];
  vertices[0] = v0;
  vertices[1] = v1;
  vertices[2] = v2;
  drawClippedNgon(context, vertices, 3);
}

void drawClippedLine(_GLContext *context, glVertex *v0a, glVertex *v1a)  {
  glVertex vertices[4];
  vertices[0] = *v0a;
  vertices[1] = *v1a;

  double *matrix = context->matrix;
  glVertex *v0 = &vertices[0];
  glVertex *v1 = &vertices[1];
  double z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
  double w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
  double z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
  double w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
  bool currentInside = z0 >= -w0;
  bool nextInside = z1 >= -w1;
  if ((!currentInside)&&(!nextInside))
    return;
  if ((!currentInside)||(!nextInside)) {
    double f = (w0+z0) / ((z0-z1)+(w0-w1));
    if (!currentInside)
      interpolateVertex(&vertices[0],v0,v1,f);
    else
      interpolateVertex(&vertices[1],v0,v1,f);
  }
  glTransformVertex(context,&vertices[0],false);
  glTransformVertex(context,&vertices[1],false);
  glLightVertex(context,&vertices[0]);
  glLightVertex(context,&vertices[1]);
  memcpy(&vertices[2],&vertices[1],sizeof(glVertex));
  memcpy(&vertices[3],&vertices[0],sizeof(glVertex));
  double dx = vertices[1].sx-vertices[0].sx;
  double dy = vertices[1].sy-vertices[0].sy;
  double l = sqrt(dx*dx+dy*dy);
  if (l>0.f) {dx/=l;dy/=l;};
  double nx = dy;
  double ny = -dx;
  double d = context->lineWidth*0.5;
  vertices[3].sx += nx*d;
  vertices[3].sy += ny*d;
  vertices[2].sx += nx*d;
  vertices[2].sy += ny*d;
  vertices[1].sx -= nx*d;
  vertices[1].sy -= ny*d;
  vertices[0].sx -= nx*d;
  vertices[0].sy -= ny*d;
  int clipFlags = glClipVertex(&glContext,&vertices[0]);
  clipFlags &= glClipVertex(&glContext,&vertices[1]);
  if (clipFlags == 0)
    glDrawQuad(context,&vertices[0],&vertices[1],&vertices[2],&vertices[3]);
}

void glSetVertex(glVertex *w) {
  _GLContext *v = &glContext;
  v->colorRed = w->colorRed;
  v->colorGreen = w->colorGreen;
  v->colorBlue = w->colorBlue;
  v->colorAlpha = w->colorAlpha;
  v->normalX = w->normalX;
  v->normalY = w->normalY;
  v->normalZ = w->normalZ;
  v->vertexX = w->vertexX;
  v->vertexY = w->vertexY;
  v->vertexZ = w->vertexZ;
  v->vertexW = w->vertexW;
  v->textureX = w->textureX;
  v->textureY = w->textureY;
  v->textureZ = w->textureZ;
  v->textureW = w->textureW;
}

bool backFaceCalc(const glVertex *v0,const glVertex *v1,const glVertex *v2) {
  currentBackFacing = false;
  if (glContext.twoSidedLighting || glContext.wireframe[0] || glContext.wireframe[1]) { // used only for two sided coloring/wireframe currently
    const double ax = v1->vertexX-v0->vertexX;
    const double ay = v1->vertexY-v0->vertexY;
    const double az = v1->vertexZ-v0->vertexZ;
    const double bx = v2->vertexX-v0->vertexX;
    const double by = v2->vertexY-v0->vertexY;
    const double bz = v2->vertexZ-v0->vertexZ;
    const double nx = ay*bz - az*by;
    const double ny = az*bx - ax*bz;
    const double nz = ax*by - ay*bx;
    const double *o = glContext.matrix;
    if (o[0*4+3]==0&&o[1*4+3]==0&&o[2*4+3]==0) { // check if orthographic projection by x,y,z not influencing w
      const double *m = glGetInverseModelView(&glContext);
      const double d = m[4*2+0]*nx+m[4*2+1]*ny+m[4*2+2]*nz;
      currentBackFacing = d < 0;
      if (glContext.frontFace != GL_CCW) currentBackFacing = !currentBackFacing;

    } else {
      const double *m = glGetInverseModelView(&glContext);
      const double vx = v0->vertexX-m[4*3+0];
      const double vy = v0->vertexY-m[4*3+1];
      const double vz = v0->vertexZ-m[4*3+2];
      const double d = vx*nx+vy*ny+vz*nz;
      currentBackFacing = d > 0;
      if (glContext.frontFace != GL_CCW) currentBackFacing = !currentBackFacing;
    }
  }
  return false;
}

void glEmitVertex() {
  glVertex *v = &glVertices[glCurrentVertexElement];
  _GLContext *w = &glContext;
  v->colorRed = w->colorRed;
  v->colorGreen = w->colorGreen;
  v->colorBlue = w->colorBlue;
  v->colorAlpha = w->colorAlpha;
  v->additionalSpecularColorRed = 0;
  v->additionalSpecularColorGreen = 0;
  v->additionalSpecularColorBlue = 0;
  v->normalX = w->normalX;
  v->normalY = w->normalY;
  v->normalZ = w->normalZ;
  v->vertexX = w->vertexX;
  v->vertexY = w->vertexY;
  v->vertexZ = w->vertexZ;
  v->vertexW = w->vertexW;
  v->textureX = w->textureX;
  v->textureY = w->textureY;
  v->textureZ = w->textureZ;
  v->textureW = w->textureW;
  glCurrentVertexElement++;
  switch(glContext.beginMode) {
    case GL_LINES: {
      if (glCurrentVertexElement==2) {
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],true);
        if (a == 0) {
          int clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            memcpy(&glVertices[2],&glVertices[1],sizeof(glVertex));
            memcpy(&glVertices[3],&glVertices[0],sizeof(glVertex));
            double dx = glVertices[1].sx-glVertices[0].sx;
            double dy = glVertices[1].sy-glVertices[0].sy;
            double l = sqrt(dx*dx+dy*dy);
            if (l>0.f) {dx/=l;dy/=l;};
            const double nx = dy * glContext.zoomX*additionalPointSpriteXStretch;
            const double ny = -dx * glContext.zoomY;
            const double d = glContext.lineWidth*0.5;
            glVertices[3].sx += nx*d;
            glVertices[3].sy += ny*d;
            glVertices[2].sx += nx*d;
            glVertices[2].sy += ny*d;
            glVertices[1].sx -= nx*d;
            glVertices[1].sy -= ny*d;
            glVertices[0].sx -= nx*d;
            glVertices[0].sy -= ny*d;
            glContext.forceNoCull++;
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
            glContext.forceNoCull--;
          }
        } else {
          if (a != 3) {
            glContext.forceNoCull++;
            drawClippedLine(&glContext,&glVertices[0],&glVertices[1]);
            glContext.forceNoCull--;
          }
        }
        glCurrentVertexElement = 0;
      }
    } break;
    case GL_LINE_STRIP: {
      if (glCurrentVertexElement==2) {
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],true);
        if (a == 0) {
          int clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            memcpy(&glVertices[2],&glVertices[1],sizeof(glVertex));
            memcpy(&glVertices[3],&glVertices[0],sizeof(glVertex));
            double dx = glVertices[1].sx-glVertices[0].sx;
            double dy = glVertices[1].sy-glVertices[0].sy;
            double l = sqrt(dx*dx+dy*dy);
            if (l>0.f) {dx/=l;dy/=l;};
            const double nx = dy*glContext.zoomX*additionalPointSpriteXStretch;
            const double ny = -dx*glContext.zoomY;
            const double d = glContext.lineWidth*0.5;
            glVertices[3].sx += nx*d;
            glVertices[3].sy += ny*d;
            glVertices[2].sx += nx*d;
            glVertices[2].sy += ny*d;
            glVertices[1].sx -= nx*d;
            glVertices[1].sy -= ny*d;
            glVertices[0].sx -= nx*d;
            glVertices[0].sy -= ny*d;
            glContext.forceNoCull++;
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
            glContext.forceNoCull--;
          }
        } else {
          if (a != 3) {
            glContext.forceNoCull++;
            drawClippedLine(&glContext,&glVertices[0],&glVertices[1]);
            glContext.forceNoCull--;
          }
        }
        glVertices[0]=glVertices[1];
        glCurrentVertexElement = 1;
      }
    } break;
    case GL_TRIANGLES: {
      if (glCurrentVertexElement==3) {
        if (backFaceCalc(&glVertices[0],&glVertices[1],&glVertices[2])) {glCurrentVertexElement = 0;currentBackFacing = false;break;}
        if (glContext.wireframe[currentBackFacing ? 1 : 0]) {
          glVertex vs[3];
          vs[0] = glVertices[0];
          vs[1] = glVertices[1];
          vs[2] = glVertices[2];
          glContext.beginMode = GL_LINES; glCurrentVertexElement = 0;
          glSetVertex(&vs[0]);glEmitVertex();
          glSetVertex(&vs[1]);glEmitVertex();
          glSetVertex(&vs[1]);glEmitVertex();
          glSetVertex(&vs[2]);glEmitVertex();
          glSetVertex(&vs[2]);glEmitVertex();
          glSetVertex(&vs[0]);glEmitVertex();
          glSetVertex(&vs[2]);
          glContext.beginMode = GL_TRIANGLES; glCurrentVertexElement = 0; currentBackFacing = false;
          break;
        }
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[2],true);
        if (a == 0) {
          int clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          clipFlags &= glClipVertex(&glContext,&glVertices[2]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            glLightVertex(&glContext,&glVertices[2]);
            glDrawTriangle(&glContext,&glVertices[0],&glVertices[1],&glVertices[2]);
          }
        } else {
          if (a != 7) {
            drawClippedTriangle(&glContext,&glVertices[0],&glVertices[1],&glVertices[2]);
          }
        }
        glCurrentVertexElement = 0; currentBackFacing = false; 
      }
    } break;
    case GL_QUADS: {
      if (glCurrentVertexElement==4) {
        if (backFaceCalc(&glVertices[0],&glVertices[1],&glVertices[2])) {glCurrentVertexElement = 0;currentBackFacing = false;break;}
        if (glContext.wireframe[currentBackFacing ? 1 : 0]) {
          glVertex vs[4];
          vs[0] = glVertices[0];
          vs[1] = glVertices[1];
          vs[2] = glVertices[2];
          vs[3] = glVertices[3];
          glContext.beginMode = GL_LINES; glCurrentVertexElement = 0;
          glSetVertex(&vs[0]);glEmitVertex();
          glSetVertex(&vs[1]);glEmitVertex();
          glSetVertex(&vs[1]);glEmitVertex();
          glSetVertex(&vs[2]);glEmitVertex();
          glSetVertex(&vs[2]);glEmitVertex();
          glSetVertex(&vs[3]);glEmitVertex();
          glSetVertex(&vs[3]);glEmitVertex();
          glSetVertex(&vs[0]);glEmitVertex();
          glSetVertex(&vs[3]);
          glContext.beginMode = GL_QUADS; glCurrentVertexElement = 0; currentBackFacing = false;
          break;
        }
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[2],true);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[3],true);
        if (a == 0) {
          int clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          clipFlags &= glClipVertex(&glContext,&glVertices[2]);
          clipFlags &= glClipVertex(&glContext,&glVertices[3]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            glLightVertex(&glContext,&glVertices[2]);
            glLightVertex(&glContext,&glVertices[3]);
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
          }
        } else {
          if (a != 15) {
            drawClippedQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
          }
        }
        glCurrentVertexElement = 0; currentBackFacing = false;
      }
    } break;
    case GL_POINTS: {
      if (glCurrentVertexElement==1) {
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        if (a == 0) {
          glVertices[1] = glVertices[0];
          glVertices[2] = glVertices[0];
          glVertices[3] = glVertices[0];
          const double pointSizeX = glContext.pointSize*0.5*glContext.zoomX*additionalPointSpriteXStretch;
          const double pointSizeY = glContext.pointSize*0.5*glContext.zoomY;
          glVertices[3].textureX = 0;
          glVertices[3].textureY = 1;
          glVertices[3].sx -= pointSizeX;
          glVertices[3].sy -= pointSizeY;
          glVertices[2].textureX = 1;
          glVertices[2].textureY = 1;
          glVertices[2].sx += pointSizeX;
          glVertices[2].sy -= pointSizeY;
          glVertices[1].textureX = 1;
          glVertices[1].textureY = 0;
          glVertices[1].sx += pointSizeX;
          glVertices[1].sy += pointSizeY;
          glVertices[0].textureX = 0;
          glVertices[0].textureY = 0;
          glVertices[0].sx -= pointSizeX;
          glVertices[0].sy += pointSizeY;
          int clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          clipFlags &= glClipVertex(&glContext,&glVertices[2]);
          clipFlags &= glClipVertex(&glContext,&glVertices[3]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            glLightVertex(&glContext,&glVertices[2]);
            glLightVertex(&glContext,&glVertices[3]);
            glContext.forceNoCull++;
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
            glContext.forceNoCull--;
          }
        }
        glCurrentVertexElement = 0;
      }
    } break;
    default: {
        glCurrentVertexElement = 0;
    } break;
  }
}

void glActiveTexture(GLenum texture) {
  glContext.activeTexture = texture - GL_TEXTURE0;
}

void glAlphaFunc(GLenum func, GLclampf ref) {
  glContext.alphaFunc = func;
  glContext.alphaFuncRef = ref;
}

void glBegin(GLenum mode) {
  //if (glContext.beginMode != 0) {glDone();exit(0);}
  glContext.beginMode = mode;
  glCurrentVertexElement = 0;
}

void glBindTexture(GLenum target, GLuint texture) {
  glContext.boundTextures[glContext.activeTexture] = texture;
}

void glBlendFunc(GLenum sfactor, GLenum dfactor) {
  glContext.blendFuncSFactor = sfactor;
  glContext.blendFuncDFactor = dfactor;
}

void glClear(GLbitfield mask) {
  int minX = 0;
  int minY = 0;
  int maxX = glFrameBufferWidth;
  int maxY = glFrameBufferHeight;
  if (glIsEnabled(GL_SCISSOR_TEST)) {
    combineIntoWindow(&minX,&minY,&maxX,&maxY,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
  }
  if (mask & GL_COLOR_BUFFER_BIT) {
    int r = (int)FLOOR(glContext.clearRed*255.f);
    int g = (int)FLOOR(glContext.clearGreen*255.f);
    int b = (int)FLOOR(glContext.clearBlue*255.f);
    int a = (int)FLOOR(glContext.clearAlpha*255.f);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    if (glFrameBufferBytesPerPixel==4) {
      unsigned int rgba = r|(g<<8)|(b<<16)|(a<<24);
      for (int y = minY; y < maxY; y++)
        for (int x = minX; x < maxX; x++)
          glFrameBuffer[x+y*glFrameBufferWidth] = rgba;
    }
  }    
  if (mask & GL_DEPTH_BUFFER_BIT) {
    for (int y = minY; y < maxY; y++)
      for (int x = minX; x < maxX; x++)
        glDepthBuffer[x+y*glFrameBufferWidth] = glContext.clearDepth; // clamping?
  }
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glContext.clearRed = red;
  glContext.clearGreen = green;
  glContext.clearBlue = blue;
  glContext.clearAlpha = alpha;
}

void glClearDepthf(GLclampf depth) {
  glContext.clearDepth = depth;
}

void glClearDepth(GLclampf depth) {
  glContext.clearDepth = depth;
}

void glClearStencil(GLint s) {
  glContext.clearStencil = s;
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
  glContext.colorRed = (float)red;
  glContext.colorGreen = (float)green;
  glContext.colorBlue = (float)blue;
}

void glColor3dv(const GLdouble *v) {
  glContext.colorRed = (float)v[0];
  glContext.colorGreen = (float)v[1];
  glContext.colorBlue = (float)v[2];
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
}

void glColor3fv(const GLfloat *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
  glContext.colorRed = (float)red / 255.f;
  glContext.colorGreen = (float)green / 255.f;
  glContext.colorBlue = (float)blue / 255.f;
}

void glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
  glContext.colorRed = (float)red;
  glContext.colorGreen = (float)green;
  glContext.colorBlue = (float)blue;
  glContext.colorAlpha = (float)alpha;
}

void glColor4dv (const GLdouble *v) {
  glContext.colorRed = (float)v[0];
  glContext.colorGreen = (float)v[1];
  glContext.colorBlue = (float)v[2];
  glContext.colorAlpha = (float)v[3];
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
  glContext.colorAlpha = alpha;
}

void glColor4fv(const GLfloat *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
  glContext.colorAlpha = v[3];
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
  glContext.colorRed = (float)red / 255.f;
  glContext.colorGreen = (float)green / 255.f;
  glContext.colorBlue = (float)blue / 255.f;
  glContext.colorAlpha = (float)alpha / 255.f;
}

void glColor4ubv(const GLubyte *v) {
  glContext.colorRed = (float)v[0] / 255.f;
  glContext.colorGreen = (float)v[1] / 255.f;
  glContext.colorBlue = (float)v[2] / 255.f;
  glContext.colorAlpha = (float)v[3] / 255.f;
}

void glColor3ubv(const GLubyte *v) {
  glContext.colorRed = (float)v[0] / 255.f;
  glContext.colorGreen = (float)v[1] / 255.f;
  glContext.colorBlue = (float)v[2] / 255.f;
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  glContext.maskRed = red;
  glContext.maskGreen = green;
  glContext.maskBlue = blue;
  glContext.maskAlpha = alpha;
}

void glColorMaterial(GLenum face, GLenum pname) {
  int f0 = 0; int f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  glContext.colorMaterialFace = face;
  for (int f = f0; f < f1; f++)
    glContext.colorMaterial[f] = pname;
}

void glCullFace(GLenum mode) {
  glContext.cullFaceMode = mode;
}

void glDeleteTextures (GLsizei n, GLuint *textures) {
  for (int i = 0; i < n; i++)
    glDeleteTexture(textures[i]);
}

void glDepthFunc(GLenum func) {
  glContext.depthFunc = func;
}

void glDepthMask(GLboolean flag) {
  glContext.depthMask = flag;
}

void glDepthRangef(GLclampf zNear, GLclampf zFar) {
  glContext.depthRangeZNear = zNear;
  glContext.depthRangeZFar = zFar;
}

void glDisable(GLenum cap) {
  _GLContext_enable(cap, false);
}

void glDisableClientState(GLenum array) {
}

void glEnable(GLenum cap) {
  _GLContext_enable(cap, true);
}

void glEnableClientState(GLenum array) {
}

void glEnd() {
  glContext.beginMode = 0;
}

void glFinish() {
}

void glFlush() {
}

void glFogfv(GLenum pname, GLfloat *params) {
  switch(pname) {
  case GL_FOG_COLOR: {memcpy(glContext.fogColor,params,4*sizeof(GLfloat));} break;
  case GL_FOG_START: {glContext.fogStart = params[0];} break;
  case GL_FOG_END: {glContext.fogEnd = params[0];} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = params[0];} break;
  case GL_FOG_MODE: {glContext.fogMode = (GLenum)params[0];} break;
  }
}

void glFogf(GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = param;} break;
  case GL_FOG_END: {glContext.fogEnd = param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = param;} break;
  case GL_FOG_MODE: {glContext.fogMode = (GLenum)param;} break;
  }
}

void glFogi(GLenum pname, GLint param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = (float)param;} break;
  case GL_FOG_END: {glContext.fogEnd = (float)param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = (float)param;} break;
  case GL_FOG_MODE: {glContext.fogMode = param;} break;
  }
}

void glFrontFace(GLenum mode) {
  glContext.frontFace = mode;
}

void glGenTextures (GLsizei n, GLuint *textures) {
  for(int i = 0; i < n; i++) {
    textures[i] = glNewTexture();
  }
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
  int i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=(GLboolean)glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=(GLboolean)glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params=(GLboolean)glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params=(GLboolean)glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=(GLboolean)(int)floor(glContext.clearRed);params[1]=(GLboolean)(int)floor(glContext.clearGreen);params[2]=(GLboolean)(int)floor(glContext.clearBlue);params[3]=(GLboolean)(int)floor(glContext.clearAlpha);} break;
  case GL_COLOR_MATERIAL_FACE: {*params=(GLboolean)glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params=(GLboolean)glContext.colorMaterial[glContext.colorMaterialFace==GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params=(GLboolean)glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]=(GLboolean)(int)floor(glContext.normalX);params[1]=(GLboolean)(int)floor(glContext.normalY);params[2]=(GLboolean)(int)floor(glContext.normalZ);} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]=(GLboolean)(int)floor(glContext.textureX);params[1]=(GLboolean)(int)floor(glContext.textureY);params[2]=(GLboolean)(int)floor(glContext.textureZ);params[3]=(GLboolean)(int)floor(glContext.textureW);} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=(GLboolean)(int)floor(glContext.clearDepth);} break;
  case GL_DEPTH_FUNC: {*params=(GLboolean)glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=(GLboolean)(int)floor(glContext.depthRangeZNear);params[1]=(GLboolean)(int)floor(glContext.depthRangeZFar);} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]= (GLboolean)glContext.fogColor[0];params[1]= (GLboolean)glContext.fogColor[1];params[2]= (GLboolean)glContext.fogColor[2];params[3]= (GLboolean)glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]= (GLboolean)glContext.fogStart;} break;
  case GL_FOG_END: { params[0]= (GLboolean)glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]= (GLboolean)glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]=(GLboolean)glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params=glContext.cullFaceMode == GL_BACK ? 1 : 0;} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=(GLboolean)(int)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=(GLboolean)glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(GLboolean)(int)floor(glContext.pointSize);} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=(GLboolean)glContext.scissorX0;params[1]=(GLboolean)(glFrameBufferHeight-glContext.scissorY1);params[2]=(GLboolean)(glContext.scissorX1-glContext.scissorX0);params[3]=(GLboolean)(glContext.scissorY1-glContext.scissorY0);} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=(GLboolean)glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=(GLboolean)glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=(GLboolean)glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=(GLboolean)glContext.viewportX0;params[1]=(GLboolean)(glFrameBufferHeight-glContext.viewportY1);params[2]=(GLboolean)(glContext.viewportX1-glContext.viewportX0);params[3]=(GLboolean)(glContext.viewportY1-glContext.viewportY0);} break;
  case GL_ZOOM_X: {*params=(GLboolean)(int)floor(glContext.zoomX);} break;
  case GL_ZOOM_Y: {*params=(GLboolean)(int)floor(glContext.zoomY);;} break;
  }
}

void glGetDoublev(GLenum pname, GLdouble *params) {
  int i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params=glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params=glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=glContext.clearRed;params[1]=glContext.clearGreen;params[2]=glContext.clearBlue;params[3]=glContext.clearAlpha;} break;
  case GL_COLOR_MATERIAL_FACE: {*params=glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params=glContext.colorMaterial[glContext.colorMaterialFace==GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params=glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]=glContext.normalX;params[1]=glContext.normalY;params[2]=glContext.normalZ;} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]=glContext.textureX;params[1]=glContext.textureY;params[2]=glContext.textureZ;params[3]=glContext.textureW;} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=glContext.clearDepth;} break;
  case GL_DEPTH_FUNC: {*params=glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=glContext.depthRangeZNear;params[1]=glContext.depthRangeZFar;} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]=glContext.fogColor[0];params[1]=glContext.fogColor[1];params[2]=glContext.fogColor[2];params[3]=glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]=glContext.fogStart;} break;
  case GL_FOG_END: { params[0]=glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]=glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]=glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params=glContext.cullFaceMode == GL_BACK ? 1 : 0;} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=glContext.lineWidth;} break;
  case GL_MATRIX_MODE: {*params=glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=glContext.pointSize;} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=glContext.scissorX0;params[1]=glFrameBufferHeight-glContext.scissorY1;params[2]=glContext.scissorX1-glContext.scissorX0;params[3]=glContext.scissorY1-glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=glContext.viewportX0;params[1]=glFrameBufferHeight-glContext.viewportY1;params[2]=glContext.viewportX1-glContext.viewportX0;params[3]=glContext.viewportY1-glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params=glContext.zoomX;} break;
  case GL_ZOOM_Y: {*params=glContext.zoomY;;} break;
  }
}

void glGetFloatv(GLenum pname, GLfloat *params) {
  int i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=(float)glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params= (float)glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params= (float)glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=glContext.clearRed;params[1]=glContext.clearGreen;params[2]=glContext.clearBlue;params[3]=glContext.clearAlpha;} break;
  case GL_COLOR_MATERIAL_FACE: {*params= (float)glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params= (float)glContext.colorMaterial[glContext.colorMaterialFace==GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params= (float)glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]= (float)glContext.normalX;params[1]= (float)glContext.normalY;params[2]= (float)glContext.normalZ;} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]= (float)glContext.textureX;params[1]= (float)glContext.textureY;params[2]= (float)glContext.textureZ;params[3]= (float)glContext.textureW;} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=glContext.clearDepth;} break;
  case GL_DEPTH_FUNC: {*params= (float)glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=glContext.depthRangeZNear;params[1]=glContext.depthRangeZFar;} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]=glContext.fogColor[0];params[1]=glContext.fogColor[1];params[2]=glContext.fogColor[2];params[3]=glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]=glContext.fogStart;} break;
  case GL_FOG_END: { params[0]=glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]=glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]= (float)glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params= (float)(glContext.cullFaceMode == GL_BACK ? 1 : 0);} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=glContext.lineWidth;} break;
  case GL_MATRIX_MODE: {*params= (float)glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (float)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=glContext.pointSize;} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (float)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]= (float)glContext.scissorX0;params[1]= (float)glFrameBufferHeight- (float)glContext.scissorY1;params[2]= (float)glContext.scissorX1- (float)glContext.scissorX0;params[3]= (float)glContext.scissorY1- (float)glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params= (float)glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params= (float)glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params= (float)glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (float)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (float)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]= (float)glContext.viewportX0;params[1]= (float)glFrameBufferHeight- (float)glContext.viewportY1;params[2]= (float)glContext.viewportX1- (float)glContext.viewportX0;params[3]= (float)glContext.viewportY1- (float)glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params= (float)glContext.zoomX;} break;
  case GL_ZOOM_Y: {*params= (float)glContext.zoomY;} break;
  }
}

void glGetIntegerv(GLenum pname, GLint *params) {
  int i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=(int)glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params=glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params=glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=(int)floor(glContext.clearRed);params[1]=(int)floor(glContext.clearGreen);params[2]=(int)floor(glContext.clearBlue);params[3]=(int)floor(glContext.clearAlpha);} break;
  case GL_COLOR_MATERIAL_FACE: {*params=glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params=glContext.colorMaterial[glContext.colorMaterialFace==GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params=glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]=(int)floor(glContext.normalX);params[1]=(int)floor(glContext.normalY);params[2]=(int)floor(glContext.normalZ);} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]=(int)floor(glContext.textureX);params[1]=(int)floor(glContext.textureY);params[2]=(int)floor(glContext.textureZ);params[3]=(int)floor(glContext.textureW);} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=(int)floor(glContext.clearDepth);} break;
  case GL_DEPTH_FUNC: {*params=glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=(int)floor(glContext.depthRangeZNear);params[1]=(int)floor(glContext.depthRangeZFar);} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]= (int)glContext.fogColor[0];params[1]= (int)glContext.fogColor[1];params[2]= (int)glContext.fogColor[2];params[3]= (int)glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]= (int)glContext.fogStart;} break;
  case GL_FOG_END: { params[0]= (int)glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]= (int)glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]=glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params=glContext.cullFaceMode == GL_BACK ? 1 : 0;} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=(int)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (int)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(int)floor(glContext.pointSize);} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (int)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=glContext.scissorX0;params[1]=glFrameBufferHeight-glContext.scissorY1;params[2]=glContext.scissorX1-glContext.scissorX0;params[3]=glContext.scissorY1-glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]= (int)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]= (int)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=glContext.viewportX0;params[1]=glFrameBufferHeight-glContext.viewportY1;params[2]=glContext.viewportX1-glContext.viewportX0;params[3]=glContext.viewportY1-glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params=(int)floor(glContext.zoomX);} break;
  case GL_ZOOM_Y: {*params=(int)floor(glContext.zoomY);;} break;
  }
}

GLenum glGetError() {
  return glContext.error;
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {params[0]=glContext.constantAttenuation[light-GL_LIGHT0];} break;
  case GL_LINEAR_ATTENUATION: {params[0]=glContext.linearAttenuation[light-GL_LIGHT0];} break;
  case GL_QUADRATIC_ATTENUATION: {params[0]=glContext.quadraticAttenuation[light-GL_LIGHT0];} break;
  case GL_SPOT_EXPONENT: {params[0]=glContext.spotExponent[light-GL_LIGHT0];} break;
  case GL_SPOT_CUTOFF: {params[0]=(float)(acos(glContext.spotCutOff[light-GL_LIGHT0])*180.0/PI);} break;
  case GL_SPOT_DIRECTION: // spot_direction already normalized
  case GL_POSITION:
  case GL_DIFFUSE:
  case GL_SPECULAR:
  case GL_AMBIENT: {
      params[0] = (float)glContext.lightRed[pname & 7][light - GL_LIGHT0];
      params[1] = (float)glContext.lightGreen[pname & 7][light - GL_LIGHT0];
      params[2] = (float)glContext.lightBlue[pname & 7][light - GL_LIGHT0];
      params[3] = (float)glContext.lightAlpha[pname & 7][light - GL_LIGHT0];
    }; break;
  }
}

const GLubyte *glGetString(GLenum name) {
  return (const GLubyte *)"";
}

GLboolean glIsEnabled(GLenum cap) {
  return _GLContext_isEnabled(cap);
}  

void glLightf(GLenum light, GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_SPOT_EXPONENT: {glContext.spotExponent[light - GL_LIGHT0] = param; return;}
  case GL_SPOT_CUTOFF: {glContext.spotCutOff[light - GL_LIGHT0] = (float)cos(param*PI/180.0); return;}
  }
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_SPOT_EXPONENT: {glContext.spotExponent[light - GL_LIGHT0] = params[0]; return;}
  case GL_SPOT_CUTOFF: {glContext.spotCutOff[light - GL_LIGHT0] = (float)cos(params[0]*PI/180); return;}
  }

  if (pname == GL_SPOT_DIRECTION) {
    double lx = params[0];
    double ly = params[1];
    double lz = params[2];
    double *matrix = glContext.matrixForMode[GL_MODELVIEW&1]; 
    double xl = lx * matrix[0*4+0] + ly * matrix[1*4+0] + lz * matrix[2*4+0];
    double yl = lx * matrix[0*4+1] + ly * matrix[1*4+1] + lz * matrix[2*4+1];
    double zl = lx * matrix[0*4+2] + ly * matrix[1*4+2] + lz * matrix[2*4+2];
    double k = sqrt(xl*xl+yl*yl+zl*zl);
    if (fabs(k)>0) {xl/=k;yl/=k;zl/=k;}
    glContext.lightRed[pname & 7][light - GL_LIGHT0] = xl;
    glContext.lightGreen[pname & 7][light - GL_LIGHT0] = yl;
    glContext.lightBlue[pname & 7][light - GL_LIGHT0] = zl;
    return;
  }
  if (pname == GL_POSITION) {
    double lx = params[0];
    double ly = params[1];
    double lz = params[2];
    double lw = params[3];

    double *matrix = glContext.matrixForMode[GL_MODELVIEW&1]; 
    double xl = lx * matrix[0*4+0] + ly * matrix[1*4+0] + lz * matrix[2*4+0] + lw * matrix[3*4+0];
    double yl = lx * matrix[0*4+1] + ly * matrix[1*4+1] + lz * matrix[2*4+1] + lw * matrix[3*4+1];
    double zl = lx * matrix[0*4+2] + ly * matrix[1*4+2] + lz * matrix[2*4+2] + lw * matrix[3*4+2];
    double wl = lx * matrix[0*4+3] + ly * matrix[1*4+3] + lz * matrix[2*4+3] + lw * matrix[3*4+3];
    if (wl != 0.0) {
      xl /= wl;
      yl /= wl;
      zl /= wl;
    }
    lx = xl;
    ly = yl;
    lz = zl;
    lw = wl;

    glContext.lightRed[pname & 7][light - GL_LIGHT0] = lx;
    glContext.lightGreen[pname & 7][light - GL_LIGHT0] = ly;
    glContext.lightBlue[pname & 7][light - GL_LIGHT0] = lz;
    glContext.lightAlpha[pname & 7][light - GL_LIGHT0] = lw;
    return;
  } 
  glContext.lightRed[pname & 7][light - GL_LIGHT0] = params[0];
  glContext.lightGreen[pname & 7][light - GL_LIGHT0] = params[1];
  glContext.lightBlue[pname & 7][light - GL_LIGHT0] = params[2];
  glContext.lightAlpha[pname & 7][light - GL_LIGHT0] = params[3];
}

void glLightModeli(GLenum pname, const GLenum param) {
  switch(pname) {
  case GL_LIGHT_MODEL_COLOR_CONTROL: {
    switch(param) {
    case GL_SEPARATE_SPECULAR_COLOR: {glContext.separateSpecular = true;} break;
    case GL_SINGLE_COLOR: {glContext.separateSpecular = false;} break;
    }
  } break;
  case GL_LIGHT_MODEL_TWO_SIDE: {glContext.twoSidedLighting = (int)param!=0;} break;
  }
}

void glLightModelf(GLenum pname, const GLfloat param) {
  switch(pname) {
  case GL_LIGHT_MODEL_TWO_SIDE: {glContext.twoSidedLighting = param!=0.f;} break;
  }
}

void glLineStipple(GLint factor, GLushort pattern) {
  glContext.lineStippleFactor = (float)factor;
  glContext.lineStipplePattern = pattern;
}

void glLineWidth(GLfloat width) {
  glContext.lineWidth = width;
}

void glLoadIdentity() {
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], identityMatrix, 4*4*sizeof(GLdouble));
  glUpdateMatrix();
}

void glLoadMatrixf(const GLfloat *m) {
  for (int i = 0; i < 4*4; i++) glContext.matrixForMode[glContext.matrixModeNr][i] = m[i];
  glUpdateMatrix();
}

void glLoadMatrixd(const GLdouble *m) {
  for (int i = 0; i < 4*4; i++) glContext.matrixForMode[glContext.matrixModeNr][i] = m[i];
  glUpdateMatrix();
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
  glContext.normalX = nx;
  glContext.normalY = ny;
  glContext.normalZ = nz;
}

void glNormal3fv(const GLfloat *n) {
  glContext.normalX = n[0];
  glContext.normalY = n[1];
  glContext.normalZ = n[2];
}

void glMateriali(GLenum face, GLenum pname, GLint param) {
  glMaterialf(face,pname,(float)param);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
  float p[3];
  p[0] = param;
  p[1] = param;
  p[2] = param;
  glMaterialfv(face,pname,p);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
  int f0 = 0; int f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  for(int f = f0; f<f1; f++) {
    switch(pname) {
      case GL_AMBIENT: {
        glContext.materialRed[f][0] = params[0];
        glContext.materialGreen[f][0] = params[1];
        glContext.materialBlue[f][0] = params[2];
      } break;
      case GL_DIFFUSE: {
        glContext.materialRed[f][1] = params[0];
        glContext.materialGreen[f][1] = params[1];
        glContext.materialBlue[f][1] = params[2];
      } break;
      case GL_SPECULAR: {
        glContext.materialRed[f][2] = params[0];
        glContext.materialGreen[f][2] = params[1];
        glContext.materialBlue[f][2] = params[2];
      } break;
      case GL_EMISSION: {
        glContext.materialRed[f][3] = params[0];
        glContext.materialGreen[f][3] = params[1];
        glContext.materialBlue[f][3] = params[2];
      } break;
      case GL_SHININESS: {
        glContext.materialRed[f][4] = params[0];
        glContext.materialGreen[f][4] = params[1];
        glContext.materialBlue[f][4] = params[2];
      } break;
    }
  }
}

void glMatrixMode(GLenum mode) {
  glContext.matrixModeNr = mode & 0x01;
}

void glMultMatrixf(const GLfloat *m) {
  glMatMulf2(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

void glMultMatrixd(const GLdouble *m) {
  glMatMul(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

void glPointSize(GLfloat size) {
  glContext.pointSize = size;
}

void glPolygonMode(GLenum face, GLenum mode) {
  int f0 = 0; int f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  for (int f = f0; f < f1; f++) {
    glContext.wireframe[f] = mode == GL_LINE;
  }
}

void glPolygonOffset(GLfloat factor, GLfloat units) {
  glContext.polygonOffsetFactor = factor;
  glContext.polygonOffsetUnits = units;
}

void glPopMatrix() {
  glMatrixStackPos--;
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], glMatrixStack[glMatrixStackPos], 4*4*sizeof(GLdouble));
  glUpdateMatrix();
}

void glPushMatrix() {
  memcpy(glMatrixStack[glMatrixStackPos], glContext.matrixForMode[glContext.matrixModeNr], 4*4*sizeof(GLdouble));
  glMatrixStackPos++;
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
  for (int yp = 0; yp < height; yp++) {
    int y2 = yp + glFrameBufferHeight - 1 - y;
    if (y2 < 0 || y2 >= glFrameBufferHeight) continue;
    for (int xp = 0; xp < width; xp++) {
      int x2 = xp + x;
      if (x2 < 0 || x2 >= glFrameBufferWidth) continue;
      const unsigned int rgba = glFrameBuffer[x2+y2*glFrameBufferWidth];
      if (type == GL_UNSIGNED_BYTE||type==GL_BYTE) {
        switch(format) {
        case GL_RGBA: {((unsigned int*)pixels)[xp+yp*width] = rgba;} break;
        case GL_BGRA: {((unsigned int*)pixels)[xp+yp*width] = FLIPRB(rgba);} break;
        case GL_RED: {((unsigned char*)pixels)[xp+yp*width] = (unsigned char)(rgba & 0xff);} break;
        case GL_GREEN: {((unsigned char*)pixels)[xp+yp*width] = (unsigned char)((rgba>>8)& 0xff);} break;
        case GL_BLUE: {((unsigned char*)pixels)[xp+yp*width] = (unsigned char)((rgba>>16)& 0xff);} break;
        case GL_RGB: {unsigned char*c=&(((unsigned char*)pixels)[(xp+yp*width)*3]);c[0]= (unsigned char)(rgba&255);c[1]= (unsigned char)((rgba>>8)&255);c[2]= (unsigned char)((rgba>>16)&255);} break;
        case GL_BGR: {unsigned char*c=&(((unsigned char*)pixels)[(xp+yp*width)*3]);c[2]= (unsigned char)(rgba&255);c[1]= (unsigned char)((rgba>>8)&255);c[0]= (unsigned char)((rgba>>16)&255);} break;
        }
      }
    }
  }
}

void glRotate(GLdouble *matrix, GLfloat angleInRadians, GLfloat x, GLfloat y, GLfloat z) {
  double m[16], rotate[16];
  double OneMinusCosAngle, CosAngle, SinAngle;
  double A_OneMinusCosAngle, C_OneMinusCosAngle;
  CosAngle = cos(angleInRadians);
  OneMinusCosAngle = 1.f - CosAngle;
  SinAngle = sin(angleInRadians);
  A_OneMinusCosAngle = x*OneMinusCosAngle;
  C_OneMinusCosAngle = z*OneMinusCosAngle;
  for (int i = 0; i < 16; i++) m[i] = matrix[i];
  rotate[0] = x*A_OneMinusCosAngle + CosAngle;
  rotate[1] = y*A_OneMinusCosAngle + z * SinAngle;
  rotate[2] = z*A_OneMinusCosAngle - y * SinAngle;
  rotate[3] = 0.0;

  rotate[4] = y*A_OneMinusCosAngle-z*SinAngle;
  rotate[5] = y*y*OneMinusCosAngle+CosAngle;
  rotate[6] = y*C_OneMinusCosAngle+x*SinAngle;
  rotate[7] = 0.0;

  rotate[8] = x*C_OneMinusCosAngle+y*SinAngle;
  rotate[9] = y*C_OneMinusCosAngle-x*SinAngle;
  rotate[10] = z*C_OneMinusCosAngle+CosAngle;
  rotate[11] = 0.0; 

  matrix[0] = m[0]*rotate[0]+m[4]*rotate[1]+m[8]*rotate[2];
  matrix[4] = m[0]*rotate[4]+m[4]*rotate[5]+m[8]*rotate[6];
  matrix[8] = m[0]*rotate[8]+m[4]*rotate[9]+m[8]*rotate[10];

  matrix[1] = m[1]*rotate[0]+m[5]*rotate[1]+m[9]*rotate[2];
  matrix[5] = m[1]*rotate[4]+m[5]*rotate[5]+m[9]*rotate[6];
  matrix[9] = m[1]*rotate[8]+m[5]*rotate[9]+m[9]*rotate[10];

  matrix[2] = m[2]*rotate[0]+m[6]*rotate[1]+m[10]*rotate[2];
  matrix[6] = m[2]*rotate[4]+m[6]*rotate[5]+m[10]*rotate[6];
  matrix[10] = m[2]*rotate[8]+m[6]*rotate[9]+m[10]*rotate[10];

  matrix[3] = m[3]*rotate[0]+m[7]*rotate[1]+m[11]*rotate[2];
  matrix[7] = m[3]*rotate[4]+m[7]*rotate[5]+m[11]*rotate[6];
  matrix[11] = m[3]*rotate[8]+m[7]*rotate[9]+m[11]*rotate[10];
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  float l = (float)sqrt(x*x+y*y+z*z);
  if (l>0) {
    x/=l;
    y/=l;
    z/=l;
  }
  glRotate(matrix,(float)(angle*2.0*PI/360.0),x,y,z);
  glUpdateMatrix();
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  matrix[0] *= (double)x;
  matrix[4] *= (double)y;
  matrix[8] *= (double)z;

  matrix[1] *= (double)x;
  matrix[5] *= (double)y;
  matrix[9] *= (double)z;

  matrix[2] *= (double)x;
  matrix[6] *= (double)y;
  matrix[10] *= (double)z;

  matrix[3] *= (double)x;
  matrix[7] *= (double)y;
  matrix[11] *= (double)z;
  glUpdateMatrix();
}

void glScalefv(const GLfloat *v) {
  glScalef(v[0],v[1],v[2]);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  matrix[0] *= (double)x;
  matrix[4] *= (double)y;
  matrix[8] *= (double)z;

  matrix[1] *= (double)x;
  matrix[5] *= (double)y;
  matrix[9] *= (double)z;

  matrix[2] *= (double)x;
  matrix[6] *= (double)y;
  matrix[10] *= (double)z;

  matrix[3] *= (double)x;
  matrix[7] *= (double)y;
  matrix[11] *= (double)z;
  glUpdateMatrix();
}

void glScaledv(const GLdouble *v) {
  glScaled(v[0],v[1],v[2]);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  glContext.scissorX0 = x;
  glContext.scissorY0 = glFrameBufferHeight-y-height;
  glContext.scissorX1 = x+width;
  glContext.scissorY1 = glFrameBufferHeight-y;
}

void glShadeModel(GLenum mode) {
  glContext.shadeMode = mode;
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask) {
  glContext.stencilFunc = func;
  glContext.stencilFuncRef = ref;
  glContext.stencilFuncMask = mask;
}

void glStencilMask(GLuint mask) {
  glContext.stencilMask = mask;
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
  glContext.stencilOpFail = fail;
  glContext.stencilOpZFail = zfail;
  glContext.stencilOpZPass = zpass;
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
}

void glTexEnvi(GLenum target, GLenum pname, GLint param) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  switch(pname) {
    case GL_TEXTURE_ENV_MODE: {
      t->texEnvMode = param;
    } break;
  }
}

void glTexGeni(GLenum coord, GLenum pname, GLint param) {
  switch(coord) {
  case GL_S: {glContext.texGenS = param;} break;
  case GL_T: {glContext.texGenT = param;} break;
  }
}

void glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    free(t->data);
    t->data = NULL;
  }
  t->data = (unsigned int*)malloc(sizeof(unsigned int)*width*height);
  t->width = width;
  t->height = height;
  int rIn=-1,gIn=-1,bIn=-1,aIn=-1,dIn=-1,sIn=-1;
  int formatStride = 4;
  switch(format) {
  case GL_ALPHA:{aIn=0;formatStride = 1;} break;
  case GL_RED:{rIn=0;formatStride = 1;} break;
  case GL_RG:{rIn=0;gIn=1;formatStride = 2;} break;
  case GL_RGB:{rIn=0;gIn=1;bIn=2;formatStride = 3;} break;
  case GL_BGR:{rIn=2;gIn=1;bIn=0;formatStride = 3;} break;
  case GL_RGBA:{rIn=0;gIn=1;bIn=2;aIn=3;formatStride = 4;} break;
  case GL_BGRA:{rIn=2;gIn=1;bIn=0;aIn=3;formatStride = 4;} break;
  case GL_RED_INTEGER:{rIn=0;} break;
  case GL_RG_INTEGER:{rIn=0;gIn=1;} break;
  case GL_RGB_INTEGER:{rIn=0;gIn=1;bIn=2;} break;
  case GL_BGR_INTEGER:{rIn=2;gIn=1;bIn=0;} break;
  case GL_RGBA_INTEGER:{rIn=0;gIn=1;bIn=2;aIn=3;} break;
  case GL_BGRA_INTEGER:{rIn=2;gIn=1;bIn=0;aIn=3;} break;
  case GL_STENCIL_INDEX:{sIn=0;} break;
  case GL_DEPTH_COMPONENT:{dIn=0;} break;
  case GL_DEPTH_STENCIL:{dIn=0;sIn=1;} break;
  }
  int input[4];
  input[0]=0xff;
  input[1]=0xff;
  input[2]=0xff;
  input[3]=0xff;
  for (int y = 0; y < height; y++) {
    int i2 = y*width;
    int i = y*width;
    for (int x = 0; x < width; x++) {
      if (pixels != NULL) {
        input[0] = ((unsigned char*)pixels)[i2*formatStride];
        if (formatStride>=2)
          input[1] = ((unsigned char*)pixels)[i2*formatStride+1];
        if (formatStride>=3)
          input[2] = ((unsigned char*)pixels)[i2*formatStride+2];
        if (formatStride>=4)
          input[3] = ((unsigned char*)pixels)[i2*formatStride+3];
      }
      unsigned int rgba = format == GL_ALPHA ? 0x00ffffff : 0x00000000;
      if (rIn != -1) rgba |= input[rIn];
      if (gIn != -1) rgba |= input[gIn]<<8;
      if (bIn != -1) rgba |= input[bIn]<<16;
      if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
      t->data[i] = rgba;
      i++;
      i2++;
    }
  }
}

void glTexParameteri(GLenum target, GLenum pname, GLint param) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  switch(pname) {
  case GL_TEXTURE_BASE_LEVEL: {t->baseLevel = param; } break;
  case GL_TEXTURE_LOD_BIAS: {t->lodBias = param; } break;
  case GL_TEXTURE_MIN_FILTER: {t->minFilter = param; } break;
  case GL_TEXTURE_MAG_FILTER: {t->magFilter = param; } break;
  case GL_TEXTURE_MIN_LOD: {t->minLod = param; } break;
  case GL_TEXTURE_MAX_LOD: {t->maxLod = param; } break;
  case GL_TEXTURE_MAX_LEVEL: {t->maxLevel = param; } break;
  case GL_TEXTURE_WRAP_S: {t->wrapS = param; } break;
  case GL_TEXTURE_WRAP_T: {t->wrapT = param; } break;
  case GL_TEXTURE_WRAP_R: {t->wrapR = param; } break;
  }
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data == NULL) return;
  int rIn=-1,gIn=-1,bIn=-1,aIn=-1,dIn=-1,sIn=-1;
  int formatStride = 4;
  switch(format) {
  case GL_ALPHA:{aIn=0;formatStride = 1;} break;
  case GL_RED:{rIn=0;formatStride = 1;} break;
  case GL_RG:{rIn=0;gIn=1;formatStride = 2;} break;
  case GL_RGB:{rIn=0;gIn=1;bIn=2;formatStride = 3;} break;
  case GL_BGR:{rIn=2;gIn=1;bIn=0;formatStride = 3;} break;
  case GL_RGBA:{rIn=0;gIn=1;bIn=2;aIn=3;formatStride = 4;} break;
  case GL_BGRA:{rIn=2;gIn=1;bIn=0;aIn=3;formatStride = 4;} break;
  case GL_RED_INTEGER:{rIn=0;} break;
  case GL_RG_INTEGER:{rIn=0;gIn=1;} break;
  case GL_RGB_INTEGER:{rIn=0;gIn=1;bIn=2;} break;
  case GL_BGR_INTEGER:{rIn=2;gIn=1;bIn=0;} break;
  case GL_RGBA_INTEGER:{rIn=0;gIn=1;bIn=2;aIn=3;} break;
  case GL_BGRA_INTEGER:{rIn=2;gIn=1;bIn=0;aIn=3;} break;
  case GL_STENCIL_INDEX:{sIn=0;} break;
  case GL_DEPTH_COMPONENT:{dIn=0;} break;
  case GL_DEPTH_STENCIL:{dIn=0;sIn=1;} break;
  }
  int input[4];
  input[0]=0xff;
  input[1]=0xff;
  input[2]=0xff;
  input[3]=0xff;
  for (int y = 0; y < height; y++) {
    int i2 = y*width;
    for (int x = 0; x < width; x++) {
      if (pixels != NULL) {
        input[0] = ((unsigned char*)pixels)[i2*formatStride];
        if (formatStride>=2)
          input[1] = ((unsigned char*)pixels)[i2*formatStride+1];
        if (formatStride>=3)
          input[2] = ((unsigned char*)pixels)[i2*formatStride+2];
        if (formatStride>=4)
          input[3] = ((unsigned char*)pixels)[i2*formatStride+3];
      }
      unsigned int rgba = format == GL_ALPHA ? 0x00ffffff : 0x00000000;
      if (rIn != -1) rgba |= input[rIn];
      if (gIn != -1) rgba |= input[gIn]<<8;
      if (bIn != -1) rgba |= input[bIn]<<16;
      if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
      int x2 = x;
      int y2 = y;
      x2 += xoffset;
      y2 += yoffset;
      if (x2 >= 0 && y2 >= 0 && x2 < (int)t->width && y2 < (int)t->height)
        t->data[x2+y2*t->width] = rgba;
      i2++;
    }
  }
}


void glTranslate(GLdouble *matrix, GLfloat x, GLfloat y, GLfloat z) {
  matrix[12]=matrix[0]*(double)x+matrix[4]*(double)y+matrix[8]*(double)z+matrix[12];
  matrix[13]=matrix[1]*(double)x+matrix[5]*(double)y+matrix[9]*(double)z+matrix[13];
  matrix[14]=matrix[2]*(double)x+matrix[6]*(double)y+matrix[10]*(double)z+matrix[14];
  matrix[15]=matrix[3]*(double)x+matrix[7]*(double)y+matrix[11]*(double)z+matrix[15];
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glTranslate(matrix,x,y,z);
  glUpdateMatrix();
}

void glTranslatefv(const GLfloat *v) {
  glTranslatef(v[0],v[1],v[2]);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glTranslate(matrix,(float)x,(float)y,(float)z);
  glUpdateMatrix();
}

void glTranslatedv(const GLdouble *v) {
  glTranslated(v[0],v[1],v[2]);
}

void glVertex2f(GLfloat x, GLfloat y) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glEmitVertex();
}

void glVertex2fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glEmitVertex();
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glEmitVertex();
}

void glVertex3fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glContext.vertexZ = v[2];
  glEmitVertex();
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glContext.vertexW = w;
  glEmitVertex();
}

void glVertex4fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glContext.vertexZ = v[2];
  glContext.vertexW = v[3];
  glEmitVertex();
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  glContext.viewportX0 = x;
  glContext.viewportY0 = glFrameBufferHeight-y-height;
  glContext.viewportX1 = x+width;
  glContext.viewportY1 = glFrameBufferHeight-y;
}

void glNormal3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.normalX = x;
  glContext.normalY = y;
  glContext.normalZ = z;
}

void glNormal3dv(const GLdouble *p) {
  glContext.normalX = p[0];
  glContext.normalY = p[1];
  glContext.normalZ = p[2];
}

void glVertex2d(GLdouble x, GLdouble y) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glEmitVertex();
}

void glVertex2dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glEmitVertex();
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glEmitVertex();
}

void glVertex3dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glContext.vertexZ = p[2];
  glEmitVertex();
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glContext.vertexW = w;
  glEmitVertex();
}

void glVertex4dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glContext.vertexZ = p[2];
  glContext.vertexW = p[3];
  glEmitVertex();
}

void glTexCoord1f(GLfloat x) {
  glContext.textureX = x;
}

void glTexCoord2f(GLfloat x, GLfloat y) {
  glContext.textureX = x;
  glContext.textureY = y;
}

void glTexCoord3f(GLfloat x, GLfloat y, GLfloat z) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
}

void glTexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
  glContext.textureW = w;
}

void glTexCoord1d(GLdouble x) {
  glContext.textureX = x;
}

void glTexCoord2d(GLdouble x, GLdouble y) {
  glContext.textureX = x;
  glContext.textureY = y;
}

void glTexCoord3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
}

void glTexCoord4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
  glContext.textureW = w;
}

void glTexCoord1dv(const GLdouble *v) {
  glTexCoord1d(v[0]);
}

void glTexCoord1fv(const GLfloat *v) {
  glTexCoord1f(v[0]);
}

void glTexCoord2dv(const GLdouble *v) {
  glTexCoord2d(v[0],v[1]);
}

void glTexCoord2fv(const GLfloat *v) {
  glTexCoord2f(v[0],v[1]);
}

void glTexCoord3dv(const GLdouble *v) {
  glTexCoord3d(v[0],v[1],v[2]);
}

void glTexCoord3fv(const GLfloat *v) {
  glTexCoord3f(v[0],v[1],v[2]);
}

void glTexCoord4dv(const GLdouble *v) {
  glTexCoord4d(v[0],v[1],v[2],v[3]);
}

void glTexCoord4fv(const GLfloat *v) {
  glTexCoord4f(v[0],v[1],v[2],v[3]);
}

// mask not supported and behaves different than glPushAttrib + only small Stack Depth, yet
void glPushAttrib(GLbitfield mask) {
  glContext.pushAttribBitsHere = mask; // maybe evaluate at glPopAttrib later
  memcpy(&glAttribStack[glAttribStackPos], &glContext, sizeof(_GLContext));
  glContext.pushAttribBitsHere = 0;
  glAttribStackPos++;
}

void glPopAttrib() {
  glAttribStackPos--;
  memcpy(&glContext,&glAttribStack[glAttribStackPos], sizeof(_GLContext));
  glUpdateMatrix();
}

void glTexParameterfv(GLenum target, GLenum pname, GLfloat *param) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  switch(pname) {
    case GL_TEXTURE_BORDER_COLOR: {
      t->borderColorRed = param[0];
      t->borderColorGreen = param[1];
      t->borderColorBlue = param[2];
      t->borderColorAlpha = param[3];
    } break;
  }
}

void glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glContext.blendColorRed = red;
  glContext.blendColorGreen = green;
  glContext.blendColorBlue = blue;
  glContext.blendColorAlpha = alpha;
}

void glBlendEquation(GLenum mode) {
  glContext.blendEquation = mode;
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ---------------------------- GLU --------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// -----------
// -----------
// -----------
static void MulMatVecf(float *dest, float *m, float *v) {
  float x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  float y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  float z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  float w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

static void MulMatVecd(double *dest, double *m, double *v) {
  double x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  double y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  double z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  double w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

static void MulMatMatf(float *dest, float *m1, float *m2) {
  glMatMulf(m1,m2,dest);
}

static void MulMatMatd(double *dest, double *m1, double *m2) {
  glMatMul(m1,m2,dest);
}

static void normalize(double *v) {
  double l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if (l>0) {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

static void planeNormal(double *normal, double *pvector1, double *pvector2) {
  normal[0]=(pvector1[1]*pvector2[2])-(pvector1[2]*pvector2[1]);
  normal[1]=(pvector1[2]*pvector2[0])-(pvector1[0]*pvector2[2]);
  normal[2]=(pvector1[0]*pvector2[1])-(pvector1[1]*pvector2[0]);
}

void __glFrustum(double *matrix, double left, double right, double bottom, double top, double znear, double zfar) {
  double matrix2[16];
  double temp, temp2, temp3, temp4;
  temp = 2.0 * znear;
  temp2 = right - left;
  temp3 = top - bottom;
  temp4 = zfar - znear;
  matrix2[0] = temp / temp2;
  matrix2[1] = 0;
  matrix2[2] = 0;
  matrix2[3] = 0;
  matrix2[4] = 0;
  matrix2[5] = temp/temp3;
  matrix2[6] = 0;
  matrix2[7] = 0;
  matrix2[8] = (right + left)/temp2;
  matrix2[9] = (top + bottom)/temp3;
  matrix2[10] = (-zfar-znear)/temp4;
  matrix2[11] = -1;
  matrix2[12] = 0;
  matrix2[13] = 0;
  matrix2[14] = (-temp*zfar)/temp4;
  matrix2[15] = 0.0;
  MulMatMatd(matrix, matrix, matrix2);
}

void __glPerspective(double *matrix, double fovyInDegrees, double aspectRatio, double znear, double zfar) {
  double ymax, xmax;
  ymax = znear * tan(fovyInDegrees*PI/360.f);
  xmax = ymax * aspectRatio;
  __glFrustum(matrix,-xmax,xmax,-ymax,ymax,znear,zfar);
}

void glLookAt(double *matrix, double *eyePosition3D, double *center3D, double *upVector3D) {
  double forward[3], side[3], up[3];
  double matrix2[16], resultMatrix[16];
  forward[0] = center3D[0]-eyePosition3D[0];
  forward[1] = center3D[1]-eyePosition3D[1];
  forward[2] = center3D[2]-eyePosition3D[2];
  normalize(forward);
  planeNormal(side, forward, upVector3D);
  normalize(side);
  planeNormal(up, side, forward);
  matrix2[0] = side[0];
  matrix2[4] = side[1];
  matrix2[8] = side[2];
  matrix2[12] = 0.0;
  matrix2[1] = up[0];
  matrix2[5] = up[1];
  matrix2[9] = up[2];
  matrix2[13] = 0.0;
  matrix2[2] = -forward[0];
  matrix2[6] = -forward[1];
  matrix2[10] = -forward[2];
  matrix2[14] = 0.0;
  matrix2[3] = matrix2[7] = matrix2[11] = 0.0;
  matrix2[15] = 1.0;
  glMatMul(matrix, matrix2, resultMatrix);
  glTranslate(resultMatrix,(float)-eyePosition3D[0], (float)-eyePosition3D[1], (float)-eyePosition3D[2]);
  memcpy(matrix,resultMatrix,16*sizeof(double));
}

int glInvertMatrixf(const float *m, float *out)
{
   //https://archive.mesa3d.org/older-versions/9.x/9.0/
   //MesaLib-9.0.zip/Mesa-9.0\src\mesa\math\m_matrix.c
   float wtmp[4][8];
   float m0, m1, m2, m3, s;
   float *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
   r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
   r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

   r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
   r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
   r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

   r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
   r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
   r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

   r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
   r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
   r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (FABSF(r3[0])>FABSF(r2[0])) SWAP_ROWS_FLOAT(r3, r2);
   if (FABSF(r2[0])>FABSF(r1[0])) SWAP_ROWS_FLOAT(r2, r1);
   if (FABSF(r1[0])>FABSF(r0[0])) SWAP_ROWS_FLOAT(r1, r0);
   if (0.0 == r0[0])  return GL_FALSE;

   /* eliminate first variable     */
   m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
   s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
   s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
   s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r0[5];
   if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r0[6];
   if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r0[7];
   if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (FABSF(r3[1])>FABSF(r2[1])) SWAP_ROWS_FLOAT(r3, r2);
   if (FABSF(r2[1])>FABSF(r1[1])) SWAP_ROWS_FLOAT(r2, r1);
   if (0.0 == r1[1])  return GL_FALSE;

   /* eliminate second variable */
   m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
   r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
   s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (FABSF(r3[2])>FABSF(r2[2])) SWAP_ROWS_FLOAT(r3, r2);
   if (0.0 == r2[2])  return GL_FALSE;

   /* eliminate third variable */
   m3 = r3[2]/r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
   r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
   r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3]) return GL_FALSE;

   s = 1.0F/r3[3];             /* now back substitute row 3 */
   r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

   m2 = r2[3];                 /* now back substitute row 2 */
   s  = 1.0F/r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
   r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
   r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
   r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];                 /* now back substitute row 1 */
   s  = 1.0F/r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
   r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
   r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];                 /* now back substitute row 0 */
   s  = 1.0F/r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
   r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out,0,0) = r0[4]; MAT(out,0,1) = r0[5],
   MAT(out,0,2) = r0[6]; MAT(out,0,3) = r0[7],
   MAT(out,1,0) = r1[4]; MAT(out,1,1) = r1[5],
   MAT(out,1,2) = r1[6]; MAT(out,1,3) = r1[7],
   MAT(out,2,0) = r2[4]; MAT(out,2,1) = r2[5],
   MAT(out,2,2) = r2[6]; MAT(out,2,3) = r2[7],
   MAT(out,3,0) = r3[4]; MAT(out,3,1) = r3[5],
   MAT(out,3,2) = r3[6]; MAT(out,3,3) = r3[7];

   return GL_TRUE;
}

int glInvertMatrix(const double *m, double *out)
{
   //https://archive.mesa3d.org/older-versions/9.x/9.0/
   //MesaLib-9.0.zip/Mesa-9.0\src\mesa\math\m_matrix.c
   double wtmp[4][8];
   double m0, m1, m2, m3, s;
   double *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m,0,0), r0[1] = MAT(m,0,1),
   r0[2] = MAT(m,0,2), r0[3] = MAT(m,0,3),
   r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,

   r1[0] = MAT(m,1,0), r1[1] = MAT(m,1,1),
   r1[2] = MAT(m,1,2), r1[3] = MAT(m,1,3),
   r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,

   r2[0] = MAT(m,2,0), r2[1] = MAT(m,2,1),
   r2[2] = MAT(m,2,2), r2[3] = MAT(m,2,3),
   r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,

   r3[0] = MAT(m,3,0), r3[1] = MAT(m,3,1),
   r3[2] = MAT(m,3,2), r3[3] = MAT(m,3,3),
   r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (FABSF(r3[0])>FABSF(r2[0])) SWAP_ROWS_DOUBLE(r3, r2);
   if (FABSF(r2[0])>FABSF(r1[0])) SWAP_ROWS_DOUBLE(r2, r1);
   if (FABSF(r1[0])>FABSF(r0[0])) SWAP_ROWS_DOUBLE(r1, r0);
   if (0.0 == r0[0])  return GL_FALSE;

   /* eliminate first variable     */
   m1 = r1[0]/r0[0]; m2 = r2[0]/r0[0]; m3 = r3[0]/r0[0];
   s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
   s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
   s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) { r1[4] -= m1 * s; r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r0[5];
   if (s != 0.0) { r1[5] -= m1 * s; r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r0[6];
   if (s != 0.0) { r1[6] -= m1 * s; r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r0[7];
   if (s != 0.0) { r1[7] -= m1 * s; r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (FABSF(r3[1])>FABSF(r2[1])) SWAP_ROWS_DOUBLE(r3, r2);
   if (FABSF(r2[1])>FABSF(r1[1])) SWAP_ROWS_DOUBLE(r2, r1);
   if (0.0 == r1[1])  return GL_FALSE;

   /* eliminate second variable */
   m2 = r2[1]/r1[1]; m3 = r3[1]/r1[1];
   r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
   s = r1[4]; if (0.0 != s) { r2[4] -= m2 * s; r3[4] -= m3 * s; }
   s = r1[5]; if (0.0 != s) { r2[5] -= m2 * s; r3[5] -= m3 * s; }
   s = r1[6]; if (0.0 != s) { r2[6] -= m2 * s; r3[6] -= m3 * s; }
   s = r1[7]; if (0.0 != s) { r2[7] -= m2 * s; r3[7] -= m3 * s; }

   /* choose pivot - or die */
   if (FABSF(r3[2])>FABSF(r2[2])) SWAP_ROWS_DOUBLE(r3, r2);
   if (0.0 == r2[2])  return GL_FALSE;

   /* eliminate third variable */
   m3 = r3[2]/r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
   r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6],
   r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3]) return GL_FALSE;

   s = 1.0F/r3[3];             /* now back substitute row 3 */
   r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;

   m2 = r2[3];                 /* now back substitute row 2 */
   s  = 1.0F/r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
   r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
   r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
   r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];                 /* now back substitute row 1 */
   s  = 1.0F/r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
   r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
   r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];                 /* now back substitute row 0 */
   s  = 1.0F/r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
   r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out,0,0) = r0[4]; MAT(out,0,1) = r0[5],
   MAT(out,0,2) = r0[6]; MAT(out,0,3) = r0[7],
   MAT(out,1,0) = r1[4]; MAT(out,1,1) = r1[5],
   MAT(out,1,2) = r1[6]; MAT(out,1,3) = r1[7],
   MAT(out,2,0) = r2[4]; MAT(out,2,1) = r2[5],
   MAT(out,2,2) = r2[6]; MAT(out,2,3) = r2[7],
   MAT(out,3,0) = r3[4]; MAT(out,3,1) = r3[5],
   MAT(out,3,2) = r3[6]; MAT(out,3,3) = r3[7];

   return GL_TRUE;
}

int gluInvertMatrixf(const float *m, float *dest) {
  return glInvertMatrixf(m,dest);
}

int gluInvertMatrix(const double *m, double *dest) {
  return glInvertMatrix(m,dest);
}

int glUnProjectf(float winx, float winy, float winz,
     float *modelview, float *projection,
     int *viewport,
     float *objectCoordinate)
{
  float m[16], A[16];
  float in[4], out[4];
  
  MulMatMatf(A, projection, modelview);
  if(glInvertMatrixf(A, m)==0)
    return 0;
  
  in[0]=(float)(((winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0)/glContext.zoomX);
  in[1]=(float)(((winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0)/glContext.zoomY);
  in[2]=(float)(2.0*winz-1.0);
  in[3]=(float)(1.0);
  
  MulMatVecf(out, m, in);
  if(out[3]==0.0)
    return 0;
  out[3]=(float)(1.0/out[3]);
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

int glUnProject(double winx, double winy, double winz,
     double *modelview, double *projection,
     int *viewport,
     double *objectCoordinate)
{
  double m[16], A[16];
  double in[4], out[4];
  
  MulMatMatd(A, projection, modelview);
  if(glInvertMatrix(A, m)==0)
    return 0;
  
  in[0]=((winx-(double)viewport[0])/(double)viewport[2]*2.0-1.0)/glContext.zoomX;
  in[1]=((winy-(double)viewport[1])/(double)viewport[3]*2.0-1.0)/glContext.zoomY;
  in[2]=2.0*winz-1.0;
  in[3]=1.0;
  
  MulMatVecd(out, m, in);
  if(out[3]==0.0)
    return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

int glProjectfx(float objx, float objy, float objz,
  float *modelview, float *projection,
  int *viewport,
  float *windowCoordinate) {
  float in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  MulMatVecf(in, modelview, in);
  MulMatVecf(in, projection, in);
  if (in[3]==0) return 0;
  in[0]/=in[3];
  in[1]/=in[3];
  in[2]/=in[3];
  in[3] = 1;
  in[0] *= (float)(viewport[2]*0.5*glContext.zoomX);
  in[1] *= (float)(viewport[3]*-0.5*glContext.zoomY);
  in[0] += (float)(viewport[0]+viewport[2]*0.5);
  in[1] += (float)(viewport[1]+viewport[3]*0.5);
  windowCoordinate[0] = in[0];
  windowCoordinate[1] = in[1];
  windowCoordinate[2] = in[2];
  return 1;
}

int glProjectx(double objx, double objy, double objz,
  double *modelview, double *projection,
  int *viewport,
  double *windowCoordinate) {
  double in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  MulMatVecd(in, modelview, in);
  MulMatVecd(in, projection, in);
  if (in[3]==0) return 0;
  in[0]/=in[3];
  in[1]/=in[3];
  in[2]/=in[3];
  in[3] = 1;
  in[0] *= viewport[2]*0.5*glContext.zoomX;
  in[1] *= viewport[3]*-0.5*glContext.zoomY;
  in[0] += viewport[0]+viewport[2]*0.5;
  in[1] += viewport[1]+viewport[3]*0.5;
  windowCoordinate[0] = in[0];
  windowCoordinate[1] = in[1];
  windowCoordinate[2] = in[2];
  return 1;
}

int glProjectf(float objx, float objy, float objz,
  float *modelview, float *projection,
  int *viewport,
  float *windowCoordinate)
{
  float fTempo[8];
  fTempo[0]=modelview[0]*objx+
  modelview[4]*objy+
  modelview[8]*objz+
  modelview[12];  //w is always 1
  fTempo[1]=modelview[1]*objx+
  modelview[5]*objy+
  modelview[9]*objz+
  modelview[13];
  fTempo[2]=modelview[2]*objx+
  modelview[6]*objy+
  modelview[10]*objz+
  modelview[14];
  fTempo[3]=modelview[3]*objx+
  modelview[7]*objy+
  modelview[11]*objz+
  modelview[15];
  fTempo[4]=projection[0]*fTempo[0]+	
  projection[4]*fTempo[1]+
  projection[8]*fTempo[2]+
  projection[12]*fTempo[3];
  fTempo[5]=projection[1]*fTempo[0]+	
  projection[5]*fTempo[1]+
  projection[9]*fTempo[2]+
  projection[13]*fTempo[3];
  fTempo[6]=projection[2]*fTempo[0]+	
  projection[6]*fTempo[1]+
  projection[10]*fTempo[2]+
  projection[14]*fTempo[3];
  fTempo[7]=-fTempo[2];
  
  if(fTempo[7]==0.0)  //The w value
    return 0;
  
  fTempo[7]=(float)(1.0/fTempo[7]);
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  
  windowCoordinate[0]=(float)((fTempo[4]*0.5*glContext.zoomX+0.5)*viewport[2]+viewport[0]);
  windowCoordinate[1]=(float)((fTempo[5]*0.5*glContext.zoomY+0.5)*viewport[3]+viewport[1]);
  windowCoordinate[2]=(float)((1.0+fTempo[6])*0.5);  //Between 0 and 1
  return 1;
}

int glProject(double objx, double objy, double objz,
 double *modelview, double *projection,
 int *viewport,
 double *windowCoordinate)
{
  double fTempo[8];
  fTempo[0]=modelview[0]*objx+
  modelview[4]*objy+
  modelview[8]*objz+
  modelview[12];  //w is always 1
  fTempo[1]=modelview[1]*objx+
  modelview[5]*objy+
  modelview[9]*objz+
  modelview[13];
  fTempo[2]=modelview[2]*objx+
  modelview[6]*objy+
  modelview[10]*objz+
  modelview[14];
  fTempo[3]=modelview[3]*objx+
  modelview[7]*objy+
  modelview[11]*objz+
  modelview[15];
  fTempo[4]=projection[0]*fTempo[0]+	
  projection[4]*fTempo[1]+
  projection[8]*fTempo[2]+
  projection[12]*fTempo[3];
  fTempo[5]=projection[1]*fTempo[0]+	
  projection[5]*fTempo[1]+
  projection[9]*fTempo[2]+
  projection[13]*fTempo[3];
  fTempo[6]=projection[2]*fTempo[0]+	
  projection[6]*fTempo[1]+
  projection[10]*fTempo[2]+
  projection[14]*fTempo[3];
  fTempo[7]=-fTempo[2];
  
  if(fTempo[7]==0.0)  //The w value
    return 0;
  
  fTempo[7]=1.0/fTempo[7];
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  
  windowCoordinate[0]=(fTempo[4]*0.5*glContext.zoomX+0.5)*viewport[2]+viewport[0];
  windowCoordinate[1]=(fTempo[5]*0.5*glContext.zoomY+0.5)*viewport[3]+viewport[1];
  windowCoordinate[2]=(1.0+fTempo[6])*0.5;  //Between 0 and 1
  return 1;
}

void glOrtho(float left, float right, float bottom, float top, float znear, float zfar) {
  const double r2 = (right-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  const double l2 = (left-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  const double t2 = (top-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;
  const double b2 = (bottom-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;

  right = (float)r2;
  left = (float)l2;
  top = (float)t2;
  bottom = (float)b2;
  
  double matrix2[16];
  double temp2, temp3, temp4;
  temp2 = (right-left);
  temp3 = top-bottom;
  temp4 = zfar-znear;
  matrix2[0]=2.0/temp2;
  matrix2[1]=0.0;
  matrix2[2]=0.0;
  matrix2[3]=0.0;
  matrix2[4]=0.0;
  matrix2[5]=2.0/temp3;
  matrix2[6]=0.0;
  matrix2[7]=0.0;
  matrix2[8]=0.0;
  matrix2[9]=0.0;
  matrix2[10]=-2.0/temp4;
  matrix2[11]=0.0;
  matrix2[12]=(-right-left)/temp2;
  matrix2[13]=(-top-bottom)/temp3;
  matrix2[14]=(-zfar-znear)/temp4;
  matrix2[15]=1.0;
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  MulMatMatd(matrix, matrix, matrix2);
  glUpdateMatrix();
}

void glFrustum(float left, float right, float bottom, float top, float znear, float zfar) {
  __glFrustum(glContext.matrixForMode[glContext.matrixModeNr],left,right,bottom,top,znear,zfar);
  glUpdateMatrix();
}

void glFrustumf(float left, float right, float bottom, float top, float znear, float zfar) {
  glFrustum(left,right,bottom,top,znear,zfar);
}

void glOrthof(float left, float right, float bottom, float top, float znear, float zfar) {
  glOrtho(left, right, bottom, top, znear, zfar);
}

void gluOrtho2D(float left, float right, float bottom, float top) {
  glOrtho(left, right, bottom, top, -1, 1);
}

void gluPerspective(GLfloat fov, GLfloat aspect, GLfloat nearPlane, GLfloat farPlane) {
  __glPerspective(glContext.matrixForMode[glContext.matrixModeNr],fov,aspect,nearPlane,farPlane);
  glUpdateMatrix();
}

void gluLookAt(GLfloat cx,GLfloat cy,GLfloat cz,GLfloat ox,GLfloat oy,GLfloat oz,GLfloat ux,GLfloat uy,GLfloat uz) {
  double eye[3];
  double center[3];
  double up[3];
  eye[0] = cx;
  eye[1] = cy;
  eye[2] = cz;
  center[0] = ox;
  center[1] = oy;
  center[2] = oz;
  up[0] = ux;
  up[1] = uy;
  up[2] = uz;
  glLookAt(glContext.matrixForMode[glContext.matrixModeNr],eye,center,up);
  glUpdateMatrix();
}

GLint gluProjectfx(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  float d[3];
  const GLint r = glProjectfx(objX, objY, objZ, model, projection, view, d);
  *winX = (float)(d[0]+glPixelCenterX); // actually you need the pixelCenter?
  *winY = (float)(d[1]-glPixelCenterY); // actually you need the pixelCenter?
  *winZ = (float)(d[2]);
  return r;
}

GLint gluProjectf(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  float d[3];
  const GLint r = glProjectf(objX, objY, objZ, model, projection, view, d);
  *winX = (float)(d[0]+glPixelCenterX); // actually you need the pixelCenter?
  *winY = (float)(d[1]-glPixelCenterY); // actually you need the pixelCenter?
  *winZ = (float)(d[2]);
  return r;
}

GLint gluUnProjectf(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *objX, GLfloat *objY, GLfloat *objZ) {
  float d[3];
  winX -= (float)glPixelCenterX; // actually you need the pixelCenter?
  winY += (float)glPixelCenterY; // actually you need the pixelCenter?
  const GLint r = glUnProjectf(winX, winY, winZ, model, projection, view, d);
  *objX = d[0];
  *objY = d[1];
  *objZ = d[2];
  return r;
}

GLint gluProjectx(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  double d[3];
  const GLint r = glProjectx(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  double d[3];
  const GLint r = glProject(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *objX, GLdouble *objY, GLdouble *objZ) {
  double d[3];
  winX -= glPixelCenterX; // actually you need the pixelCenter?
  winY += glPixelCenterY; // actually you need the pixelCenter?
  const GLint r = glUnProject(winX, winY, winZ, model, projection, view, d);
  *objX = d[0];
  *objY = d[1];
  *objZ = d[2];
  return r;
}


// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ---------------------------- Renderer ---------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// -----------
// -----------
// -----------
static bool dontPaint = false;
INLINE double getBary(double x, double y, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  double nx = v2->sy - v1->sy;
  double ny = -(v2->sx - v1->sx);
  double l = (v0->sx-v1->sx)*nx + (v0->sy-v1->sy)*ny;
  if (fabs(l)<0.001) {l = 0.001;dontPaint=true;}
  double p = (x-v1->sx)*nx + (y-v1->sy)*ny;
  return (p/l); // actually it must have been g/l and nx and ny been normalized
}

#define __POLYMINMAX__\
  int pminx,pminy,pmaxx,pmaxy;\
  pminx=(int)FLOOR(v0->sx);\
  pminy=(int)FLOOR(v0->sy);\
  pmaxx=(int)FLOOR(v0->sx);\
  pmaxy=(int)FLOOR(v0->sy);\
  if ((int)FLOOR(v1->sx)<pminx) pminx=(int)FLOOR(v1->sx);\
  if ((int)FLOOR(v1->sy)<pminy) pminy=(int)FLOOR(v1->sy);\
  if ((int)FLOOR(v1->sx)>pmaxx) pmaxx=(int)FLOOR(v1->sx);\
  if ((int)FLOOR(v1->sy)>pmaxy) pmaxy=(int)FLOOR(v1->sy);\
  if ((int)FLOOR(v2->sx)<pminx) pminx=(int)FLOOR(v2->sx);\
  if ((int)FLOOR(v2->sy)<pminy) pminy=(int)FLOOR(v2->sy);\
  if ((int)FLOOR(v2->sx)>pmaxx) pmaxx=(int)FLOOR(v2->sx);\
  if ((int)FLOOR(v2->sy)>pmaxy) pmaxy=(int)FLOOR(v2->sy);\
  pmaxx++;\
  pmaxy++;

#define __POLYCLIP__\
  bool fullyClipped = false;\
  if (pminx<glClipRectX0) pminx=glClipRectX0;\
  if (pminy<glClipRectY0) pminy=glClipRectY0;\
  if (pmaxx<=glClipRectX0) {fullyClipped=true;pmaxx=glClipRectX0;}\
  if (pmaxy<=glClipRectY0) {fullyClipped=true;pmaxy=glClipRectY0;}\
  if (pminx>=glClipRectX1) {fullyClipped=true;pminx=glClipRectX1-1;}\
  if (pminy>=glClipRectY1) {fullyClipped=true;pminy=glClipRectY1-1;}\
  if (pmaxx>glClipRectX1) pmaxx=glClipRectX1;\
  if (pmaxy>glClipRectY1) pmaxy=glClipRectY1;

#define __PAINTPOLYQUAD_BEGINY__\
  int x,y;\
  double bary0,bary1,bary2;\
  double baryAdd0,baryAdd1,baryAdd2;\
  unsigned int*pDest;\
  float *zDest;\
  for (y = pminy;y<pmaxy;y++) {

#define __PAINTPOLYQUAD_INITBARYFORX__\
  bary0 = getBary(pminx,y,v0,v1,v2);\
  bary1 = getBary(pminx,y,v1,v2,v0);\
  bary2 = getBary(pminx,y,v2,v0,v1);\
  baryAdd0 = (getBary(pmaxx,y,v0,v1,v2)-bary0)/(pmaxx-pminx);\
  baryAdd1 = (getBary(pmaxx,y,v1,v2,v0)-bary1)/(pmaxx-pminx);\
  baryAdd2 = (getBary(pmaxx,y,v2,v0,v1)-bary2)/(pmaxx-pminx);

#define __PAINTPOLYQUAD_BEGINX__QUADREGION\
  int dminx = pminx;\
  int dmaxx = pmaxx;\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth];\
  for (x = dminx;x<dmaxx;x++) {

#define __PAINTPOLYQUAD_BEGINX__TRI\
  int dminx = pminx;\
  int dmaxx = pmaxx;\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth];\
  int xp[3]; int xc=0;\
  if ((y>=vk0->sy)&&(y<vk1->sy)) xp[xc++]=(int)((vk1->sx-vk0->sx)*(y-vk0->sy)/(vk1->sy-vk0->sy)+vk0->sx);\
  if ((y>=vk0->sy)&&(y<vk2->sy)) xp[xc++]=(int)((vk2->sx-vk0->sx)*(y-vk0->sy)/(vk2->sy-vk0->sy)+vk0->sx);\
  if ((y>=vk1->sy)&&(y<vk2->sy)) xp[xc++]=(int)((vk2->sx-vk1->sx)*(y-vk1->sy)/(vk2->sy-vk1->sy)+vk1->sx);\
  if (xc == 2) {\
    if (xp[1]<xp[0]) {int k = xp[0]; xp[0] = xp[1]; xp[1] = k;}\
    xp[0]-=2;\
    xp[1]+=2;\
    if (xp[1]<dmaxx) {dmaxx = xp[1];}\
    if (xp[0]>dminx) {\
      int addx = xp[0]-dminx;\
      dminx += addx;\
      pDest += addx;\
      zDest += addx;\
      bary0 += (double)addx * baryAdd0;\
      bary1 += (double)addx * baryAdd1;\
      bary2 += (double)addx * baryAdd2;\
    }\
  }\
  for (x = dminx;x<dmaxx;x++) {
// __TRI has some overcoverage on both ends (starty andor maybe endy, but for subpixel/subtexel maybe that's ok)

#define __PAINTPOLYQUAD_BEGINX__ __PAINTPOLYQUAD_BEGINX__TRI // __PAINTPOLYQUAD_BEGINX__QUADREGION (maybe used instead)

#define __BARY0__(__v__) ((__v__)*bary0)
#define __BARY1__(__v__) ((__v__)*bary1)
#define __BARY2__(__v__) ((__v__)*bary2)
#define __BARY0PADD__(__v__) ((__v__)*(bary0+xbaryAdd0))
#define __BARY1PADD__(__v__) ((__v__)*(bary1+xbaryAdd1))
#define __BARY2PADD__(__v__) ((__v__)*(bary2+xbaryAdd2))

bool glCCW(const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  const float dx0 = (float)(v1->sx - v0->sx);
  const float dy0 = (float)(v1->sy - v0->sy);
  const float dx1 = (float)(v2->sx - v0->sx);
  const float dy1 = (float)(v2->sy - v0->sy);
  return (dx0*dy1 - dy0*dx1)>=0.f;
}

INLINE bool glCheckDepthFunction(float depthBufferValue, float newValue, int depthFunction) {
  switch(depthFunction) {
  case GL_NEVER: {return false;}
  case GL_LESS: {return newValue < depthBufferValue;}
  case GL_EQUAL: {return newValue == depthBufferValue;}
  case GL_LEQUAL: {return newValue <= depthBufferValue;}
  case GL_GREATER: {return newValue > depthBufferValue;}
  case GL_NOTEQUAL: {return newValue != depthBufferValue;}
  case GL_GEQUAL: {return newValue >= depthBufferValue;}
  case GL_ALWAYS: {return true;}
  }
  return true;
}


INLINE int textureWrap(int t, int siz, GLenum filterMode) {
  if ((unsigned int)t<(unsigned int)siz) return t;
  switch(filterMode) {
  case GL_MIRRORED_REPEAT: {
      t = t<0?(siz*2-1-((-t)%(siz*2))):(t%(siz*2));
      if (t < siz) return t;
      return siz - 1 - (t - siz);
    }
  case GL_CLAMP_TO_EDGE: {
      return t<0?0:(t>siz-1?siz-1:t);
    }
  case GL_CLAMP_TO_BORDER: {
      return t<0?-1:(t>siz-1?-1:t);
    }
  }
  return t<0?(siz-1-((-t)%siz)):(t%siz); // GL_REPEAT
}

INLINE unsigned int doBlend(unsigned int dest, unsigned int source, int sourceFunc, int destFunc, unsigned int constantColor, int blendEquation) {

  if ((blendEquation == GL_FUNC_ADD) && (sourceFunc == GL_ONE) && (destFunc == GL_ONE)) {
    // purelay additive speedup for dest = 0xxx00000 and source = 0xffxxxxxx; seems to have no or almost no effect, but whatever
    if ( ((source & 0xff000000)==0xff000000)  && ((dest & 0x00ffffff)==0x00) ) {
      return source;
    }
  }

  int Sr = source & 255;
  int Sg = (source>>8) & 255;
  int Sb = (source>>16) & 255;
  int Sa = (source>>24) & 255;

  int Dr = dest & 255;
  int Dg = (dest>>8) & 255;
  int Db = (dest>>16) & 255;
  int Da = (dest>>24) & 255;

  int Cr = constantColor & 255;
  int Cg = (constantColor>>8) & 255;
  int Cb = (constantColor>>16) & 255;
  int Ca = (constantColor>>24) & 255;

  int sr=0,sg=0,sb=0,sa=0;
  int dr=0,dg=0,db=0,da=0;

  bool fullS = false;
  switch(sourceFunc) {
    case GL_ZERO: {
      sr = 0;
      sg = 0;
      sb = 0;
      sa = 0;
    } break;
    case GL_ONE: {
      sr = 255;
      sg = 255;
      sb = 255;
      sa = 255;
      fullS = true;
    } break;
    case GL_SRC_COLOR: {
      sr = Sr;
      sg = Sg;
      sb = Sb;
      sa = Sa;
    } break;
    case GL_ONE_MINUS_SRC_COLOR: {
      sr = 255-Sr;
      sg = 255-Sg;
      sb = 255-Sb;
      sa = 255-Sa;
    } break;
    case GL_DST_COLOR: {
      sr = Dr;
      sg = Dg;
      sb = Db;
      sa = Da;
    } break;
    case GL_ONE_MINUS_DST_COLOR: {
      sr = 255-Dr;
      sg = 255-Dg;
      sb = 255-Db;
      sa = 255-Da;
    } break;
    case GL_SRC_ALPHA: {
      sr = Sa;
      sg = Sa;
      sb = Sa;
      sa = Sa;
      if (Sa == 255) fullS = true;
    } break;
    case GL_ONE_MINUS_SRC_ALPHA: {
      sr = 255-Sa;
      sg = 255-Sa;
      sb = 255-Sa;
      sa = 255-Sa;
      if (Sa == 0) fullS = true;
    } break;
    case GL_DST_ALPHA: {
      sr = Da;
      sg = Da;
      sb = Da;
      sa = Da;
      if (Da == 255) fullS = true;
    } break;
    case GL_ONE_MINUS_DST_ALPHA: {
      sr = 255-Da;
      sg = 255-Da;
      sb = 255-Da;
      sa = 255-Da;
      if (Da == 0) fullS = true;
    } break;
    case GL_CONSTANT_COLOR: {
      sr = Cr;
      sg = Cg;
      sb = Cb;
      sa = Ca;
    } break;
    case GL_ONE_MINUS_CONSTANT_COLOR: {
      sr = 255-Cr;
      sg = 255-Cg;
      sb = 255-Cb;
      sa = 255-Ca;
    } break;
    case GL_CONSTANT_ALPHA: {
      sr = Ca;
      sg = Ca;
      sb = Ca;
      sa = Ca;
      if (Ca == 255) fullS = true;
    } break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: {
      sr = 255-Ca;
      sg = 255-Ca;
      sb = 255-Ca;
      sa = 255-Ca;
      if (Ca == 0) fullS = true;
    } break;
  }

  bool fullD = false;
  switch(destFunc) {
    case GL_ZERO: {
      dr = 0;
      dg = 0;
      db = 0;
      da = 0;
    } break;
    case GL_ONE: {
      dr = 255;
      dg = 255;
      db = 255;
      da = 255;
      fullD = true;
    } break;
    case GL_SRC_COLOR: {
      dr = Sr;
      dg = Sg;
      db = Sb;
      da = Sa;
    } break;
    case GL_ONE_MINUS_SRC_COLOR: {
      dr = 255-Sr;
      dg = 255-Sg;
      db = 255-Sb;
      da = 255-Sa;
    } break;
    case GL_DST_COLOR: {
      dr = Dr;
      dg = Dg;
      db = Db;
      da = Da;
    } break;
    case GL_ONE_MINUS_DST_COLOR: {
      dr = 255-Dr;
      dg = 255-Dg;
      db = 255-Db;
      da = 255-Da;
    } break;
    case GL_SRC_ALPHA: {
      dr = Sa;
      dg = Sa;
      db = Sa;
      da = Sa;
      if (Sa == 255) fullD = true;
    } break;
    case GL_ONE_MINUS_SRC_ALPHA: {
      dr = 255-Sa;
      dg = 255-Sa;
      db = 255-Sa;
      da = 255-Sa;
      if (Sa == 0) fullD = true;
    } break;
    case GL_DST_ALPHA: {
      dr = Da;
      dg = Da;
      db = Da;
      da = Da;
      if (Da == 255) fullD = true;
    } break;
    case GL_ONE_MINUS_DST_ALPHA: {
      dr = 255-Da;
      dg = 255-Da;
      db = 255-Da;
      da = 255-Da;
      if (Da == 0) fullD = true;
    } break;
    case GL_CONSTANT_COLOR: {
      dr = Cr;
      dg = Cg;
      db = Cb;
      da = Ca;
    } break;
    case GL_ONE_MINUS_CONSTANT_COLOR: {
      dr = 255-Cr;
      dg = 255-Cg;
      db = 255-Cb;
      da = 255-Ca;
    } break;
    case GL_CONSTANT_ALPHA: {
      dr = Ca;
      dg = Ca;
      db = Ca;
      da = Ca;
      if (Ca == 255) fullD = true;
    } break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: {
      dr = 255-Ca;
      dg = 255-Ca;
      db = 255-Ca;
      da = 255-Ca;
      if (Ca == 0) fullD = true;
    } break;
  }

  if (!fullS) {
    Sr = (Sr*sr) DIVQ255;
    Sg = (Sg*sg) DIVQ255;
    Sb = (Sb*sb) DIVQ255;
    Sa = (Sa*sa) DIVQ255;
  }
  if (!fullD) {
    Dr = (Dr*dr) DIVQ255;
    Dg = (Dg*dg) DIVQ255;
    Db = (Db*db) DIVQ255;
    Da = (Da*da) DIVQ255;
  }

  switch(blendEquation) {
  case GL_FUNC_ADD: {
    Sr += Dr;
    Sg += Dg;
    Sb += Db;
    Sa += Da;
    } break;
  case GL_FUNC_SUBTRACT: {
    Sr -= Dr;
    Sg -= Dg;
    Sb -= Db;
    Sa -= Da;
    } break;
  case GL_FUNC_REVERSE_SUBTRACT: {
    Sr = Dr - Sr;
    Sg = Dg - Sg;
    Sb = Db - Sb;
    Sa = Da - Sa;
    } break;
  case GL_MIN: {
    Sr = glMini(Dr,Sr);
    Sg = glMini(Dg,Sg);
    Sb = glMini(Db,Sb);
    Sa = glMini(Da,Sa);
    } break;
  case GL_MAX: {
    Sr = glMaxi(Dr,Sr);
    Sg = glMaxi(Dg,Sg);
    Sb = glMaxi(Db,Sb);
    Sa = glMaxi(Da,Sa);
    } break;
  }
  Sr = glClampi(Sr,0,255);
  Sg = glClampi(Sg,0,255);
  Sb = glClampi(Sb,0,255);
  Sa = glClampi(Sa,0,255);
  return Sr|(Sg<<8)|(Sb<<16)|(Sa<<24);
}

INLINE double __clamp__(double v, double a, double b) {
  return v<a?a:(v>b?b:v);
}

#define __CLIP_FAR_PLANE__ if (v0->sz>1.0&&v1->sz>1.0&&v2->sz>1.0) return;

void glDrawTrianglePrecise(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2) {
  bool forceWrapRepeat = false;

  if (glIsEnabled2(context,GL_CULL_FACE)) {
    if (context->forceNoCull==0) {
      bool backFacing = glCCW(v0,v1,v2);
      if (context->frontFace == GL_CCW) backFacing = !backFacing;
      if (context->cullFaceMode == GL_FRONT && backFacing) return;
      if (context->cullFaceMode == GL_BACK && (!backFacing)) return;
      if (context->cullFaceMode == GL_FRONT_AND_BACK) return;
    }
  }   
  __CLIP_FAR_PLANE__
  __POLYMINMAX__
  int glClipRectX0 = 0;
  int glClipRectY0 = 0;
  int glClipRectX1 = glFrameBufferWidth;
  int glClipRectY1 = glFrameBufferHeight;
  combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,context->viewportX0,context->viewportY0,context->viewportX1,context->viewportY1);
  if (glIsEnabled2(context,GL_SCISSOR_TEST)) {
    combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,context->scissorX0,context->scissorY0,context->scissorX1,context->scissorY1);
  }
  __POLYCLIP__
  if (fullyClipped) 
    return;

  bool stencil = glIsEnabled2(context,GL_STENCIL_TEST); // not supported
  bool blending = glIsEnabled2(context,GL_BLEND);
  bool maskRed = context->maskRed;
  bool maskGreen = context->maskGreen;
  bool maskBlue = context->maskBlue;
  bool maskAlpha = context->maskAlpha;
  bool notMasked = maskRed && maskGreen && maskBlue && maskAlpha;

  double v0w = 1.0/(v0->sw);
  double v1w = 1.0/(v1->sw);
  double v2w = 1.0/(v2->sw);
  double v0z = (v0->sz*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;
  double v1z = (v1->sz*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;
  double v2z = (v2->sz*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;

  glVertex *vk0 = v0;\
  glVertex *vk1 = v1;\
  glVertex *vk2 = v2;\
  if (vk0->sy>vk1->sy) {glVertex *t; t = vk0; vk0 = vk1; vk1 = t;}\
  if (vk0->sy>vk2->sy) {glVertex *t; t = vk0; vk0 = vk2; vk2 = t;}\
  if (vk1->sy>vk2->sy) {glVertex *t; t = vk1; vk1 = vk2; vk2 = t;}\

  glTexture *t = &glTextures[context->boundTextures[context->activeTexture]];
  bool textured = glIsEnabled2(context,GL_TEXTURE_2D) && t->data != NULL && t->name != 0;
  bool filtering = (t->magFilter != GL_NEAREST)&&(t->magFilter != GL_NEAREST_MIPMAP_NEAREST)&&(t->magFilter != GL_NEAREST_MIPMAP_LINEAR);
  float tx0,ty0;
  float tx1,ty1;
  float tx2,ty2;
  unsigned int *tdata0 = NULL;
  unsigned int borderColor = 0xff000000;
  int twidth0;
  int theight0;
  int texEnvMode;
  // texture
  if (textured) {
    twidth0 = t->width;
    theight0 = t->height;
    tdata0 = t->data;
    texEnvMode = t->texEnvMode;

    double scx = glTexelCenterX;
    double scy = glTexelCenterY;
    if (!filtering) {scx = 0; scy = 0;}


    tx0 = (float)v0->textureX;
    ty0 = (float)v0->textureY;
  

    tx1 = (float)v1->textureX;
    ty1 = (float)v1->textureY;

    tx2 = (float)v2->textureX;
    ty2 = (float)v2->textureY;

    if (glIsEnabled2(context,GL_TEXTURE_GEN_S)||glIsEnabled2(context,GL_TEXTURE_GEN_T)) {
      fixSphereMapUV(context, &tx0,&ty0,&tx1,&ty1);
      fixSphereMapUV(context, &tx0,&ty0,&tx2,&ty2);
      forceWrapRepeat = true;
    }

    tx0 = (float)((tx0*twidth0+scx)*v0w);
    ty0 = (float)((ty0*theight0+scy)*v0w);

    tx1 = (float)((tx1*twidth0+scx)*v1w);
    ty1 = (float)((ty1*theight0+scy)*v1w);

    tx2 = (float)((tx2*twidth0+scx)*v2w);
    ty2 = (float)((ty2*theight0+scy)*v2w);

    borderColor = ((int)FLOOR(glClampf(t->borderColorRed*255.f,0.f,255.f)));
    borderColor |= ((int)FLOOR(glClampf(t->borderColorGreen*255.f,0.f,255.f)))<<8;
    borderColor |= ((int)FLOOR(glClampf(t->borderColorBlue*255.f,0.f,255.f)))<<16;
    borderColor |= ((int)FLOOR(glClampf(t->borderColorAlpha*255.f,0.f,255.f)))<<24;

    if ((filtering) && (t->minFilter == GL_NEAREST)||(t->minFilter==GL_NEAREST_MIPMAP_NEAREST)||(t->minFilter==GL_NEAREST_MIPMAP_LINEAR)) {
      const float tw = (float)twidth0;
      const float th = (float)theight0;
      const float lengtht0 = (float)sqrt((v1->textureX-v0->textureX)*(v1->textureX-v0->textureX)*tw*tw+(v1->textureY-v0->textureY)*(v1->textureY-v0->textureY)*th*th);
      const float lengtht1 = (float)sqrt((v2->textureX-v0->textureX)*(v2->textureX-v0->textureX)*tw*tw+(v2->textureY-v0->textureY)*(v2->textureY-v0->textureY)*th*th);
      const float lengtht2 = (float)sqrt((v2->textureX-v1->textureX)*(v2->textureX-v1->textureX)*tw*tw+(v2->textureY-v1->textureY)*(v2->textureY-v1->textureY)*th*th);
      const float lengths0 = (float)sqrt((v1->sx-v0->sx)*(v1->sx-v0->sx)+(v1->sy-v0->sy)*(v1->sy-v0->sy));
      const float lengths1 = (float)sqrt((v2->sx-v0->sx)*(v2->sx-v0->sx)+(v2->sy-v0->sy)*(v2->sy-v0->sy));
      const float lengths2 = (float)sqrt((v2->sx-v1->sx)*(v2->sx-v1->sx)+(v2->sy-v1->sy)*(v2->sy-v1->sy));
      if ((lengths0<lengtht0) && (lengths1<lengtht1) && (lengths2<lengtht2)) {
        filtering = false;
      }
    }
  }
  int blendFuncS = context->blendFuncSFactor;
  int blendFuncD = context->blendFuncDFactor;
  int blendEquation = context->blendEquation;
  unsigned int constantColor = ((int)FLOOR(glClampf(context->blendColorRed*255.f,0.f,255.f)));
  constantColor |= ((int)FLOOR(glClampf(context->blendColorGreen*255.f,0.f,255.f)))<<8;
  constantColor |= ((int)FLOOR(glClampf(context->blendColorBlue*255.f,0.f,255.f)))<<16;
  constantColor |= ((int)FLOOR(glClampf(context->blendColorAlpha*255.f,0.f,255.f)))<<24;
  bool writeDepth = context->depthMask;  
  int depthFunction = context->depthFunc;
  bool depthTest = glIsEnabled2(context,GL_DEPTH_TEST);
  if (depthTest&&(depthFunction==GL_ALWAYS)) depthTest=false;
  int alphaFunction = context->alphaFunc;
  int alphaRef = (int)FLOOR(context->alphaFuncRef*255.f);
  bool alphaTest = glIsEnabled2(context,GL_ALPHA_TEST);

  // rgba
  double r0 = v0->colorRed;
  double g0 = v0->colorGreen;
  double b0 = v0->colorBlue;
  double a0 = v0->colorAlpha;

  double r1 = v1->colorRed;
  double g1 = v1->colorGreen;
  double b1 = v1->colorBlue;
  double a1 = v1->colorAlpha;

  double r2 = v2->colorRed;
  double g2 = v2->colorGreen;
  double b2 = v2->colorBlue;
  double a2 = v2->colorAlpha;

  if (r0 < 0) r0 = 0;
  if (g0 < 0) g0 = 0;
  if (b0 < 0) b0 = 0;
  if (a0 < 0) a0 = 0;
  if (r0 > 1) r0 = 1;
  if (g0 > 1) g0 = 1;
  if (b0 > 1) b0 = 1;
  if (a0 > 1) a0 = 1;

  if (r1 < 0) r1 = 0;
  if (g1 < 0) g1 = 0;
  if (b1 < 0) b1 = 0;
  if (a1 < 0) a1 = 0;
  if (r1 > 1) r1 = 1;
  if (g1 > 1) g1 = 1;
  if (b1 > 1) b1 = 1;
  if (a1 > 1) a1 = 1;

  if (r2 < 0) r2 = 0;
  if (g2 < 0) g2 = 0;
  if (b2 < 0) b2 = 0;
  if (a2 < 0) a2 = 0;
  if (r2 > 1) r2 = 1;
  if (g2 > 1) g2 = 1;
  if (b2 > 1) b2 = 1;
  if (a2 > 1) a2 = 1;

  r0 *= 255.0;
  g0 *= 255.0;
  b0 *= 255.0;
  a0 *= 255.0;

  r1 *= 255.0;
  g1 *= 255.0;
  b1 *= 255.0;
  a1 *= 255.0;

  r2 *= 255.0;
  g2 *= 255.0;
  b2 *= 255.0;
  a2 *= 255.0;

  int r,g,b,a,rf,gf,bf,af;
  bool interpolateColor = true;
  bool modulate = true;
  if ( (int)FLOOR(r0)==(int)FLOOR(r1)&&(int)FLOOR(r1)==(int)FLOOR(r2)
    &&(int)FLOOR(g0)==(int)FLOOR(g1)&&(int)FLOOR(g1)==(int)FLOOR(g2)
    &&(int)FLOOR(b0)==(int)FLOOR(b1)&&(int)FLOOR(b1)==(int)FLOOR(b2)
    &&(int)FLOOR(a0)==(int)FLOOR(a1)&&(int)FLOOR(a1)==(int)FLOOR(a2)) {
    interpolateColor = false;
    rf = (int)r0;
    gf = (int)g0;
    bf = (int)b0;
    af = (int)a0;
    if ((rf == 255)&&(gf == 255)&&(bf == 255)&&(af == 255)) 
      modulate = false;
  }   
   
  r0 *= v0w;
  g0 *= v0w;
  b0 *= v0w;
  a0 *= v0w;

  r1 *= v1w;
  g1 *= v1w;
  b1 *= v1w;
  a1 *= v1w;

  r2 *= v2w;
  g2 *= v2w;
  b2 *= v2w;
  a2 *= v2w;

  double sr0 = v0->additionalSpecularColorRed;
  double sg0 = v0->additionalSpecularColorGreen;
  double sb0 = v0->additionalSpecularColorBlue;

  double sr1 = v1->additionalSpecularColorRed;
  double sg1 = v1->additionalSpecularColorGreen;
  double sb1 = v1->additionalSpecularColorBlue;

  double sr2 = v2->additionalSpecularColorRed;
  double sg2 = v2->additionalSpecularColorGreen;
  double sb2 = v2->additionalSpecularColorBlue;


  const bool fogging = glIsEnabled2(context,GL_FOG);
  double f0=0,f1=0,f2=0;
  if (fogging) {
    if (!context->separateSpecular) {
      sr0 = 0;
      sg0 = 0;
      sb0 = 0;
      sr1 = 0;
      sg1 = 0;
      sb1 = 0;
      sr2 = 0;
      sg2 = 0;
      sb2 = 0;
    }
    double *cm = glContext.matrixForMode[GL_MODELVIEW & 1]; // for eyespace z
    double fz0 = v0->vertexX * cm[0*4+2] + v0->vertexY * cm[1*4+2] + v0->vertexZ * cm[2*4+2] + v0->vertexW * cm[3*4+2];
    double fw0 = v0->vertexX * cm[0*4+3] + v0->vertexY * cm[1*4+3] + v0->vertexZ * cm[2*4+3] + v0->vertexW * cm[3*4+3];
    double fz1 = v1->vertexX * cm[0*4+2] + v1->vertexY * cm[1*4+2] + v1->vertexZ * cm[2*4+2] + v1->vertexW * cm[3*4+2];
    double fw1 = v1->vertexX * cm[0*4+3] + v1->vertexY * cm[1*4+3] + v1->vertexZ * cm[2*4+3] + v1->vertexW * cm[3*4+3];
    double fz2 = v2->vertexX * cm[0*4+2] + v2->vertexY * cm[1*4+2] + v2->vertexZ * cm[2*4+2] + v2->vertexW * cm[3*4+2];
    double fw2 = v2->vertexX * cm[0*4+3] + v2->vertexY * cm[1*4+3] + v2->vertexZ * cm[2*4+3] + v2->vertexW * cm[3*4+3];
    if (fw0!=0.0) fz0/=fw0;
    if (fw1!=0.0) fz1/=fw1;
    if (fw2!=0.0) fz2/=fw2;
    fz0 = fabs(fz0);
    fz1 = fabs(fz1);
    fz2 = fabs(fz2);
    if (context->fogMode==GL_LINEAR) {
      double fogLength = context->fogEnd-context->fogStart;
      if (fogLength!=0.0) {
        f0 = (context->fogEnd-fz0)/fogLength;
        f1 = (context->fogEnd-fz1)/fogLength;
        f2 = (context->fogEnd-fz2)/fogLength;
      }
    }
    if (context->fogMode==GL_EXP) {
      f0 = exp(-context->fogDensity*fz0);
      f1 = exp(-context->fogDensity*fz1);
      f2 = exp(-context->fogDensity*fz2);
    }
    if (context->fogMode==GL_EXP2) {
      f0 = context->fogDensity*fz0;
      f1 = context->fogDensity*fz1;
      f2 = context->fogDensity*fz2;
      f0 = exp(-f0*f0);
      f1 = exp(-f1*f1);
      f2 = exp(-f2*f2);
    }
    f0 = 1.0-__clamp__(f0,0.0,1.0);
    f1 =  1.0-__clamp__(f1,0.0,1.0);
    f2 = 1.0-__clamp__(f2,0.0,1.0);
    sr0 = sr0*(1-f0)+context->fogColor[0]*f0;
    sg0 = sg0*(1-f0)+context->fogColor[1]*f0;
    sb0 = sb0*(1-f0)+context->fogColor[2]*f0;
    sr1 = sr1*(1-f1)+context->fogColor[0]*f1;
    sg1 = sg1*(1-f1)+context->fogColor[1]*f1;
    sb1 = sb1*(1-f1)+context->fogColor[2]*f1;
    sr2 = sr2*(1-f2)+context->fogColor[0]*f2;
    sg2 = sg2*(1-f2)+context->fogColor[1]*f2;
    sb2 = sb2*(1-f2)+context->fogColor[2]*f2;
    f0 = 1 - f0;
    f1 = 1 - f1;
    f2 = 1 - f2;
    if (f0<1.0/256.0&&f1<1.0/256.0&&f2<1.0/256.0) {textured = false; interpolateColor = false;}
    f0 *= v0w; // fadeout colors by this
    f1 *= v1w;
    f2 *= v2w;
  }


  int srf,sgf,sbf;

  const bool separateSpecular = context->separateSpecular||fogging;
  bool interpolateSpecular = true;
  if (separateSpecular) {
    if (sr0 < 0) sr0 = 0;
    if (sg0 < 0) sg0 = 0;
    if (sb0 < 0) sb0 = 0;
    if (sr0 > 1) sr0 = 1;
    if (sg0 > 1) sg0 = 1;
    if (sb0 > 1) sb0 = 1;
  
    if (sr1 < 0) sr1 = 0;
    if (sg1 < 0) sg1 = 0;
    if (sb1 < 0) sb1 = 0;
    if (sr1 > 1) sr1 = 1;
    if (sg1 > 1) sg1 = 1;
    if (sb1 > 1) sb1 = 1;
  
    if (sr2 < 0) sr2 = 0;
    if (sg2 < 0) sg2 = 0;
    if (sb2 < 0) sb2 = 0;
    if (sr2 > 1) sr2 = 1;
    if (sg2 > 1) sg2 = 1;
    if (sb2 > 1) sb2 = 1;
  
    sr0 *= 255.0;
    sg0 *= 255.0;
    sb0 *= 255.0;
  
    sr1 *= 255.0;
    sg1 *= 255.0;
    sb1 *= 255.0;
  
    sr2 *= 255.0;
    sg2 *= 255.0;
    sb2 *= 255.0;
  
    if ( (int)FLOOR(sr0)==(int)FLOOR(sr1)&&(int)FLOOR(sr1)==(int)FLOOR(sr2)
      &&(int)FLOOR(sg0)==(int)FLOOR(sg1)&&(int)FLOOR(sg1)==(int)FLOOR(sg2)
      &&(int)FLOOR(sb0)==(int)FLOOR(sb1)&&(int)FLOOR(sb1)==(int)FLOOR(sb2)) {
      interpolateSpecular = false;
      srf = (int)sr0;
      sgf = (int)sg0;
      sbf = (int)sb0;
    }   
     
    sr0 *= v0w;
    sg0 *= v0w;
    sb0 *= v0w;
  
    sr1 *= v1w;
    sg1 *= v1w;
    sb1 *= v1w;
  
    sr2 *= v2w;
    sg2 *= v2w;
    sb2 *= v2w;
  }

  double iw = 0;
  const bool wValue = textured||interpolateColor;
  const bool normalAlphaBlending = blendFuncS == GL_SRC_ALPHA && blendFuncD == GL_ONE_MINUS_SRC_ALPHA && blendEquation == GL_FUNC_ADD;
  const bool preMultipliedAlpha = blendFuncS == GL_ONE && blendFuncD == GL_ONE_MINUS_SRC_ALPHA && blendEquation == GL_FUNC_ADD;
  const bool normalAlphaBlendingOrPreMultipliedAlpha = normalAlphaBlending || preMultipliedAlpha;
  const bool useExplicitAlpha = context->useExplicitAlpha;
  const bool alphaSolelyOpacity = blending&&(normalAlphaBlending||(blendFuncS == GL_SRC_ALPHA && blendFuncD == GL_ONE && blendEquation == GL_FUNC_ADD));
  const bool colorSolelyPossible = (texEnvMode==GL_MODULATE)||(texEnvMode==GL_ADD)||(texEnvMode==GL_REPLACE);
  const bool colorSolelyOpacity = colorSolelyPossible&&blending&&(blendFuncS == GL_ONE && blendFuncD == GL_ONE && blendEquation == GL_FUNC_ADD);

  int explicitAlphaValue = (int)FLOOR(context->explicitAlpha * 255.f);
  if (explicitAlphaValue<0) explicitAlphaValue=0;
  if (explicitAlphaValue>255) explicitAlphaValue=255;
  unsigned char eAlpha = (unsigned char)explicitAlphaValue;

  const unsigned int wrapS = forceWrapRepeat ? GL_REPEAT : t->wrapS;
  const unsigned int wrapT = forceWrapRepeat ? GL_REPEAT : t->wrapT;
  int tpx,tpy,tpxa,tpya;

  dontPaint = false;
  __PAINTPOLYQUAD_BEGINY__
  __PAINTPOLYQUAD_INITBARYFORX__
  if (dontPaint) return;
#ifdef __FASTTEXTURING__
  bool fastTexturing = glFastTexturing;
  int fastApprox = 0;
  double xbaryAdd0 = baryAdd0*glFastTextureSpanWidth;
  double xbaryAdd1 = baryAdd1*glFastTextureSpanWidth;
  double xbaryAdd2 = baryAdd2*glFastTextureSpanWidth;
#endif // __FASTTEXTURING__
  __PAINTPOLYQUAD_BEGINX__
      if (bary0>=0&&bary1>=0&&bary2>=0) {
        double zp = __BARY0__(v0z)+__BARY1__(v1z)+__BARY2__(v2z);
        if ((!depthTest)||glCheckDepthFunction(*zDest,(float)zp,depthFunction)) {
          if (wValue) iw = 1.0/(__BARY0__(v0w)+__BARY1__(v1w)+__BARY2__(v2w));
          unsigned int rgba;
          bool writePixel = true;
          bool writePixel2 = true;
          if (textured) {
            if (filtering) {
#ifdef __FASTTEXTURING__
              if (fastTexturing) {
                if (fastApprox==0) {
                  fastApprox=glFastTextureSpanWidth;
                  double iw2=iw*0x10000;
                  tpx = (int)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw2);
                  tpy = (int)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw2);
                  if (wValue) iw2 = (1.0/(__BARY0PADD__(v0w)+__BARY1PADD__(v1w)+__BARY2PADD__(v2w)))*0x10000;
                  int k = glFastTextureSpanWidth;
                  tpxa = ((int)FLOOR((__BARY0PADD__(tx0)+__BARY1PADD__(tx1)+__BARY2PADD__(tx2))*iw2)-tpx)/k;
                  tpya = ((int)FLOOR((__BARY0PADD__(ty0)+__BARY1PADD__(ty1)+__BARY2PADD__(ty2))*iw2)-tpy)/k;
                  if (k+x>=dmaxx) fastTexturing = false; // this is maybe not the triangle edge
                }
                const int kx = tpx>>16;
                const int ky = tpy>>16;
                int tix0 = textureWrap(kx, twidth0, wrapS);
                int tiy0 = textureWrap(ky, theight0, wrapT);
                const int tix1 = textureWrap(kx+1, twidth0, wrapS);
                int tiy1 = textureWrap(ky+1, theight0, wrapT);
                const int txf = (tpx>>8)&255;
                const int tyf = (tpy>>8)&255;
                const int p1v = ((256-txf)*(256-tyf))>>8; 
                const int p2v = ((txf)*(256-tyf))>>8; 
                const int p3v = ((txf)*(tyf))>>8; 
                const int p4v = ((256-txf)*(tyf))>>8;
                tiy0 *= twidth0;
                tiy1 *= twidth0;
                const unsigned int rgba00 = (tix0|tiy0)>=0?tdata0[tix0+tiy0]:borderColor;
                const unsigned int rgba10 = (tix1|tiy0)>=0?tdata0[tix1+tiy0]:borderColor;
                const unsigned int rgba11 = (tix1|tiy1)>=0?tdata0[tix1+tiy1]:borderColor;
                const unsigned int rgba01 = (tix0|tiy1)>=0?tdata0[tix0+tiy1]:borderColor;
                rgba = (((rgba00>>8) & 0x00ff00ff)*p1v)&0xff00ff00;
                rgba += (((rgba10>>8) & 0x00ff00ff)*p2v)&0xff00ff00;
                rgba += (((rgba11>>8) & 0x00ff00ff)*p3v)&0xff00ff00;
                rgba += (((rgba01>>8) & 0x00ff00ff)*p4v)&0xff00ff00;
                rgba += (((rgba00 & 0x00ff00ff)*p1v)>>8)&0x00ff00ff;
                rgba += (((rgba10 & 0x00ff00ff)*p2v)>>8)&0x00ff00ff;
                rgba += (((rgba11 & 0x00ff00ff)*p3v)>>8)&0x00ff00ff;
                rgba += (((rgba01 & 0x00ff00ff)*p4v)>>8)&0x00ff00ff;
                tpx+=tpxa;
                tpy+=tpya;
                fastApprox--;
              } else {
#endif // __FASTTEXTURING__
                const float tx = (float)((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw);
                const float ty = (float)((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw);
                const int tix0 = textureWrap((int)FLOOR(tx), twidth0, wrapS);
                int tiy0 = textureWrap((int)FLOOR(ty), theight0, wrapT);
                const int tix1 = textureWrap((int)FLOOR(tx)+1, twidth0, wrapS);
                int tiy1 = textureWrap((int)FLOOR(ty)+1, theight0, wrapT);
                int txf = (int)FLOOR(tx*256.f) & 255;
                int tyf = (int)FLOOR(ty*256.f) & 255;
                const int p1v = ((256-txf)*(256-tyf))>>8; 
                const int p2v = ((txf)*(256-tyf))>>8; 
                const int p3v = ((txf)*(tyf))>>8; 
                const int p4v = ((256-txf)*(tyf))>>8;
                tiy0 *= twidth0;
                tiy1 *= twidth0;
                const unsigned int rgba00 = (tix0|tiy0)>=0?tdata0[tix0+tiy0]:borderColor;
                const unsigned int rgba10 = (tix1|tiy0)>=0?tdata0[tix1+tiy0]:borderColor;
                const unsigned int rgba11 = (tix1|tiy1)>=0?tdata0[tix1+tiy1]:borderColor;
                const unsigned int rgba01 = (tix0|tiy1)>=0?tdata0[tix0+tiy1]:borderColor;
                rgba = (((rgba00>>8) & 0x00ff00ff)*p1v)&0xff00ff00;
                rgba += (((rgba10>>8) & 0x00ff00ff)*p2v)&0xff00ff00;
                rgba += (((rgba11>>8) & 0x00ff00ff)*p3v)&0xff00ff00;
                rgba += (((rgba01>>8) & 0x00ff00ff)*p4v)&0xff00ff00;
                rgba += (((rgba00 & 0x00ff00ff)*p1v)>>8)&0x00ff00ff;
                rgba += (((rgba10 & 0x00ff00ff)*p2v)>>8)&0x00ff00ff;
                rgba += (((rgba11 & 0x00ff00ff)*p3v)>>8)&0x00ff00ff;
                rgba += (((rgba01 & 0x00ff00ff)*p4v)>>8)&0x00ff00ff;
#ifdef __FASTTEXTURING__
              }
#endif // __FASTTEXTURING__
            } else {
#ifdef __FASTTEXTURING__
              if (fastTexturing) {
                if (fastApprox==0) {
                  fastApprox=glFastTextureSpanWidth;
                  double iw2=iw*0x10000;
                  tpx = (int)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw2);
                  tpy = (int)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw2);
                  if (wValue) iw2 = (1.0/(__BARY0PADD__(v0w)+__BARY1PADD__(v1w)+__BARY2PADD__(v2w)))*0x10000;
                  int k = glFastTextureSpanWidth;
                  tpxa = ((int)FLOOR((__BARY0PADD__(tx0)+__BARY1PADD__(tx1)+__BARY2PADD__(tx2))*iw2)-tpx)/k;
                  tpya = ((int)FLOOR((__BARY0PADD__(ty0)+__BARY1PADD__(ty1)+__BARY2PADD__(ty2))*iw2)-tpy)/k;
                  if (k+x>=dmaxx) fastTexturing = false; // this is maybe not the triangle edge
                }
                int tix0 = textureWrap(tpx>>16, twidth0, wrapS);
                int tiy0 = textureWrap(tpy>>16, theight0, wrapT);
                if ((int)(tix0|tiy0)>=0) {
                  rgba = tdata0[tix0+tiy0*twidth0];
                } else {
                  rgba = borderColor;
                }
                tpx+=tpxa;
                tpy+=tpya;
                fastApprox--;
              } else {
#endif // __FASTTEXTURING__
                int tix0 = textureWrap((int)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw), twidth0, wrapS);
                int tiy0 = textureWrap((int)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw), theight0, wrapT);
                if ((int)(tix0|tiy0)>=0) {
                  rgba = tdata0[tix0+tiy0*twidth0];
                } else {
                  rgba = borderColor;
                }
#ifdef __FASTTEXTURING__
              }
#endif // __FASTTEXTURING__
            }
            if (alphaSolelyOpacity) writePixel2 = (rgba&0xff000000)!=0;
            if (colorSolelyOpacity) writePixel2 = rgba!=0; // alpha channel has to be 0,too for optimized additive transparency
          }
          
          if (writePixel2) {
            if (interpolateColor) {
              r = (int)((__BARY0__(r0)+__BARY1__(r1)+__BARY2__(r2))*iw);
              g = (int)((__BARY0__(g0)+__BARY1__(g1)+__BARY2__(g2))*iw);
              b = (int)((__BARY0__(b0)+__BARY1__(b1)+__BARY2__(b2))*iw);
              a = (int)((__BARY0__(a0)+__BARY1__(a1)+__BARY2__(a2))*iw);
            } else {
              r = rf;
              g = gf;
              b = bf;
              a = af;
            }
  
            if (textured) {
              if (texEnvMode==GL_MODULATE) {
                if (modulate) {
                  r *= rgba & 255;
                  g *= (rgba >> 8) & 255;
                  b *= (rgba >> 16) & 255;
                  a *= (rgba >> 24) & 255;
                  r DIVE255;
                  g DIVE255;
                  b DIVE255;
                  a DIVE255;
                } else {
                  r = rgba & 255;
                  g = (rgba >> 8) & 255;
                  b = (rgba >> 16) & 255;
                  a = (rgba >> 24) & 255;
                }
              } else {
                switch(texEnvMode) {
                  case GL_REPLACE: {
                    r = rgba & 255;
                    g = (rgba >> 8) & 255;
                    b = (rgba >> 16) & 255;
                  } break;
                  case GL_DECAL: {
                    const int as = ((rgba >> 24)&255);
                    r = r*(255-as)+(rgba & 255)*as;
                    g = g*(255-as)+((rgba>>8) & 255)*as;
                    b = b*(255-as)+((rgba>>16) & 255)*as;
                    r DIVE255;
                    g DIVE255;
                    b DIVE255;
                  } break;
                  case GL_ADD: {
                    const int as = ((rgba >> 24)&255);
                    r += rgba & 255;
                    g += (rgba >> 8) & 255;
                    b += (rgba >> 16) & 255;
                    a = (a*as)>>8;
                    if (r > 255) r = 255;
                    if (g > 255) g = 255;
                    if (b > 255) b = 255;
                  } break;
                  default: {//case GL_MODULATE: {
                    r *= rgba & 255;
                    g *= (rgba >> 8) & 255;
                    b *= (rgba >> 16) & 255;
                    a *= (rgba >> 24) & 255;
                    r DIVE255;
                    g DIVE255;
                    b DIVE255;
                    a DIVE255;
                  } break;
                }
              }
            }
  
            if (alphaTest) {
              switch(alphaFunction) {
              case GL_NEVER: {writePixel = false;} break;
              case GL_LESS: {writePixel = a<alphaRef;} break;
              case GL_EQUAL: {writePixel = a==alphaRef;} break;
              case GL_LEQUAL: {writePixel = a<=alphaRef;} break;
              case GL_GREATER: {writePixel = a>alphaRef;} break;
              case GL_NOTEQUAL: {writePixel = a!=alphaRef;} break;
              case GL_GEQUAL: {writePixel = a>=alphaRef;} break;
              case GL_ALWAYS: {writePixel = true;} break;
              }
            }
  
            if (separateSpecular) {
              if (fogging) {
                const double f = (__BARY0__(f0)+__BARY1__(f1)+__BARY2__(f2))*iw;
                r = (int)(r*f);
                g = (int)(g*f);
                b = (int)(b*f);
              }
              if (interpolateSpecular) {
                r += (int)((__BARY0__(sr0)+__BARY1__(sr1)+__BARY2__(sr2))*iw);
                g += (int)((__BARY0__(sg0)+__BARY1__(sg1)+__BARY2__(sg2))*iw);
                b += (int)((__BARY0__(sb0)+__BARY1__(sb1)+__BARY2__(sb2))*iw);
              } else {
                r += srf;
                g += sgf;
                b += sbf;
              }
              if (r>255) r = 255;
              if (g>255) g = 255;
              if (b>255) b = 255;
            }
          } else {
            if (alphaTest) {
              switch(alphaFunction) {
              case GL_NEVER: {writePixel = false;} break;
              case GL_LESS: {writePixel = 0<alphaRef;} break;
              case GL_EQUAL: {writePixel = 0==alphaRef;} break;
              case GL_LEQUAL: {writePixel = 0<=alphaRef;} break;
              case GL_GREATER: {writePixel = 0>alphaRef;} break;
              case GL_NOTEQUAL: {writePixel = 0!=alphaRef;} break;
              case GL_GEQUAL: {writePixel = 0>=alphaRef;} break;
              case GL_ALWAYS: {writePixel = true;} break;
              }
            }
          }

          if (writePixel) {
            if (writeDepth) 
              *zDest=(float)zp;
            if (!blending)
              if (notMasked) {
                *pDest=r|(g<<8)|(b<<16)|(a<<24);
              } else {
                unsigned char *pDest2 = (unsigned char *)pDest;
                if (maskRed) pDest2[0] = (unsigned char)r;
                if (maskGreen) pDest2[1] = (unsigned char)g;
                if (maskBlue) pDest2[2] = (unsigned char)b;
                if (maskAlpha) pDest2[3] = (unsigned char)a;
              }
            else {
              if (writePixel2) {
                if (notMasked) {
                  if (normalAlphaBlendingOrPreMultipliedAlpha) {
                    unsigned int a8 = (a<<16)/255;
                    unsigned char *c = (unsigned char*)pDest;
                    if (preMultipliedAlpha) {
                      a8 = 0x10000-a8;
                      r += (c[0]*a8)>>16;
                      g += (c[1]*a8)>>16;
                      b += (c[2]*a8)>>16;
                      a += (c[3]*a8)>>16;
                      if (r>255) r = 255;
                      if (g>255) g = 255;
                      if (b>255) b = 255;
                      if (a>255) a = 255;
                      *pDest=r|(g<<8)|(b<<16)|(a<<24);
                    } else {
                      c[0] = (unsigned char)(c[0] + (((r-c[0])*a8)>>16));
                      c[1] = (unsigned char)(c[1] + (((g-c[1])*a8)>>16));
                      c[2] = (unsigned char)(c[2] + (((b-c[2])*a8)>>16));
                      c[3] = (unsigned char)(c[3] + (((a-c[3])*a8)>>16));
                    }
                  } else {
                    *pDest=doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                  }
                } else {
                  unsigned int k = doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                  unsigned char *pDest2 = (unsigned char *)pDest;
                  unsigned char *k2 = (unsigned char*)&k;
                  if (maskRed) pDest2[0] = k2[0];
                  if (maskGreen) pDest2[1] = k2[1];
                  if (maskBlue) pDest2[2] = k2[2];
                  if (maskAlpha) pDest2[3] = k2[3];
                }
              }
            }
            if (useExplicitAlpha) {
              ((unsigned char *)pDest)[3] = eAlpha;
            }
          }
        }
      }
      pDest++;
      zDest++;
      bary0+=baryAdd0;
      bary1+=baryAdd1;
      bary2+=baryAdd2;
    }
  }
}

void glDrawQuad(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3) {
  //glVertices[3].sx -= pointSizeX;
  //glVertices[3].sy -= pointSizeY;
  //glVertices[2].sx += pointSizeX;
  //glVertices[2].sy -= pointSizeY;
  //glVertices[1].sx += pointSizeX;
  //glVertices[1].sy += pointSizeY;
  //glVertices[0].sx -= pointSizeX;
  //glVertices[0].sy += pointSizeY;
  //01
  //32
  glDrawTriangle(context,v0,v1,v2);
  glDrawTriangle(context,v2,v3,v0);
}

bool noRefresh = false;
void glSetupMouse();

#ifndef __GLDISABLEDOSFUNCTIONS__
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ---------------------------- VESA -------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// -----------
// -----------
// -----------
#ifdef __DJGPP__
#define DJGPPVESA 1
#endif

extern unsigned short glGraphicsModeToRestore;
unsigned short getBiosGraphicsMode();
bool glVesaBGRA = true;

// --------------------------------------
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
// --------------------------------------
#pragma pack(push)
#pragma pack(1)
typedef struct glVbeInfoBlock {
  char signature[4];
  uint16_t version;
  char *oemString;
  uint32_t capabilities;
  uint16_t *videoModes;
  uint16_t totalMemory;
  uint16_t softwareRev;
  char *vendor;
  char *productName;
  char *productRev;
  char reserved1[222];
  char oemData[256];
} glVbeInfoBlock;
// --------------------------------------
typedef struct glVbeModeInfo {
  uint16_t attributes;
  uint8_t windowA;
  uint8_t windowB;
  uint16_t granularity;
  uint16_t windowSize;
  uint16_t segmentA;
  uint16_t segmentB;
  uint32_t winFuncPtr;
  uint16_t pitch;
  uint16_t width;
  uint16_t height;
  uint8_t wChar;
  uint8_t yChar;
  uint8_t planes;
  uint8_t bpp;
  uint8_t banks;
  uint8_t memoryModel;
  uint8_t bankSize;
  uint8_t imagePages;
  uint8_t reserved0;
  uint8_t redMask;
  uint8_t redPosition;
  uint8_t greenMask;
  uint8_t greenPosition;
  uint8_t blueMask;
  uint8_t bluePosition;
  uint8_t reservedMask;
  uint8_t reservedPosition;
  uint8_t directColorModeInfo;
  uint32_t linearFrameBuffer;
  uint32_t offScreenMemOffset;
  uint16_t offScreenMemSize;
  uint8_t reserved1[256-50];
} glVbeModeInfo;
#pragma pack(pop)

// --------------------------------------
// ---- DPMI for VESA ---
// --------------------------------------

// --------------------------------------
#ifndef DJGPPVESA
// --------------------------------------

#include <i86.h>
#define DJGPPVGAMEMON
#define DJGPPVGAMEMOFF
#define VGARAMBASE (0xa0000)

#include <conio.h>
#include <stdio.h>
typedef struct {
  short di;
  short si;
  short bp;
  short reserved;
  short bx;
  short dx;
  short cx;
  short ax;
  short flags;
  short es,ds,fs,gs,ip,cs,sp,ss;
  int cflag;
} RMREGS;
typedef struct {
  long edi;
  long esi;
  long ebp;
  long reserved;
  long ebx;
  long edx;
  long ecx;
  long eax;
  short flags;
  short es,ds,fs,gs,ip,cs,sp,ss;
} _RMREGS;
typedef struct {
  short es,ds,fs,gs,ip,cs,sp,ss;
} RMSREGS;

#define IN(reg) rmregs.e##reg = in->##reg
#define OUT(reg) out->##reg = (unsigned short)rmregs.e##reg
void *glVesaPointer(const void *d) {
  int segment = ((unsigned int)d) >> 16;
  int offset = ((unsigned int)d) & 0xffff;
  return (void*)(((unsigned int)segment<<4)+offset);
}
int VESABUF_sel=0x00;
int VESABUF_rseg=0x00;
int VESABUF_len=1024;
void DPMI_allocRealSeg(int size, int *sel, int *r_seg) {
  union REGS r;
  r.w.ax = 0x100;
  r.w.bx = (unsigned short)((size+0x0f)>>4);
  int386(0x31,&r,&r);
  if (r.w.cflag)
    exit(0);
  *sel = r.w.dx;
  *r_seg = r.w.ax;
}
void DPMI_freeRealSeg(unsigned short sel) {
  union REGS r;
  r.w.ax = 0x101;
  r.w.dx = (unsigned short)sel;
  int386(0x31,&r,&r);
}
static void ExitVBEBuf() {
  DPMI_freeRealSeg((unsigned short)VESABUF_sel);
}
short DPMI_int386(int intno, RMREGS *in, RMREGS *out) {
  _RMREGS rmregs;
  union REGS r;
  struct SREGS sr;

  memset(&rmregs,0,sizeof(rmregs));
  IN(ax); IN(bx); IN(cx); IN(dx); IN(si); IN(di);

  segread(&sr);
  r.w.ax = 0x300;
  r.h.bl = (unsigned char)intno;
  r.h.bh = 0;
  r.w.cx = 0;
  sr.es = sr.ds;
  r.x.edi = (unsigned)&rmregs;
  int386x(0x31,&r,&r,&sr);

  OUT(ax); OUT(bx); OUT(cx); OUT(dx); OUT(si); OUT(di);
  out->cflag = rmregs.flags & 0x01;
  return out->ax;
}
short DPMI_int386x(int intno, RMREGS *in, RMREGS *out, RMSREGS *sregs) {
  _RMREGS rmregs;
  union REGS r;
  struct SREGS sr;

  memset(&rmregs,0,sizeof(rmregs));
  IN(ax); IN(bx); IN(cx); IN(dx); IN(si); IN(di);
  rmregs.es = sregs->es;
  rmregs.ds = sregs->ds;

  segread(&sr);
  r.w.ax = 0x300;
  r.h.bl = (unsigned char)intno;
  r.h.bh = 0;
  r.w.cx = 0;
  sr.es = sr.ds;
  r.x.edi = (unsigned)&rmregs;
  int386x(0x31,&r,&r,&sr);

  OUT(ax); OUT(bx); OUT(cx); OUT(dx); OUT(si); OUT(di);
  sregs->es = rmregs.es;
  sregs->cs = rmregs.cs;
  sregs->ss = rmregs.ss;
  sregs->ds = rmregs.ds;
  out->cflag = rmregs.flags & 0x01;
  return out->ax;
}
void VBE_initRMBuf() {
  if (!VESABUF_sel) {
    DPMI_allocRealSeg(VESABUF_len,&VESABUF_sel,&VESABUF_rseg);
    atexit(ExitVBEBuf);
  }
}
void VBE_callESDI(RMREGS *regs, void *buffer, int size) {
  RMSREGS sregs;
  VBE_initRMBuf();
  sregs.es = (unsigned short)VESABUF_rseg;
  regs->di = 0;
  _fmemcpy(MK_FP(VESABUF_sel,0),buffer,size);
  DPMI_int386x(0x10,regs,regs,&sregs);
  _fmemcpy(buffer,MK_FP(VESABUF_sel,0),size);
  //ExitVBEBuf();
}
uint32_t mapPhysicalToLinear(unsigned int physicalAddress, unsigned int size) {
  union REGS r;
  r.w.ax = 0x800;
  r.w.bx = (unsigned short)(physicalAddress>>16);
  r.w.cx = (unsigned short)(physicalAddress & 0xffff);
  r.w.si = (unsigned short)(size >> 16);
  r.w.di = (unsigned short)(size & 0xffff);
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return 0;
  return (((uint32_t)r.w.bx)<<16)+r.w.cx;
}
bool unmapPhysical(unsigned int linearAddress) {
  union REGS r;
  r.w.ax = 0x801;
  r.w.bx = (unsigned short)(linearAddress>>16);
  r.w.cx = (unsigned short)(linearAddress & 0xffff);
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return false;
  return true;
}
// --------------------------------------
// --------------------------------------
// --------------------------------------
bool glVesa(int xRes,int yRes, int bPP) {
  glGraphicsModeToRestore = getBiosGraphicsMode();
  glVbeInfoBlock vbeInfo;
  glVbeModeInfo modeInfo;
  RMREGS regs;
  regs.ax = 0x4f00;
  VBE_callESDI(&regs,&vbeInfo,sizeof(glVbeInfoBlock));
  if (regs.ax != 0x004f) return false;
  uint16_t *u = (uint16_t *)glVesaPointer(vbeInfo.videoModes);
  uint16_t *v = u;
  while(*v != 0xffff) {v++;}
  uint16_t *w=(uint16_t*)malloc((v-u)*sizeof(uint16_t));
  memcpy(w,u,((unsigned int)v)-((unsigned int)u));
  v = w;
  while(*v != 0xffff) {
    int mode = *v;
    regs.ax = 0x4f01;
    regs.cx = (unsigned short)mode;
    VBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP) {
      construtGL();
      regs.ax = 0x4f02;
      regs.bx = (unsigned short)(mode|0x4000);
      DPMI_int386(0x10,&regs,&regs); // needed here for later mouse setup
      glVesaBGRA = (modeInfo.bluePosition==0);
      glFrameBufferWidth = xRes;
      glFrameBufferHeight = yRes;
      glFrameBufferDedicated = (unsigned int*)mapPhysicalToLinear(modeInfo.linearFrameBuffer,4096*1024-1);
      glFrameBuffer = (unsigned int *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
      glDepthBuffer = (float *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(float));
      memset(glFrameBufferDedicated,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      glFrameBufferWidth0 = glFrameBufferWidth;
      glFrameBufferHeight0 = glFrameBufferHeight;
      glFrameBuffer0 = glFrameBuffer;
      glDepthBuffer0 = glDepthBuffer;
      glSetupMouse();
      glSetTime(0);
      glViewport(0,0,xRes,yRes);
      free(w);
      noRefresh = false;
      return true;
    }
    v++;
  }
  free(w);
  return false;
}
// --------------------------------------
#else // DJGPPVESA
// --------------------------------------

#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#define VGARAMBASE (0xa0000+__djgpp_conventional_base)
#define DJGPPVGAMEMON if (useNearPointers) __djgpp_nearptr_enable();
#define DJGPPVGAMEMOFF if (useNearPointers) __djgpp_nearptr_disable();

int VESABUF_len=1024; // length of struct to work on with realmode es:di
bool VESABUF_sel_init = false; // we need VESABUF_sel just once
__dpmi_meminfo linearFrameBufferStruct; // physical to linear address mapping for linear framebuffer
_go32_dpmi_seginfo VESABUF_sel; // struct to work on with realmode es:di
int vesasel; // final linear framebuffer accessor (_farpokel)

bool unmapPhysical(unsigned int linearAddress) {
  __dpmi_free_physical_address_mapping(&linearFrameBufferStruct);
  __dpmi_free_ldt_descriptor(vesasel);
  return true;
}

static void ExitVBEBuf() {
  _go32_dpmi_free_dos_memory(&VESABUF_sel); // called at exit of app
}

void VBE_callESDI(__dpmi_regs *regs, void *buffer, int size) {
  if (!VESABUF_sel_init) {
    VESABUF_sel_init = true;
    VESABUF_sel.size = (VESABUF_len+15)/16;
    _go32_dpmi_allocate_dos_memory(&VESABUF_sel);
    atexit(ExitVBEBuf);
  }
  regs->x.es = VESABUF_sel.rm_segment;
  regs->x.di = VESABUF_sel.rm_offset;
  unsigned long addr; // 32 bit
  __dpmi_get_segment_base_address(VESABUF_sel.pm_selector, &addr);
  DJGPPVGAMEMON
  void *ptr = (void*)(addr+__djgpp_conventional_base);
  memcpy(ptr,buffer,size);
  __dpmi_simulate_real_mode_interrupt(0x10, regs);
  memcpy(buffer,ptr,size);
  //printf("%c%c%c%c\n",((char*)buffer)[0],((char*)buffer)[1],((char*)buffer)[2],((char*)buffer)[3]);
  DJGPPVGAMEMOFF
}
bool glVesa(int xRes,int yRes, int bPP) {
  glGraphicsModeToRestore = getBiosGraphicsMode();
  glVbeInfoBlock vbeInfo;
  glVbeModeInfo modeInfo;
  __dpmi_regs regs;
  memset(&regs,0,sizeof(__dpmi_regs));
  regs.x.ax = 0x4f00;
  VBE_callESDI(&regs,&vbeInfo,sizeof(glVbeInfoBlock));
  if (regs.x.ax != 0x004f) return false;

  unsigned int videoModes = (((unsigned int)vbeInfo.videoModes)>>16<<4)+(((unsigned int)vbeInfo.videoModes) & 0xffff);
  unsigned int ofs = videoModes;
  do {
    unsigned short k = 0;
    dosmemget(ofs,2,&k);
    if (k == 0xffff) break;
    ofs += 2;
  } while(true);
  uint16_t *w=(uint16_t*)malloc((ofs-videoModes)/2*sizeof(uint16_t));
  uint16_t *v=w;
  dosmemget(videoModes,ofs-videoModes,w);

  while(*v != 0xffff) {
    int mode = *v;
    memset(&regs,0,sizeof(__dpmi_regs));
    regs.x.ax = 0x4f01;
    regs.x.cx = (unsigned short)mode;
    VBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP) {
      construtGL();
      memset(&regs,0,sizeof(__dpmi_regs));
      regs.x.ax = 0x4f02;
      regs.x.bx = (unsigned short)(mode|0x4000);
      __dpmi_simulate_real_mode_interrupt(0x10, &regs); // needed here for later mouse setup
      glVesaBGRA = (modeInfo.bluePosition==0);
      glFrameBufferWidth = xRes;
      glFrameBufferHeight = yRes;
      // linear framebuffer mapping to vesasel not glFrameBufferDedicated
      glFrameBufferDedicated = NULL;
      linearFrameBufferStruct.size = 4096*1024-1;
      linearFrameBufferStruct.address = modeInfo.linearFrameBuffer;
      __dpmi_physical_address_mapping(&linearFrameBufferStruct);
      vesasel = __dpmi_allocate_ldt_descriptors(1);
      __dpmi_set_segment_base_address(vesasel,linearFrameBufferStruct.address);
      __dpmi_set_segment_limit(vesasel,linearFrameBufferStruct.size-1);
      glFrameBuffer = (unsigned int *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
      glDepthBuffer = (float *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(float));
      for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) _farpokel(vesasel,i<<2,0);
      memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      glFrameBufferWidth0 = glFrameBufferWidth;
      glFrameBufferHeight0 = glFrameBufferHeight;
      glFrameBuffer0 = glFrameBuffer;
      glDepthBuffer0 = glDepthBuffer;
      glSetupMouse();
      glSetTime(0);
      glViewport(0,0,xRes,yRes);
      free(w);
      noRefresh = false;
      return true;
    }
    v++;
  }
  free(w);
  return false;
}
// --------------------------------------
#endif // DJGPPVESA
// --------------------------------------

unsigned short glGraphicsModeToRestore=0x02;

void setBiosGraphicsMode(unsigned short mode) {
    union REGS regs;
    regs.w.ax = mode;
    int386(0x10, &regs, &regs);
}

unsigned short getBiosGraphicsMode() {
    union REGS regs;
    regs.w.ax = 0x0f00;
    int386 (0x10, &regs, &regs);
    return regs.h.al;
}

static unsigned int toColor(int r,int g,int b) {
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  return r|(g<<8)|(b<<16)|0xff000000;
}
static void setPalette(int index, unsigned int color) {
  outp(0x3c8,index);
  outp(0x3c9,(color & 255)>>2);
  outp(0x3c9,((color>>8) & 255)>>2);
  outp(0x3c9,((color>>16) & 255)>>2);
}
static void setHiColorPalette() {
  int i;
  for (i = 0; i < 64; i++) setPalette(i,toColor(0,i*4+3,0));
  for (i = 0; i < 16*8; i++) setPalette(i+128,toColor((i/8)*16+15,0,(i & 7)*32+31));
}
static void setModeX() {
  outp(0x3c4,4);
  int a = inp(0x3c5);
  a &= 255-8;
  a |= 4;
  outp(0x3c5,a);  
  outp(0x3d4,0x14);
  a = inp (0x3d5);
  a &= 255-64;
  outp(0x3d5,a);
  outp(0x3d4,0x17);
  a = inp(0x3d5);
  a |= 64;
  outp(0x3d5,a);
}
static void set400Lines() {
  outp(0x3d4,9);
  int a = inp(0x3d5);
  a &= 16+32+64;//001110000b
  outp(0x3d5,a);
}
static void setVGABufferStart(unsigned int adr) {
  outp(0x3d4,0x0c);
  outp(0x3d5,(adr >> 8) & 255);
  outp(0x3d4,0x0d);
  outp(0x3d5,adr & 255);
}
static bool hiColor = false;
static bool otherFrame = false;
unsigned char hiRedTab[256];
bool glVGA() {
  glGraphicsModeToRestore = getBiosGraphicsMode();
  construtGL();
  setBiosGraphicsMode(0x13);
  setModeX();
  set400Lines();
  setHiColorPalette();
  hiColor = true;
  otherFrame = false;
  unsigned char *vga = (unsigned char*)(VGARAMBASE);
  DJGPPVGAMEMON
  memset(vga,0,0x10000);
  DJGPPVGAMEMOFF
  setVGABufferStart(otherFrame ? 0x8000 : 0x0000);
  int xRes = 320;
  int yRes = 200;
  glFrameBufferWidth = xRes;
  glFrameBufferHeight = yRes;
  glFrameBuffer = (unsigned int *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
  glDepthBuffer = (float *)malloc(glFrameBufferWidth*glFrameBufferHeight*sizeof(float));
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
  memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
  glFrameBufferWidth0 = glFrameBufferWidth;
  glFrameBufferHeight0 = glFrameBufferHeight;
  glFrameBuffer0 = glFrameBuffer;
  glDepthBuffer0 = glDepthBuffer;
  glSetupMouse();
  glSetTime(0);
  glViewport(0,0,xRes,yRes);
  for (int i = 0; i < 256; i++) {
    hiRedTab[i] = (unsigned char)(128+(((i>>4)&15)<<3));
  }
  noRefresh = false;
  return true;
}
void glRefreshHiColor() {
  DJGPPVGAMEMON
  unsigned char *vga = (unsigned char*)(VGARAMBASE+(otherFrame?0x8000:0x00));
  for (int p = 0; p < 4; p++) {
    outp(0x3c4,0x02);
    outp(0x3c5,1<<p);
    unsigned char *s = (unsigned char*)&glFrameBuffer[p];
    for (int y = 0; y < 200; y++) {
      unsigned char *w1 = &vga[y*(320/4*2)];
      unsigned char *w2 = &vga[y*(320/4*2)+320/4];
      for (int x = 320/4; x > 0; x--) {
        *w2++ = (unsigned char)(s[1]>>2);
        *w1++ = (unsigned char)((s[2]>>5)|hiRedTab[s[0]]);
        s += 16;
      }
    }
  }
  DJGPPVGAMEMOFF
  setVGABufferStart(otherFrame ? 0x8000 : 0x0000);
  otherFrame = !otherFrame;   
}
void glRefresh() {
  if (noRefresh) return;
  if (hiColor) {glRefreshHiColor();return;}
  unsigned int rgba;
  if (glVesaBGRA) {
    for (int i = 0; i < glFrameBufferWidth0*glFrameBufferHeight0; i++) {
      rgba = glFrameBuffer0[i];
#  if __DJGPP__
      _farpokel(vesasel,i<<2,(rgba & 0xff00ff00)|((rgba>>16)&0xff)|((rgba<<16)&0x00ff0000));
#  else
      glFrameBufferDedicated[i] = (rgba & 0xff00ff00)|((rgba>>16)&0xff)|((rgba<<16)&0x00ff0000);
#  endif
    }
  } else {
    for (int i = 0; i < glFrameBufferWidth0*glFrameBufferHeight0; i++) {
      rgba = glFrameBuffer0[i];
#  if __DJGPP__
      _farpokel(vesasel,i<<2,rgba);
#  else
      glFrameBufferDedicated[i] = rgba;
#  endif
    }
  }
}
void glDone() {
  if (!noRefresh) { // glDirect?
    if (glFrameBuffer0!=NULL) free(glFrameBuffer0);
    if (glDepthBuffer0!=NULL) free(glDepthBuffer0);
    if (!hiColor) unmapPhysical((unsigned int)glFrameBufferDedicated);
    setBiosGraphicsMode(glGraphicsModeToRestore);
  }
}
#endif // __GLDISABLEDOSFUNCTIONS__

bool glDirect(unsigned int *frameBuffer, float *depthBuffer, unsigned int width, unsigned int height) {
  construtGL();
  glFrameBufferWidth = width;
  glFrameBufferHeight = height;
  glFrameBuffer = frameBuffer;
  glDepthBuffer = depthBuffer;
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*glFrameBufferBytesPerPixel);
  memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
  glFrameBufferWidth0 = glFrameBufferWidth;
  glFrameBufferHeight0 = glFrameBufferHeight;
  glFrameBuffer0 = glFrameBuffer;
  glDepthBuffer0 = glDepthBuffer;
  glSetupMouse();
  glSetTime(0);
  glViewport(0,0,width,height);
  noRefresh = true;
  return true;
}

void glSetMonitorAspectRatio(float aspect) {
  glContext.zoomX = 1.0/(aspect*glFrameBufferHeight0/glFrameBufferWidth0);
  glContext.zoomY = 1.0;
}

double glGetMonitorAspectRatio() {
  return 1.0/glContext.zoomX/((double)glFrameBufferHeight0/glFrameBufferWidth0);
}

void glZoomX(float zoom) {
  glContext.zoomX = zoom;
}

void glZoomY(float zoom) {
  glContext.zoomY = zoom;
}

float glGetZoomX() {
  return (float)glContext.zoomX;
}

float glGetZoomY() {
  return (float)glContext.zoomY;
}

void glExplicitAlpha(bool useExplicitAlpha, float alpha) {
  glContext.explicitAlpha = alpha;
  glContext.useExplicitAlpha = useExplicitAlpha;
}
                                                                                                                                                                         
float glPixelLastSX = 0;
float glPixelLastSY = 0;
bool glPixelLastValid = false;

bool glPixel(bool newXYZ, float xp, float yp, float zp, int x, int y, unsigned int color) {
  if (newXYZ) {
    glVertex v;
    v.vertexX = xp;
    v.vertexY = yp;
    v.vertexZ = zp;
    v.vertexW = 1;
    glPixelLastValid = (glTransformVertex(&glContext, &v, true) == 0);
    glPixelLastSX = (float)v.sx;
    glPixelLastSY = (float)v.sy;
  }
  if (glPixelLastValid)
  {
    int glClipRectX0 = 0;
    int glClipRectY0 = 0;
    int glClipRectX1 = glFrameBufferWidth;
    int glClipRectY1 = glFrameBufferHeight;
    combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,glContext.viewportX0,glContext.viewportY0,glContext.viewportX1,glContext.viewportY1);
    if (glIsEnabled(GL_SCISSOR_TEST)) {
      combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
    }
    int px = (int)(glPixelLastSX+x);
    int py = (int)(glPixelLastSY+y);
    if (px < glClipRectX0) return false; 
    if (py < glClipRectY0) return false;
    if (px >= glClipRectX1) return false;
    if (py >= glClipRectY1) return false;
    glFrameBuffer[px+py*glFrameBufferWidth] = color;
    return true;
  }
  return false;
}


// --------------------------------------

#ifndef __GLDISABLEDOSFUNCTIONS__

int glKey = 0;

unsigned short glNextKey() {
  union REGS regs;
  regs.w.ax = 0x0600;
  regs.w.dx = 0xff;
  int386(0x21, &regs, &regs);
  glKey = regs.w.ax & 0xff;
  if (glKey == 0) {
    regs.w.ax = 0x0600;
    regs.w.dx = 0xff;
    int386(0x21, &regs, &regs);
    glKey += (regs.w.ax & 0xff)<<8;
  }
  return (unsigned short)glKey;
}

int glLastMouseX = 0;
int glLastMouseY = 0;
int glLastMouseB = 0;
int glMouseX = 0;
int glMouseY = 0;
int glMouseB = 0;

void glSetMousePos(int x, int y) {
  union REGS regs;
  regs.w.ax = 0x04;
  regs.w.cx = (unsigned short)(x & 0xffff);
  regs.w.dx = (unsigned short)(y & 0xffff);
  int386(0x33, &regs, &regs);
}

void glNextMouseDelta(double *mouseX, double *mouseY) {
  union REGS regs;
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseX = glMouseX;
  glLastMouseY = glMouseY;
  glMouseX = regs.w.cx & 0xffff;
  glMouseY = regs.w.dx & 0xffff;
  *mouseX = glMouseX - glLastMouseX;
  *mouseY = glMouseY - glLastMouseY;
  // center mouse
  glSetMousePos(glFrameBufferWidth0/2, glFrameBufferHeight0/2);
  // update lastMouseX this way
  glMouseX = glFrameBufferWidth0/2;
  glMouseY = glFrameBufferHeight0/2;
}


char glMouseButtons() {
  union REGS regs;
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseB = glMouseB;
  glMouseB = regs.w.bx & 0xffff;
  return (char)(glMouseB & 0xff);
}

void glSetupMouse() {
    union REGS regs;
    regs.w.ax = 0x07;
    regs.w.cx = 0;
    regs.w.dx = (unsigned short)(glFrameBufferWidth0-1);
    int386(0x33, &regs, &regs);
    regs.w.ax = 0x08;
    regs.w.cx = 0;
    regs.w.dx = (unsigned short)(glFrameBufferHeight0-1);
    int386(0x33, &regs, &regs);
    glMouseX = glFrameBufferWidth0/2;
    glMouseY = glFrameBufferHeight0/2;
    glSetMousePos(glMouseX, glMouseY);
}

void glSpecialKeys(bool *shiftKey, bool *ctrlKey, bool *altKey) {
#ifdef __WATCOMC__
  int keyState = *((char *)0x417);
#endif // __WATCOMC__
#ifdef __DJGPP__
  char keyState;
  dosmemget(0x417,1,&keyState);
#endif // __DJGPP__
  *shiftKey = (keyState & 0x03) != 0;
  *ctrlKey = (keyState & 0x04) != 0;
  *altKey = (keyState & 0x08) != 0;
}

#ifdef __DJGPP__
typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;
int64_t glStartClock = 0;
#include <time.h>
double glSeconds() {
  return (double)(uclock() - glStartClock) / UCLOCKS_PER_SEC;
}
void glSetTime(double seconds) {
  glStartClock = uclock() - (int64_t)(seconds * UCLOCKS_PER_SEC);
}
#endif // __DJGPP__

#ifdef __WATCOMC__
double glStartClock = 0;
// very bad resolution, things seem to stutter by that, better use speaker.hpp and it's reprogrammed timer
double glSeconds() {
  return (double)(clock() - glStartClock) / CLOCKS_PER_SEC;
}
void glSetTime(double seconds) {
  glStartClock = clock() - seconds * CLOCKS_PER_SEC;
}
#endif // __WATCOMC__

#endif // __GLDISABLEDOSFUNCTIONS__


void glTexturePointer(int width, int height, unsigned int *textureData) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    free(t->data);
    t->data = NULL;
  }
  t->data = textureData;
  t->width = width;
  t->height = height;
}

void glSetRenderTarget(unsigned int *frameBufferOrNULL, float *depthBuffer, unsigned int width, unsigned int height) {
  if (frameBufferOrNULL == NULL) {
    glFrameBufferWidth = glFrameBufferWidth0;
    glFrameBufferHeight = glFrameBufferHeight0;
    glFrameBuffer = glFrameBuffer0;
    glDepthBuffer = glDepthBuffer0;
  } else {
    glFrameBufferWidth = width;
    glFrameBufferHeight = height;
    glFrameBuffer = frameBufferOrNULL;
    glDepthBuffer = depthBuffer;
  }
  glViewport(0,0,glFrameBufferWidth,glFrameBufferHeight);
}

unsigned int glGetTextureWidth(unsigned int textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return 0;
  return t->width;

}

unsigned int glGetTextureHeight(unsigned int textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return 0;
  return t->height;
}

GLuint *glGetTexturePointer(GLuint textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return NULL;
  return t->data;
}

#ifdef __GLDISABLEDOSFUNCTIONS__
// you may use glDirect(unsigned int *frameBuffer, float *depthBuffer, unsigned int width, unsigned int height);
double glSeconds() {return 0;}
void glSetTime(double seconds) {;}
void glSpecialKeys(bool *shiftKey, bool *ctrlKey, bool *altKey) {;}
void glSetupMouse() {;}
void glNextMouseDelta(double *mouseX, double *mouseY) {;}
char glMouseButtons() {return 0;}
void glSetMousePos(int x, int y) {;}
unsigned short glNextKey() {return 0;}
void glDone() {;}
void glRefresh() {;}
bool glVGA() {return false;}
bool glVesa(int xRes,int yRes, int bPP) {return false;}
#endif // __GLDISABLEDOSFUNCTIONS__

// --------------------------------------
// --------------------------------------
// --------------------------------------

#ifdef __cplusplus
}
#endif

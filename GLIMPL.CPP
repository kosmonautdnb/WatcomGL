//-------------------------
// OpenGL emulation for WatcomC/Dos. 
//--------------
// Just the most basic functionality. And some glu.
// Of course with texturing/blending and alphatest etc.. 
// It is required that Vesa 3.0 e.g. 640x480x32 works on your device.
// by Stefan Mader in 2025
//-------------------------
// Issues: In very rare cases (on 3d x or y==0) polygons may have seams
#include "gl.h"
#include <string.h>
#include <math.h>
#include <time.h>

#define PI 3.14159265358979323846
#define FLOOR(__v__) (__v__) // not needed since conversion to int truncates

double glPixelCenterX = -0.5;
double glPixelCenterY = -0.5;
double glTexelCenterX = -0.5;
double glTexelCenterY = -0.5;
unsigned int *glFrameBufferDedicated = NULL; // linear framebuffer
int glFrameBufferWidth = 0;
int glFrameBufferHeight = 0;
int glFrameBufferBytesPerPixel = 4;
unsigned int *glFrameBuffer = NULL;
//unsigned int *glStencilBuffer = NULL; // not supported
float *glDepthBuffer = NULL;

int glFrameBufferWidth0 = 0;
int glFrameBufferHeight0 = 0;
unsigned int *glFrameBuffer0 = NULL;
//unsigned int *glStencilBuffer0 = NULL;
float *glDepthBuffer0 = NULL;

GLdouble identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};
bool currentBackFacing = false;

#define __MATRIX_STACK_SIZE__ 16
#define __ATTRIB_STACK_SIZE__ 4

__inline float glClampf(float a, float n, float x) {
  return a < n ? n : (a > x ? x : a);
}

__inline int glClampi(int a, int n, int x) {
  return a < n ? n : (a > x ? x : a);
}

__inline int glMini(int a, int n) {
  return a < n ? a : n;
}

__inline int glMaxi(int a, int x) {
  return a > x ? a : x;
}

_GLContext::_GLContext() {
  int i;
  error = 0;
  viewportX0 = 0;
  viewportY0 = 0;
  viewportX1 = 0;
  viewportY1 = 0;
  clearRed = 0;
  clearGreen = 0;
  clearBlue = 0;
  clearAlpha = 0;
  clearDepth = 1.f;
  clearStencil = 0;
  activeTexture = 0;
  alphaFunc = 0;
  alphaFuncRef = 0;
  blendFuncSFactor = GL_ONE;
  blendFuncDFactor = GL_ZERO;
  cullFaceMode = GL_BACK;
  depthFunc = GL_LESS;
  depthMask = true;
  depthRangeZNear = 0;
  depthRangeZFar = 1;
  pointSize = 0;
  polygonOffsetFactor = 0;
  polygonOffsetUnits = 0;
  memcpy(matrixForMode[0],identityMatrix,4*4*sizeof(double));
  memcpy(matrixForMode[1],identityMatrix,4*4*sizeof(double));
  memcpy(inverseMatrixForMode[0],identityMatrix,4*4*sizeof(double));
  memcpy(inverseMatrixForMode[1],identityMatrix,4*4*sizeof(double));
  memcpy(matrix,identityMatrix,4*4*sizeof(double));
  matrixModeNr = 0;
  lineStippleFactor = 0;
  lineStipplePattern = 0;
  lineWidth = 1;
  scissorX0 = 0;
  scissorY0 = 0;
  scissorX1 = 0;
  scissorY1 = 0;
  shadeMode = 0;
  stencilFunc = 0;
  stencilFuncRef = 0;
  stencilFuncMask = 0;
  stencilMask = 0;
  stencilOpFail = 0;
  stencilOpZFail = 0;
  stencilOpZPass = 0;
  colorRed = 1.f;
  colorGreen = 1.f;
  colorBlue = 1.f;
  colorAlpha = 1.f;
  normalX = 0.0;
  normalY = 0.0;
  normalZ = 0.0;
  vertexX = 1.0;
  vertexY = 1.0;
  vertexZ = 1.0;
  vertexW = 1.0;
  textureX = 0.0;
  textureY = 0.0;
  textureZ = 0.0;
  textureW = 0.0;
  beginMode = 0;

  colorMaterialFace = GL_FRONT;
  for (i = 0; i < 2; i++) {
    colorMaterial[i] = GL_AMBIENT_AND_DIFFUSE;
    // ambient
    materialRed[i][0] = 0;
    materialGreen[i][0] = 0;
    materialBlue[i][0] = 0;
    materialAlpha[i][0] = 1;
    // diffuse
    materialRed[i][1] = 1;
    materialGreen[i][1] = 1;
    materialBlue[i][1] = 1;
    materialAlpha[i][1] = 1;
    // specular
    materialRed[i][2] = 0;
    materialGreen[i][2] = 0;
    materialBlue[i][2] = 0;
    materialAlpha[i][2] = 1;
    // emission
    materialRed[i][3] = 0;
    materialGreen[i][3] = 0;
    materialBlue[i][3] = 0;
    materialAlpha[i][3] = 1;
    // shininess
    materialRed[i][4] = 1;
    materialGreen[i][4] = 1;
    materialBlue[i][4] = 1;
    materialAlpha[i][4] = 1;
  }

  for (i = 0;  i < __GL_MAX_LIGHTS__; i++) {
    // ambient
    lightRed[0][i] = 0;
    lightGreen[0][i] = 0;
    lightBlue[0][i] = 0;
    lightAlpha[0][i] = 0;
    // diffuse
    lightRed[1][i] = 1;
    lightGreen[1][i] = 1;
    lightBlue[1][i] = 1;
    lightAlpha[1][i] = 1;
    // specular
    lightRed[2][i] = 0;
    lightGreen[2][i] = 0;
    lightBlue[2][i] = 0;
    lightAlpha[2][i] = 0;
    // position
    lightRed[3][i] = 0;
    lightGreen[3][i] = 0;
    lightBlue[3][i] = 1;
    lightAlpha[3][i] = 0;
    // attenuation
    constantAttenuation[i] = 1;
    linearAttenuation[i] = 0;
    quadraticAttenuation[i] = 0;
  }
  for (i = 0; i < GLMAXTEXTUREUNITS; i++) {
    boundTextures[i] = 0;
  }
  blendColorRed = 0;
  blendColorGreen = 0;
  blendColorBlue = 0;
  blendColorAlpha = 1;
  blendEquation = GL_FUNC_ADD;
  frontFace = GL_CCW;
  maskRed = true;
  maskGreen = true;
  maskBlue = true;
  maskAlpha = true;
  forceNoCull = 0;
  zoomX = 1.0;
  zoomY = 1.0;
  pushAttribBitsHere = 0;
  explicitAlpha = 0.f;
  useExplicitAlpha = false;
  separateSpecular = false;
  texGenS = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  texGenT = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  needNewInverseModelView = false;
  twoSidedLighting = false;
  wireframe = false;

  fogStart = 0.0;
  fogEnd = 1.0;
  fogColor[0] = 0;
  fogColor[1] = 0;
  fogColor[2] = 0;
  fogColor[3] = 0;
  fogDensity = 1.0;
  fogMode = GL_EXP;
}

void _GLContext::enable(GLenum prop, bool enable) {enabledCaps[prop&255]=enable;}
GLboolean _GLContext::isEnabled(GLenum prop) {return enabledCaps[prop&255];}

_GLContext glContext;
GLdouble glMatrixStack[__MATRIX_STACK_SIZE__][4*4];
GLint glMatrixStackPos = 0;
_GLContext glAttribStack[__ATTRIB_STACK_SIZE__];
GLint glAttribStackPos = 0;

glVertex glVertices[4];
int glCurrentVertexElement = 0;

TriangleDrawer glDrawTriangle = glDrawTrianglePrecise;
void glSetTriangleDrawer(TriangleDrawer drawer) {
  glDrawTriangle = drawer;
}
void glDrawQuad(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3);

glTexture glTextures[GLMAXTEXTURES]={{0,0,0,NULL,0,0,GL_LINEAR,1000,1000,GL_NEAREST,0,GL_REPEAT,GL_REPEAT,GL_REPEAT,1.f,0.f,1.f,1.f,GL_MODULATE}};

GLuint glNewTexture() {
  for (int i = 1; i < GLMAXTEXTURES; i++) {
    if (glTextures[i].name == 0x00) {
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
      delete[] glTextures[i].data;
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

void combineIntoWindow(GLint &x0, GLint &y0, GLint &x1, GLint &y1,GLint nx0, GLint ny0, GLint nx1, GLint ny1) {
  x0 = nx0<x0?x0:nx0;
  y0 = ny0<y0?y0:ny0;
  x1 = nx1>x1?x1:nx1;
  y1 = ny1>y1?y1:ny1;
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

void glMatMulf2(const GLdouble *ma, const GLfloat *mb, GLdouble *r) {
  GLdouble result[16];
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      float a = 0.f;
      for (int k = 0; k < 4; k++) {
        a += ma[i+k*4] * mb[j*4+k];
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
      float a = 0.f;
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

__inline int glTransformVertex(_GLContext *context, glVertex *v, bool clip) {
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

void fixSphereMapUV(_GLContext *context, float &tx0, float &ty0, float &tx1, float &ty1) {
  if (context->isEnabled(GL_TEXTURE_GEN_S)&&(context->texGenS == GL_SPHERE_MAP_DUAL_PARABOLOID||context->texGenS == GL_SPHERE_MAP_ATAN2||context->texGenS == GL_SPHERE_MAP)) {
    if (tx1-tx0>0.5) tx1-=1.0;
    if (tx1-tx0<-0.5) tx1+=1.0;
  }
  if (context->isEnabled(GL_TEXTURE_GEN_T)&&(context->texGenT == GL_SPHERE_MAP_DUAL_PARABOLOID||context->texGenT == GL_SPHERE_MAP_ATAN2||context->texGenT == GL_SPHERE_MAP)) {
    if (ty1-ty0>0.5) ty1-=1.0;
    if (ty1-ty0<-0.5) ty1+=1.0;
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
  case GL_SPHERE_MAP: {v->textureY = reflectionY*0.5+0.5;} break;
  case GL_SPHERE_MAP_ATAN2: {v->textureY = acos(-reflectionY)/PI;} break;
  case GL_SPHERE_MAP_DUAL_PARABOLOID: {v->textureY = reflectionY/(fabs(reflectionZ)+1.0)*0.5+0.5;} break;
  }
}

void glLightVertex(_GLContext *context, glVertex *v) {
  if (context->isEnabled(GL_LIGHTING)) {
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

    int face = currentBackFacing ? 1:0;
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


    if (context->isEnabled(GL_COLOR_MATERIAL)) {
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
    //if (context->isEnabled(GL_NORMALIZE)) {
      float l = sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
      if (l>0.f) {
        normalX/=l;
        normalY/=l;
        normalZ/=l;
      }
    //}

    for (int i = 0; i < __GL_MAX_LIGHTS__; i++) {
      if (context->isEnabled(GL_LIGHT0+i)) {
        const float lightAmbientRed = context->lightRed[0][i];
        const float lightAmbientGreen = context->lightGreen[0][i];
        const float lightAmbientBlue = context->lightBlue[0][i];
        const float lightDiffuseRed = context->lightRed[1][i];
        const float lightDiffuseGreen = context->lightGreen[1][i];
        const float lightDiffuseBlue = context->lightBlue[1][i];
        const float lightDiffuseAlpha = context->lightAlpha[1][i];
        const float lightSpecularRed = context->lightRed[2][i];
        const float lightSpecularGreen = context->lightGreen[2][i];
        const float lightSpecularBlue =  context->lightBlue[2][i];
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
        const bool halveVector = false;
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
        ambientRed += materialAmbientRed*lightAmbientRed;
        ambientGreen += materialAmbientGreen*lightAmbientGreen;
        ambientBlue += materialAmbientBlue*lightAmbientBlue;
        diffuse *= attenuation;
        diffuseRed += materialDiffuseRed*lightDiffuseRed*diffuse;
        diffuseGreen += materialDiffuseGreen*lightDiffuseGreen*diffuse;
        diffuseBlue += materialDiffuseBlue*lightDiffuseBlue*diffuse;
        double shininess = materialShininessRed;
        if (shininess < 0.001) shininess = 0.001;
        specular = pow(specular,shininess) * attenuation;
        specularRed += materialSpecularRed*lightSpecularRed*specular;
        specularGreen += materialSpecularGreen*lightSpecularGreen*specular;
        specularBlue += materialSpecularBlue*lightSpecularBlue*specular;
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
  if (context->isEnabled(GL_TEXTURE_GEN_S)||context->isEnabled(GL_TEXTURE_GEN_T)) {
    const double *matrix2 = glGetInverseModelView(context);
    glTexGen_(context,v,v->vertexX-matrix2[3*4+0],v->vertexY-matrix2[3*4+1],v->vertexZ-matrix2[3*4+2],v->normalX,v->normalY,v->normalZ);
  }
}

__inline int glClipVertex(_GLContext *context, glVertex *v) {
  int clipFlags = (v->sx < context->viewportX0) ? 1 : 0;
  clipFlags |= (v->sy < context->viewportY0) ? 2 : 0;
  clipFlags |= (v->sx > context->viewportX1) ? 4 : 0;
  clipFlags |= (v->sy > context->viewportY1) ? 8 : 0;
  return clipFlags;
}

void interpolateVertex(glVertex *dest, glVertex *v0, glVertex *v1, double f) {
  dest->colorRed = v0->colorRed + f * (v1->colorRed-v0->colorRed);
  dest->colorGreen = v0->colorGreen + f * (v1->colorGreen-v0->colorGreen);
  dest->colorBlue = v0->colorBlue + f * (v1->colorBlue-v0->colorBlue);
  dest->colorAlpha = v0->colorAlpha + f * (v1->colorAlpha-v0->colorAlpha);

  if (glContext.separateSpecular) {
    dest->additionalSpecularColorRed = v0->additionalSpecularColorRed + f * (v1->additionalSpecularColorRed-v0->additionalSpecularColorRed);
    dest->additionalSpecularColorGreen = v0->additionalSpecularColorGreen + f * (v1->additionalSpecularColorGreen-v0->additionalSpecularColorGreen);
    dest->additionalSpecularColorBlue = v0->additionalSpecularColorBlue + f * (v1->additionalSpecularColorBlue-v0->additionalSpecularColorBlue);
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
      float f = (w0+z0) / ((z0-z1)+(w0-w1));
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],false);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else 
    if((!currentInside) && nextInside) {
      float f = (w0+z0) / ((z0-z1)+(w0-w1));
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

bool backFaceCalc(const glVertex &v0,const glVertex &v1,const glVertex &v2) {
  if (glContext.twoSidedLighting) {
    double ax = v1.vertexX-v0.vertexX;
    double ay = v1.vertexY-v0.vertexY;
    double az = v1.vertexZ-v0.vertexZ;
    double bx = v2.vertexX-v0.vertexX;
    double by = v2.vertexY-v0.vertexY;
    double bz = v2.vertexZ-v0.vertexZ;
    double nx = ay*bz - az*by;
    double ny = az*bx - ax*bz;
    double nz = ax*by - ay*bx;
    //double *m = glContext.matrixForMode[GL_MODELVIEW & 1];
    //double d = m[4*0+2]*nx+m[4*1+2]*ny+m[4*2+2]*nz; // this transpose just works for orthogonal matrices
    double *m = glGetInverseModelView(&glContext); // maybe the inverse is better here
    double d = m[4*2+0]*nx+m[4*2+1]*ny+m[4*2+2]*nz;
    currentBackFacing = d < 0; // used only for coloring currently
  }
  return false;
}

void glEmitVertex() {
  currentBackFacing = false;
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
            const double nx = dy * glContext.zoomX;
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
            const double nx = dy*glContext.zoomX;
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
        if (glContext.wireframe) {
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
          glContext.beginMode = GL_TRIANGLES; glCurrentVertexElement = 0;
          break;
        }
        if (backFaceCalc(glVertices[0],glVertices[1],glVertices[2])) {glCurrentVertexElement = 0;break;}
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
        glCurrentVertexElement = 0;
      }
    } break;
    case GL_QUADS: {
      if (glCurrentVertexElement==4) {
        if (glContext.wireframe) {
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
          glContext.beginMode = GL_QUADS; glCurrentVertexElement = 0;
          break;
        }
        if (backFaceCalc(glVertices[0],glVertices[1],glVertices[2])) {glCurrentVertexElement = 0;break;}
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
        glCurrentVertexElement = 0;
      }
    } break;
    case GL_POINTS: {
      if (glCurrentVertexElement==1) {
        int a = glTransformVertex(&glContext,&glVertices[0],true);
        if (a == 0) {
          glVertices[1] = glVertices[0];
          glVertices[2] = glVertices[0];
          glVertices[3] = glVertices[0];
          const double pointSizeX = glContext.pointSize*0.5*glContext.zoomX;
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

void glActiveTexture (GLenum texture) {
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

void glBindTexture (GLenum target, GLuint texture) {
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
    combineIntoWindow(minX,minY,maxX,maxY,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
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
  //if ((mask & GL_STENCIL_BUFFER_BIT) && (glStencilBuffer != NULL)) {
  //  unsigned int k = glContext.clearStencil;
  //  for (int y = minY; y < maxY; y++)
  //    for (int x = minX; x < maxX; x++)
  //      glStencilBuffer[x+y*glFrameBufferWidth] = k;
  //}
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
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
}

void glColor3dv(const GLdouble *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
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
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
  glContext.colorAlpha = alpha;
}

void glColor4dv (const GLdouble *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
  glContext.colorAlpha = v[3];
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
  glContext.enable(cap, false);
}

void glDisableClientState(GLenum array) {
//  glContext.enableClientState(array, false);
}

void glEnable(GLenum cap) {
  glContext.enable(cap, true);
}

void glEnableClientState(GLenum array) {
//  glContext.enableClientState(array, true);
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
  case GL_FOG_MODE: {glContext.fogMode = params[0];} break;
  }
}

void glFogf(GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = param;} break;
  case GL_FOG_END: {glContext.fogEnd = param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = param;} break;
  case GL_FOG_MODE: {glContext.fogMode = param;} break;
  }
}

void glFogi(GLenum pname, GLint param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = param;} break;
  case GL_FOG_END: {glContext.fogEnd = param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = param;} break;
  case GL_FOG_MODE: {glContext.fogMode = param;} break;
  }
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
  case GL_ALPHA_TEST_FUNC: {*params=glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=glContext.alphaFuncRef;} break;
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
  case GL_LINE_WIDTH: {*params=(int)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=__GL_MAX_LIGHTS__;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(int)floor(glContext.pointSize);} break;
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
  case GL_ZOOM_X: {*params=(int)floor(glContext.zoomX);} break;
  case GL_ZOOM_Y: {*params=(int)floor(glContext.zoomY);;} break;
  }
}

void glGetDoublev (GLenum pname, GLdouble *params) {
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
  case GL_MAX_LIGHTS: {*params=__GL_MAX_LIGHTS__;} break;
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

void glGetFloatv (GLenum pname, GLfloat *params) {
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
  case GL_MAX_LIGHTS: {*params=__GL_MAX_LIGHTS__;} break;
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
  case GL_ZOOM_Y: {*params=glContext.zoomY;} break;
  }
}

void glGetIntegerv (GLenum pname, GLint *params) {
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
  case GL_LINE_WIDTH: {*params=(int)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=__GL_MAX_LIGHTS__;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(int)floor(glContext.pointSize);} break;
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
  case GL_POSITION:
  case GL_DIFFUSE:
  case GL_SPECULAR:
  case GL_AMBIENT: {
      params[0] = glContext.lightRed[pname & 3][light - GL_LIGHT0];
      params[1] = glContext.lightGreen[pname & 3][light - GL_LIGHT0];
      params[2] = glContext.lightBlue[pname & 3][light - GL_LIGHT0];
      params[3] = glContext.lightAlpha[pname & 3][light - GL_LIGHT0];
    }; break;
  }
}

const GLubyte *glGetString(GLenum name) {
  return "";
}

GLboolean glIsEnabled(GLenum cap) {
  return glContext.isEnabled(cap);
}  

void glLightf(GLenum light, GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = param; return;} break;
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = param; return;} break;
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = param; return;} break;
  }
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = params[0]; return;} break;
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = params[0]; return;} break;
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = params[0]; return;} break;
  }

  if (pname == GL_POSITION) {
    double lx = params[0];
    double ly = params[1];
    double lz = params[2];
    double lw = params[3];
    //if (fabs(lw)>0.0) {
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
    //}
    glContext.lightRed[pname & 3][light - GL_LIGHT0] = lx;
    glContext.lightGreen[pname & 3][light - GL_LIGHT0] = ly;
    glContext.lightBlue[pname & 3][light - GL_LIGHT0] = lz;
    glContext.lightAlpha[pname & 3][light - GL_LIGHT0] = lw;
  } else {
    glContext.lightRed[pname & 3][light - GL_LIGHT0] = params[0];
    glContext.lightGreen[pname & 3][light - GL_LIGHT0] = params[1];
    glContext.lightBlue[pname & 3][light - GL_LIGHT0] = params[2];
    glContext.lightAlpha[pname & 3][light - GL_LIGHT0] = params[3];
  }
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
  glContext.lineStippleFactor = factor;
  glContext.lineStipplePattern = pattern;
}

void glLineWidth(GLfloat width) {
  glContext.lineWidth = width;
}

void glLoadIdentity() {
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], identityMatrix, 4*4*sizeof(GLdouble));
  glUpdateMatrix();
}

void glLoadMatrixf (const GLfloat *m) {
  for (int i = 0; i < 4*4; i++) glContext.matrixForMode[glContext.matrixModeNr][i] = m[i];
  glUpdateMatrix();
}

void glLoadMatrixd (const GLdouble *m) {
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
  glMaterialf(face,pname,param);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param) {
  float p[3];
  p[0] = param;
  p[1] = param;
  p[2] = param;
  glMaterialfv(face,pname,p);
}

void glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {
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

void glMatrixMode (GLenum mode) {
  glContext.matrixModeNr = mode & 0x01;
}

void glMultMatrixf (const GLfloat *m) {
  glMatMulf2(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

void glMultMatrixd (const GLdouble *m) {
  glMatMul(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

void glPointSize (GLfloat size) {
  glContext.pointSize = size;
}

void glPolygonMode (GLenum face, GLenum mode) {
  glContext.wireframe = mode == GL_LINE;
}

void glPolygonOffset (GLfloat factor, GLfloat units) {
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

#define FLIPRB(__v__) (((__v__)&0xff00ff00)|(((__v__)>>16) & 0x000000ff)|(((__v__)<<16) & 0x00ff0000))

void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
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
        case GL_RED: {((unsigned char*)pixels)[xp+yp*width] = rgba & 0xff;} break;
        case GL_GREEN: {((unsigned char*)pixels)[xp+yp*width] = (rgba>>8)& 0xff;} break;
        case GL_BLUE: {((unsigned char*)pixels)[xp+yp*width] = (rgba>>16)& 0xff;} break;
        case GL_RGB: {unsigned char*c=&((unsigned char*)pixels)[(xp+yp*width)*3];c[0]=rgba&255;c[1]=(rgba>>8)&255;c[2]=(rgba>>16)&255;} break;
        case GL_BGR: {unsigned char*c=&((unsigned char*)pixels)[(xp+yp*width)*3];c[2]=rgba&255;c[1]=(rgba>>8)&255;c[0]=(rgba>>16)&255;} break;
        }
      }
    }
  }
}

void glhRotate(GLdouble *matrix, GLfloat angleInRadians, GLfloat x, GLfloat y, GLfloat z) {
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
  double l = sqrt(x*x+y*y+z*z);
  if (l>0) {
    x/=l;
    y/=l;
    z/=l;
  }
  glhRotate(matrix,angle*2.0*PI/360.0,x,y,z);
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

void glTexEnvfv (GLenum target, GLenum pname, const GLfloat *params) {

}

void glTexEnvi (GLenum target, GLenum pname, GLint param) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  switch(pname) {
    case GL_TEXTURE_ENV_MODE: {
      t->texEnvMode = param;
    } break;
  }
}

void glTexGeni (GLenum coord, GLenum pname, GLint param) {
  switch(coord) {
  case GL_S: {glContext.texGenS = param;} break;
  case GL_T: {glContext.texGenT = param;} break;
  }
}

void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    delete[] t->data;
    t->data = NULL;
  }
  t->data = new unsigned int[width*height];
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
    int i2 = width*height-(y+1)*width; // textures are flipped read out (source bottom left is tex pixel uv 0.0)
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

void glTexSubImage2D (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
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
    int i2 = width*height-(y+1)*width; // textures are flipped read out (source bottom left is tex pixel uv 0.0)
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
      if (x2 >= 0 && y2 >= 0 && x2 < t->width && y2 < t->height)
        t->data[x2+y2*t->width] = rgba;
      i2++;
    }
  }
}


void glhTranslate(GLdouble *matrix, GLfloat x, GLfloat y, GLfloat z) {
  matrix[12]=matrix[0]*(double)x+matrix[4]*(double)y+matrix[8]*(double)z+matrix[12];
  matrix[13]=matrix[1]*(double)x+matrix[5]*(double)y+matrix[9]*(double)z+matrix[13];
  matrix[14]=matrix[2]*(double)x+matrix[6]*(double)y+matrix[10]*(double)z+matrix[14];
  matrix[15]=matrix[3]*(double)x+matrix[7]*(double)y+matrix[11]*(double)z+matrix[15];
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glhTranslate(matrix,x,y,z);
  glUpdateMatrix();
}

void glTranslatefv(const GLfloat *v) {
  glTranslatef(v[0],v[1],v[2]);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z) {
  double *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glhTranslate(matrix,x,y,z);
  glUpdateMatrix();
}

void glTranslatedv(const GLdouble *v) {
  glTranslated(v[0],v[1],v[2]);
}

void glVertex2f (GLfloat x, GLfloat y) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glEmitVertex();
}

void glVertex2fv (const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glEmitVertex();
}

void glVertex3f (GLfloat x, GLfloat y, GLfloat z) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glEmitVertex();
}

void glVertex3fv (const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glContext.vertexZ = v[2];
  glEmitVertex();
}

void glVertex4f (GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glContext.vertexW = w;
  glEmitVertex();
}

void glVertex4fv (const GLfloat *v) {
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

void glPushAttrib(GLbitfield mask) {
  // mask not supported and behaves different than glPushAttrib + only small Stack Depth, yet
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

void glTexParameterfv (GLenum target, GLenum pname, GLfloat *param) {
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

GLAPI void APIENTRY glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glContext.blendColorRed = red;
  glContext.blendColorGreen = green;
  glContext.blendColorBlue = blue;
  glContext.blendColorAlpha = alpha;
}

GLAPI void APIENTRY glBlendEquation(GLenum mode) {
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
void MultiplyMatrixByVector4by4OpenGL_FLOAT(float *dest, float *m, float *v) {
  float x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  float y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  float z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  float w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

void MultiplyMatrixByVector4by4OpenGL_DOUBLE(double *dest, double *m, double *v) {
  double x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  double y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  double z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  double w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

void MultiplyMatrices4by4OpenGL_FLOAT(float *dest, float *m1, float *m2) {
  glMatMulf(m1,m2,dest);
}

void MultiplyMatrices4by4OpenGL_DOUBLE(double *dest, double *m1, double *m2) {
  glMatMul(m1,m2,dest);
}

void NormalizeVector(double *v) {
  double l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if (l>0) {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

void ComputeNormalOfPlane(double *normal, double *pvector1, double *pvector2) {
  normal[0]=(pvector1[1]*pvector2[2])-(pvector1[2]*pvector2[1]);
  normal[1]=(pvector1[2]*pvector2[0])-(pvector1[0]*pvector2[2]);
  normal[2]=(pvector1[0]*pvector2[1])-(pvector1[1]*pvector2[0]);
}

void glhFrustum(double *matrix, double left, double right, double bottom, double top, double znear, double zfar) {
  double matrix2[16],temp, temp2, temp3, temp4;
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
  MultiplyMatrices4by4OpenGL_DOUBLE(matrix, matrix, matrix2);
}

void glhPerspective(double *matrix, double fovyInDegrees, double aspectRatio, double znear, double zfar) {
  double ymax, xmax;
  double temp, temp2, temp3, temp4;
  ymax = znear * tan(fovyInDegrees*PI/360.f);
  xmax = ymax * aspectRatio;
  glhFrustum(matrix,-xmax,xmax,-ymax,ymax,znear,zfar);
}

void glhLookAt(double *matrix, double *eyePosition3D, double *center3D, double *upVector3D) {
  double forward[3], side[3], up[3];
  double matrix2[16], resultMatrix[16];
  forward[0] = center3D[0]-eyePosition3D[0];
  forward[1] = center3D[1]-eyePosition3D[1];
  forward[2] = center3D[2]-eyePosition3D[2];
  NormalizeVector(forward);
  ComputeNormalOfPlane(side, forward, upVector3D);
  NormalizeVector(side);
  ComputeNormalOfPlane(up, side, forward);
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
  glhTranslate(resultMatrix,-eyePosition3D[0],-eyePosition3D[1],-eyePosition3D[2]);
  memcpy(matrix,resultMatrix,16*sizeof(double));
}

#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

int glhInvertMatrixf(const float *m, float *out)
{
   float wtmp[4][8];
   float m0, m1, m2, m3, s;
   float *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
      r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
      r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
      r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
      r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0]) > fabs(r2[0]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0]))
      SWAP_ROWS_FLOAT(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0]))
      SWAP_ROWS_FLOAT(r1, r0);
   if (0.0 == r0[0])
      return 0;

   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[1]) > fabs(r2[1]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1]))
      SWAP_ROWS_FLOAT(r2, r1);
   if (0.0 == r1[1])
      return 0;

   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[2]) > fabs(r2[2]))
      SWAP_ROWS_FLOAT(r3, r2);
   if (0.0 == r2[2])
      return 0;

   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3])
      return 0;

   s = 1.0 / r3[3];		/* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;

   m2 = r2[3];			/* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];			/* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];			/* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out, 0, 0) = r0[4];
   MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
   MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
   MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
   MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
   MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
   MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
   MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
   MAT(out, 3, 3) = r3[7];

   return 1;
}

int glhInvertMatrix(const double *m, double *out)
{
   double wtmp[4][8];
   double m0, m1, m2, m3, s;
   double *r0, *r1, *r2, *r3;

   r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];

   r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
      r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
      r0[4] = 1.0, r0[5] = r0[6] = r0[7] = 0.0,
      r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
      r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
      r1[5] = 1.0, r1[4] = r1[6] = r1[7] = 0.0,
      r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
      r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
      r2[6] = 1.0, r2[4] = r2[5] = r2[7] = 0.0,
      r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
      r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
      r3[7] = 1.0, r3[4] = r3[5] = r3[6] = 0.0;

   /* choose pivot - or die */
   if (fabs(r3[0]) > fabs(r2[0]))
      SWAP_ROWS_DOUBLE(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0]))
      SWAP_ROWS_DOUBLE(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0]))
      SWAP_ROWS_DOUBLE(r1, r0);
   if (0.0 == r0[0])
      return 0;

   /* eliminate first variable     */
   m1 = r1[0] / r0[0];
   m2 = r2[0] / r0[0];
   m3 = r3[0] / r0[0];
   s = r0[1];
   r1[1] -= m1 * s;
   r2[1] -= m2 * s;
   r3[1] -= m3 * s;
   s = r0[2];
   r1[2] -= m1 * s;
   r2[2] -= m2 * s;
   r3[2] -= m3 * s;
   s = r0[3];
   r1[3] -= m1 * s;
   r2[3] -= m2 * s;
   r3[3] -= m3 * s;
   s = r0[4];
   if (s != 0.0) {
      r1[4] -= m1 * s;
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r0[5];
   if (s != 0.0) {
      r1[5] -= m1 * s;
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r0[6];
   if (s != 0.0) {
      r1[6] -= m1 * s;
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r0[7];
   if (s != 0.0) {
      r1[7] -= m1 * s;
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[1]) > fabs(r2[1]))
      SWAP_ROWS_DOUBLE(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1]))
      SWAP_ROWS_DOUBLE(r2, r1);
   if (0.0 == r1[1])
      return 0;

   /* eliminate second variable */
   m2 = r2[1] / r1[1];
   m3 = r3[1] / r1[1];
   r2[2] -= m2 * r1[2];
   r3[2] -= m3 * r1[2];
   r2[3] -= m2 * r1[3];
   r3[3] -= m3 * r1[3];
   s = r1[4];
   if (0.0 != s) {
      r2[4] -= m2 * s;
      r3[4] -= m3 * s;
   }
   s = r1[5];
   if (0.0 != s) {
      r2[5] -= m2 * s;
      r3[5] -= m3 * s;
   }
   s = r1[6];
   if (0.0 != s) {
      r2[6] -= m2 * s;
      r3[6] -= m3 * s;
   }
   s = r1[7];
   if (0.0 != s) {
      r2[7] -= m2 * s;
      r3[7] -= m3 * s;
   }

   /* choose pivot - or die */
   if (fabs(r3[2]) > fabs(r2[2]))
      SWAP_ROWS_DOUBLE(r3, r2);
   if (0.0 == r2[2])
      return 0;

   /* eliminate third variable */
   m3 = r3[2] / r2[2];
   r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
      r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];

   /* last check */
   if (0.0 == r3[3])
      return 0;

   s = 1.0 / r3[3];		/* now back substitute row 3 */
   r3[4] *= s;
   r3[5] *= s;
   r3[6] *= s;
   r3[7] *= s;

   m2 = r2[3];			/* now back substitute row 2 */
   s = 1.0 / r2[2];
   r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
      r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
   m1 = r1[3];
   r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1,
      r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
   m0 = r0[3];
   r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0,
      r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;

   m1 = r1[2];			/* now back substitute row 1 */
   s = 1.0 / r1[1];
   r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
      r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
   m0 = r0[2];
   r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0,
      r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;

   m0 = r0[1];			/* now back substitute row 0 */
   s = 1.0 / r0[0];
   r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
      r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);

   MAT(out, 0, 0) = r0[4];
   MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
   MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4];
   MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
   MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4];
   MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
   MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4];
   MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
   MAT(out, 3, 3) = r3[7];

   return 1;
}

int gluInvertMatrixf(const float *m, float *dest) {
  return glhInvertMatrixf(m,dest);
}

int gluInvertMatrix(const double *m, double *dest) {
  return glhInvertMatrix(m,dest);
}

int glhUnProjectf(float winx, float winy, float winz,
     float *modelview, float *projection,
     int *viewport,
     float *objectCoordinate)
{
  //Transformation matrices
  float m[16], A[16];
  float in[4], out[4];
  
  //Calculation for inverting a matrix, compute projection x modelview
  //and store in A[16]
  MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
  //Now compute the inverse of matrix A
  if(glhInvertMatrixf(A, m)==0)
    return 0;
  
  //Transformation of normalized coordinates between -1 and 1
  in[0]=((winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0)/glContext.zoomX;
  in[1]=((winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0)/glContext.zoomY;
  in[2]=2.0*winz-1.0;
  in[3]=1.0;
  
  //Objects coordinates
  MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
  if(out[3]==0.0)
    return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

int glhUnProject(double winx, double winy, double winz,
     double *modelview, double *projection,
     int *viewport,
     double *objectCoordinate)
{
  //Transformation matrices
  double m[16], A[16];
  double in[4], out[4];
  
  //Calculation for inverting a matrix, compute projection x modelview
  //and store in A[16]
  MultiplyMatrices4by4OpenGL_DOUBLE(A, projection, modelview);
  //Now compute the inverse of matrix A
  if(glhInvertMatrix(A, m)==0)
    return 0;
  
  //Transformation of normalized coordinates between -1 and 1
  in[0]=((winx-(double)viewport[0])/(double)viewport[2]*2.0-1.0)/glContext.zoomX;
  in[1]=((winy-(double)viewport[1])/(double)viewport[3]*2.0-1.0)/glContext.zoomY;
  in[2]=2.0*winz-1.0;
  in[3]=1.0;
  
  //Objects coordinates
  MultiplyMatrixByVector4by4OpenGL_DOUBLE(out, m, in);
  if(out[3]==0.0)
    return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

int glhProjectfx(float objx, float objy, float objz,
  float *modelview, float *projection,
  int *viewport,
  float *windowCoordinate) {
  float in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  MultiplyMatrixByVector4by4OpenGL_FLOAT(in, modelview, in);
  MultiplyMatrixByVector4by4OpenGL_FLOAT(in, projection, in);
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

int glhProjectx(double objx, double objy, double objz,
  double *modelview, double *projection,
  int *viewport,
  double *windowCoordinate) {
  double in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  MultiplyMatrixByVector4by4OpenGL_DOUBLE(in, modelview, in);
  MultiplyMatrixByVector4by4OpenGL_DOUBLE(in, projection, in);
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

int glhProjectf(float objx, float objy, float objz,
  float *modelview, float *projection,
  int *viewport,
  float *windowCoordinate)
{
  //Transformation vectors
  float fTempo[8];
  //Modelview transform
  fTempo[0]=modelview[0]*objx+
  modelview[4]*objy+
  modelview[8]*objz+
  modelview[12];		//w is always 1
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
  //Projection transform, the final row of projection matrix is always [0 0 -1 0]
  //so we optimize for that.
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
  
  //The result normalizes between -1 and 1
  if(fTempo[7]==0.0)	//The w value
    return 0;
  
  fTempo[7]=1.0/fTempo[7];
  
  //Perspective division
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  
  //Window coordinates
  //Map x, y to range 0-1
  windowCoordinate[0]=(fTempo[4]*0.5*glContext.zoomX+0.5)*viewport[2]+viewport[0];
  windowCoordinate[1]=(fTempo[5]*0.5*glContext.zoomY+0.5)*viewport[3]+viewport[1];
  
  //This is only correct when glDepthRange(0.0, 1.0)
  windowCoordinate[2]=(1.0+fTempo[6])*0.5;	//Between 0 and 1
  return 1;
}

int glhProject(double objx, double objy, double objz,
 double *modelview, double *projection,
 int *viewport,
 double *windowCoordinate)
{
  //Transformation vectors
  double fTempo[8];
  //Modelview transform
  fTempo[0]=modelview[0]*objx+
  modelview[4]*objy+
  modelview[8]*objz+
  modelview[12];		//w is always 1
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
  //Projection transform, the final row of projection matrix is always [0 0 -1 0]
  //so we optimize for that.
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
  
  //The result normalizes between -1 and 1
  if(fTempo[7]==0.0)	//The w value
    return 0;
  
  fTempo[7]=1.0/fTempo[7];
  
  //Perspective division
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  
  //Window coordinates
  //Map x, y to range 0-1
  windowCoordinate[0]=(fTempo[4]*0.5*glContext.zoomX+0.5)*viewport[2]+viewport[0];
  windowCoordinate[1]=(fTempo[5]*0.5*glContext.zoomY+0.5)*viewport[3]+viewport[1];
  
  //This is only correct when glDepthRange(0.0, 1.0)
  windowCoordinate[2]=(1.0+fTempo[6])*0.5;	//Between 0 and 1
  return 1;
}

void glOrtho(float left, float right, float bottom, float top, float znear, float zfar) {
//  GLfloat x,y,z;
//  GLfloat tx,ty,tz;
//  GLfloat m[16];
//  x = 2.0 / (right-left);
//  y = 2.0 / (top-bottom);
//  z = -2.0 / (zfar - znear);
//  tx = -(right+left)/(right-left);
//  ty = -(top+bottom)/(top-bottom);
//  tz = -(zfar+znear)/(zfar-znear);
//#define M(row,col) m[col*4+row]  
//  M(0,0)=x; M(0,1)=0; M(0,2) = 0.f; M(0,3) = tx;
//  M(1,0)=0; M(1,1)=y; M(1,2) = 0.f; M(1,3) = ty;
//  M(2,0)=0; M(2,1)=0; M(2,2) = z; M(2,3) = tz;
//  M(3,0)=0; M(3,1)=0; M(3,2) = 0; M(3,3) = 1;
//  glMultMatrixf(m);
  double r2 = (right-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  double l2 = (left-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  double t2 = (top-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;
  double b2 = (bottom-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;

  right = r2;
  left = l2;
  top = t2;
  bottom = b2;
  
  double matrix2[16], temp2, temp3, temp4, resultMatrix[16];
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
  MultiplyMatrices4by4OpenGL_DOUBLE(matrix, matrix, matrix2);
  glUpdateMatrix();
}

void glFrontFace(GLenum mode) {
  glContext.frontFace = mode;
}

void glFrustum(float left, float right, float bottom, float top, float znear, float zfar) {
  glhFrustum(glContext.matrixForMode[glContext.matrixModeNr],left,right,bottom,top,znear,zfar);
  glUpdateMatrix();
}

void glFrustumf(float left, float right, float bottom, float top, float znear, float zfar) {
  glFrustum(left,right,bottom,top,znear,zfar);
  glUpdateMatrix();
}

void glOrthof(float left, float right, float bottom, float top, float znear, float zfar) {
  glOrtho(left, right, bottom, top, znear, zfar);
  glUpdateMatrix();
}

void gluOrtho2D(float left, float right, float bottom, float top) {
  glOrtho(left, right, bottom, top, -1, 1);
  glUpdateMatrix();
}

void gluPerspective(GLfloat fov, GLfloat aspect, GLfloat nearPlane, GLfloat farPlane) {
  glhPerspective(glContext.matrixForMode[glContext.matrixModeNr],fov,aspect,nearPlane,farPlane);
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
  glhLookAt(glContext.matrixForMode[glContext.matrixModeNr],eye,center,up);
  glUpdateMatrix();
}

GLint gluProjectfx(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  float d[3];
  const GLint r = glhProjectfx(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluProjectf(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  float d[3];
  const GLint r = glhProjectf(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluUnProjectf(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *objX, GLfloat *objY, GLfloat *objZ) {
  float d[3];
  winX -= glPixelCenterX; // actually you need the pixelCenter?
  winY += glPixelCenterY; // actually you need the pixelCenter?
  const GLint r = glhUnProjectf(winX, winY, winZ, model, projection, view, d);
  *objX = d[0];
  *objY = d[1];
  *objZ = d[2];
  return r;
}

GLint gluProjectx(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  double d[3];
  const GLint r = glhProjectx(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  double d[3];
  const GLint r = glhProject(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *objX, GLdouble *objY, GLdouble *objZ) {
  double d[3];
  winX -= glPixelCenterX; // actually you need the pixelCenter?
  winY += glPixelCenterY; // actually you need the pixelCenter?
  const GLint r = glhUnProject(winX, winY, winZ, model, projection, view, d);
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
float glSign(float v) {
  return v < 0 ? -1 : (v > 0 ? 1 : 0);
}
__inline double getBary(double x, double y, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
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

#include <stdlib.h>
#include <stdio.h>
#define __PAINTPOLYQUAD_BEGINX__TRI\
  int dminx = pminx;\
  int dmaxx = pmaxx;\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth];\
  int xp[3]; int xc=0;\
  if ((y>=vk0->sy)&&(y<vk1->sy)) xp[xc++]=(vk1->sx-vk0->sx)*(y-vk0->sy)/(vk1->sy-vk0->sy)+vk0->sx;\
  if ((y>=vk0->sy)&&(y<vk2->sy)) xp[xc++]=(vk2->sx-vk0->sx)*(y-vk0->sy)/(vk2->sy-vk0->sy)+vk0->sx;\
  if ((y>=vk1->sy)&&(y<vk2->sy)) xp[xc++]=(vk2->sx-vk1->sx)*(y-vk1->sy)/(vk2->sy-vk1->sy)+vk1->sx;\
  if (xc == 2) {\
    if (xp[1]<xp[0]) {int k = xp[0]; xp[0] = xp[1]; xp[1] = k;}\
    xp[0]-=2;\
    xp[1]+=2;\
    if (xp[1]>dminx&&xp[1]<dmaxx) {dmaxx = xp[1];}\
    if (xp[0]>dminx&&xp[0]<dmaxx) {\
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

bool glCCW(const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  const float dx0 = v1->sx - v0->sx;
  const float dy0 = v1->sy - v0->sy;
  const float dx1 = v2->sx - v0->sx;
  const float dy1 = v2->sy - v0->sy;
  return (dx0*dy1 - dy0*dx1)>=0;
}

__inline bool glCheckDepthFunction(float depthBufferValue, float newValue, int depthFunction) {
  switch(depthFunction) {
  case GL_NEVER: {return false;} break;
  case GL_LESS: {return newValue < depthBufferValue;} break;
  case GL_EQUAL: {return newValue == depthBufferValue;} break;
  case GL_LEQUAL: {return newValue <= depthBufferValue;} break;
  case GL_GREATER: {return newValue > depthBufferValue;} break;
  case GL_NOTEQUAL: {return newValue != depthBufferValue;} break;
  case GL_GEQUAL: {return newValue >= depthBufferValue;} break;
  case GL_ALWAYS: {return true;} break;
  }
  return true;
}


__inline int textureWrap(float tx, int siz, GLenum filterMode) {
  int t = (int)FLOOR(tx); // pseudo floor is a trunc directly below 0 makes a seam but is much faster
  if (t<siz&&t>=0) return t;
  switch(filterMode) {
  case GL_MIRRORED_REPEAT: {
      t = t<0?(siz*2-1-((-t)%(siz*2))):(t%(siz*2));
      if (t < siz) return t;
      return siz - 1 - (t - siz);
    } break;
  case GL_CLAMP_TO_EDGE: {
      return t<0?0:(t>siz-1?siz-1:t);
    } break;
  case GL_CLAMP_TO_BORDER: {
      return t<0?-1:(t>siz-1?-1:t);
    } break;
  }
  return t<0?(siz-1-((-t)%siz)):(t%siz); // GL_REPEAT
}

__inline unsigned int doBlend(unsigned int dest, unsigned int source, int sourceFunc, int destFunc, unsigned int constantColor, int blendEquation) {

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
    Sr = (Sr*sr)/255;
    Sg = (Sg*sg)/255;
    Sb = (Sb*sb)/255;
    Sa = (Sa*sa)/255;
  }
  if (!fullD) {
    Dr = (Dr*dr)/255;
    Dg = (Dg*dg)/255;
    Db = (Db*db)/255;
    Da = (Da*da)/255;
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

__inline double __clamp__(double v, double a, double b) {
  return v<a?a:(v>b?b:v);
}

#define __CLIP_FAR_PLANE__ if (v0->sz>1.0&&v1->sz>1.0&&v2->sz>1.0) return;

void glDrawTrianglePrecise(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2) {
  bool forceWrapRepeat = false;

  if (context->isEnabled(GL_CULL_FACE)) {
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
  combineIntoWindow(glClipRectX0,glClipRectY0,glClipRectX1,glClipRectY1,context->viewportX0,context->viewportY0,context->viewportX1,context->viewportY1);
  if (context->isEnabled(GL_SCISSOR_TEST)) {
    combineIntoWindow(glClipRectX0,glClipRectY0,glClipRectX1,glClipRectY1,context->scissorX0,context->scissorY0,context->scissorX1,context->scissorY1);
  }
  __POLYCLIP__
  if (fullyClipped) 
    return;

  bool stencil = context->isEnabled(GL_STENCIL_TEST); // not supported
  bool blending = context->isEnabled(GL_BLEND);
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
  bool textured = context->isEnabled(GL_TEXTURE_2D) && t->data != NULL && t->name != 0;
  bool filtering = (t->magFilter != GL_NEAREST)&&(t->magFilter != GL_NEAREST_MIPMAP_NEAREST)&&(t->magFilter != GL_NEAREST_MIPMAP_LINEAR);
  float tx0,ty0,tz0,tw0;
  float tx1,ty1,tz1,tw1;
  float tx2,ty2,tz2,tw2;
  float tx3,ty3,tz3,tw3;
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


    tx0 = v0->textureX;
    ty0 = v0->textureY;
  

    tx1 = v1->textureX;
    ty1 = v1->textureY;

    tx2 = v2->textureX;
    ty2 = v2->textureY;

    if (context->isEnabled(GL_TEXTURE_GEN_S)||context->isEnabled(GL_TEXTURE_GEN_T)) {
      fixSphereMapUV(context, tx0,ty0,tx1,ty1);
      fixSphereMapUV(context, tx0,ty0,tx2,ty2);
      forceWrapRepeat = true;
    }

    tx0 = (tx0*twidth0+scx)*v0w;
    ty0 = (ty0*theight0+scy)*v0w;
  

    tx1 = (tx1*twidth0+scx)*v1w;
    ty1 = (ty1*theight0+scy)*v1w;

    tx2 = (tx2*twidth0+scx)*v2w;
    ty2 = (ty2*theight0+scy)*v2w;

    borderColor = ((int)FLOOR(glClampf(t->borderColorRed*255.f,0.f,255.f)));
    borderColor |= ((int)FLOOR(glClampf(t->borderColorGreen*255.f,0.f,255.f)))<<8;
    borderColor |= ((int)FLOOR(glClampf(t->borderColorBlue*255.f,0.f,255.f)))<<16;
    borderColor |= ((int)FLOOR(glClampf(t->borderColorAlpha*255.f,0.f,255.f)))<<24;

    if ((filtering) && (t->minFilter == GL_NEAREST)||(t->minFilter==GL_NEAREST_MIPMAP_NEAREST)||(t->minFilter==GL_NEAREST_MIPMAP_LINEAR)) {
      const float tw = twidth0;
      const float th = theight0;
      const float lengtht0 = sqrt((v1->textureX-v0->textureX)*(v1->textureX-v0->textureX)*tw*tw+(v1->textureY-v0->textureY)*(v1->textureY-v0->textureY)*th*th);
      const float lengtht1 = sqrt((v2->textureX-v0->textureX)*(v2->textureX-v0->textureX)*tw*tw+(v2->textureY-v0->textureY)*(v2->textureY-v0->textureY)*th*th);
      const float lengtht2 = sqrt((v2->textureX-v1->textureX)*(v2->textureX-v1->textureX)*tw*tw+(v2->textureY-v1->textureY)*(v2->textureY-v1->textureY)*th*th);
      const float lengths0 = sqrt((v1->sx-v0->sx)*(v1->sx-v0->sx)+(v1->sy-v0->sy)*(v1->sy-v0->sy));
      const float lengths1 = sqrt((v2->sx-v0->sx)*(v2->sx-v0->sx)+(v2->sy-v0->sy)*(v2->sy-v0->sy));
      const float lengths2 = sqrt((v2->sx-v1->sx)*(v2->sx-v1->sx)+(v2->sy-v1->sy)*(v2->sy-v1->sy));
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
  bool depthTest = context->isEnabled(GL_DEPTH_TEST);
  int alphaFunction = context->alphaFunc;
  int alphaRef = (int)FLOOR(context->alphaFuncRef*255.f);
  bool alphaTest = context->isEnabled(GL_ALPHA_TEST);

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
  if ( (int)FLOOR(r0)==(int)FLOOR(r1)&&(int)FLOOR(r1)==(int)FLOOR(r2)
    &&(int)FLOOR(g0)==(int)FLOOR(g1)&&(int)FLOOR(g1)==(int)FLOOR(g2)
    &&(int)FLOOR(b0)==(int)FLOOR(b1)&&(int)FLOOR(b1)==(int)FLOOR(b2)
    &&(int)FLOOR(a0)==(int)FLOOR(a1)&&(int)FLOOR(a1)==(int)FLOOR(a2)) {
    interpolateColor = false;
    rf = r0;
    gf = g0;
    bf = b0;
    af = a0;
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


  const bool fogging = context->isEnabled(GL_FOG);
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
      srf = sr0;
      sgf = sg0;
      sbf = sb0;
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

  int explicitAlphaValue = (int)FLOOR(context->explicitAlpha * 255.f);
  if (explicitAlphaValue<0) explicitAlphaValue=0;
  if (explicitAlphaValue>255) explicitAlphaValue=255;

  const unsigned int wrapS = forceWrapRepeat ? GL_REPEAT : t->wrapS;
  const unsigned int wrapT = forceWrapRepeat ? GL_REPEAT : t->wrapT;

  dontPaint = false;
  __PAINTPOLYQUAD_BEGINY__
  __PAINTPOLYQUAD_INITBARYFORX__
  if (dontPaint) return;
  __PAINTPOLYQUAD_BEGINX__
      if (bary0>=0&&bary1>=0&&bary2>=0) {
        double zp = __BARY0__(v0z)+__BARY1__(v1z)+__BARY2__(v2z);
        if ((!depthTest)||glCheckDepthFunction(*zDest,zp,depthFunction)) {
          if (wValue) iw = 1.0/(__BARY0__(v0w)+__BARY1__(v1w)+__BARY2__(v2w));
          if (interpolateColor) {
            r = (__BARY0__(r0)+__BARY1__(r1)+__BARY2__(r2))*iw;
            g = (__BARY0__(g0)+__BARY1__(g1)+__BARY2__(g2))*iw;
            b = (__BARY0__(b0)+__BARY1__(b1)+__BARY2__(b2))*iw;
            a = (__BARY0__(a0)+__BARY1__(a1)+__BARY2__(a2))*iw;
          } else {
            r = rf;
            g = gf;
            b = bf;
            a = af;
          }
          if (textured) {
            float tx = (__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw;
            float ty = (__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw;
            int tix0 = textureWrap(tx, twidth0, wrapS);
            int tiy0 = textureWrap(ty, theight0, wrapT);
            unsigned int rgba = 0xffffffff;
            if (filtering) {
              int tix1 = textureWrap(tx+1, twidth0, wrapS);
              int tiy1 = textureWrap(ty+1, theight0, wrapT);
              int txf = (int)FLOOR(fmod(tx*256.f,256.f));
              int tyf = (int)FLOOR(fmod(ty*256.f,256.f));
              if (txf<0) txf = 256+txf;
              if (tyf<0) tyf = 256+tyf;
              int p1v = ((256-txf)*(256-tyf))>>8; 
              int p2v = ((txf)*(256-tyf))>>8; 
              int p3v = ((txf)*(tyf))>>8; 
              int p4v = ((256-txf)*(tyf))>>8;
              tiy0 *= twidth0;
              tiy1 *= twidth0;
              unsigned int rgba00 = tix0>=0&&tiy0>=0?tdata0[tix0+tiy0]:borderColor;
              unsigned int rgba10 = tix1>=0&&tiy0>=0?tdata0[tix1+tiy0]:borderColor;
              unsigned int rgba11 = tix1>=0&&tiy1>=0?tdata0[tix1+tiy1]:borderColor;
              unsigned int rgba01 = tix0>=0&&tiy1>=0?tdata0[tix0+tiy1]:borderColor;
              rgba = (((rgba00>>8) & 0x00ff00ff)*p1v)&0xff00ff00;
              rgba += (((rgba10>>8) & 0x00ff00ff)*p2v)&0xff00ff00;
              rgba += (((rgba11>>8) & 0x00ff00ff)*p3v)&0xff00ff00;
              rgba += (((rgba01>>8) & 0x00ff00ff)*p4v)&0xff00ff00;
              rgba += (((rgba00 & 0x00ff00ff)*p1v)>>8)&0x00ff00ff;
              rgba += (((rgba10 & 0x00ff00ff)*p2v)>>8)&0x00ff00ff;
              rgba += (((rgba11 & 0x00ff00ff)*p3v)>>8)&0x00ff00ff;
              rgba += (((rgba01 & 0x00ff00ff)*p4v)>>8)&0x00ff00ff;
            } else {
              if (tix0>=0&&tiy0>=0) {
                rgba  = tdata0[tix0+tiy0*twidth0];
              } else {
                rgba = borderColor;
              }
            }
            if (texEnvMode==GL_MODULATE) {
              r *= rgba & 255;
              g *= (rgba >> 8) & 255;
              b *= (rgba >> 16) & 255;
              a *= (rgba >> 24) & 255;
              r /= 255;
              g /= 255;
              b /= 255;
              a /= 255;
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
                  r /= 255;
                  g /= 255;
                  b /= 255;
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
                  r /= 255;
                  g /= 255;
                  b /= 255;
                  a /= 255;
                } break;
              }
            }
          }
               
          if (separateSpecular) {
            if (fogging) {
              const double f = (__BARY0__(f0)+__BARY1__(f1)+__BARY2__(f2))*iw;
              r *= f;
              g *= f;
              b *= f;
            }
            if (interpolateSpecular) {
              r += (__BARY0__(sr0)+__BARY1__(sr1)+__BARY2__(sr2))*iw;
              g += (__BARY0__(sg0)+__BARY1__(sg1)+__BARY2__(sg2))*iw;
              b += (__BARY0__(sb0)+__BARY1__(sb1)+__BARY2__(sb2))*iw;
            } else {
              r += srf;
              g += sgf;
              b += sbf;
            }
            if (r>255) r = 255;
            if (g>255) g = 255;
            if (b>255) b = 255;
          }

          bool writePixel = true;
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
          if (writePixel) {
            if (writeDepth) 
              *zDest=zp;
            if (!blending)
              if (notMasked) {
                *pDest=r|(g<<8)|(b<<16)|(a<<24);
              } else {
                unsigned char *pDest2 = (unsigned char *)pDest;
                if (maskRed) pDest2[0] = r;
                if (maskGreen) pDest2[1] = g;
                if (maskBlue) pDest2[2] = b;
                if (maskAlpha) pDest2[3] = a;
              }
            else {
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
                    c[0] += ((r-c[0])*a8)>>16;
                    c[1] += ((g-c[1])*a8)>>16;
                    c[2] += ((b-c[2])*a8)>>16;
                    c[3] += ((a-c[3])*a8)>>16;    
                  }
                } else {
                  *pDest=doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                }
              } else {
                unsigned int k = doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                unsigned char *pDest2 = (unsigned char *)pDest;
                unsigned char *k2 = (unsigned char*)k;
                if (maskRed) pDest2[0] = k2[0];
                if (maskGreen) pDest2[1] = k2[1];
                if (maskBlue) pDest2[2] = k2[2];
                if (maskAlpha) pDest2[3] = k2[3];
              }
            }
            if (useExplicitAlpha) {
              ((unsigned char *)pDest)[3] = explicitAlphaValue;
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
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
// ---- DPMI for VESA ---
#include <i86.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
int glGraphicsModeToRestore=0x02;
void setBiosGraphicsMode(int mode) {
    union REGS regs;
    regs.x.eax = mode;
    int386(0x10, &regs, &regs);
}

int getBiosGraphicsMode() {
    union REGS regs;
    regs.w.ax = 0x0f00;
    int386 (0x10, &regs, &regs);
    return regs.h.al;
}
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
#define OUT(reg) out->##reg = rmregs.e##reg
int VESABUF_sel=0x00;
int VESABUF_rseg=0x00;
int VESABUF_len=1024;
void DPMI_allocRealSeg(int size, int *sel, int *r_seg) {
  union REGS r;
  r.w.ax = 0x100;
  r.w.bx = (size+0x0f)>>4;
  int386(0x31,&r,&r);
  if (r.w.cflag)
    exit(0);
  *sel = r.w.dx;
  *r_seg = r.w.ax;
}
void DPMI_freeRealSeg(unsigned sel) {
  union REGS r;
  r.w.ax = 0x101;
  r.w.dx = sel;
  int386(0x31,&r,&r);
}
static void ExitVBEBuf() {
  DPMI_freeRealSeg(VESABUF_sel);
}
short DPMI_int386(int intno, RMREGS *in, RMREGS *out) {
  _RMREGS rmregs;
  union REGS r;
  struct SREGS sr;

  memset(&rmregs,0,sizeof(rmregs));
  IN(ax); IN(bx); IN(cx); IN(dx); IN(si); IN(di);

  segread(&sr);
  r.w.ax = 0x300;
  r.h.bl = intno;
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
  r.h.bl = intno;
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
VBE_initRMBuf() {
  if (!VESABUF_sel) {
    DPMI_allocRealSeg(VESABUF_len,&VESABUF_sel,&VESABUF_rseg);
    atexit(ExitVBEBuf);
  }
}
void VBE_callESDI(RMREGS *regs, void *buffer, int size) {
  RMSREGS sregs;
  VBE_initRMBuf();
  sregs.es = VESABUF_rseg;
  regs->di = 0;
  _fmemcpy(MK_FP(VESABUF_sel,0),buffer,size);
  DPMI_int386x(0x10,regs,regs,&sregs);
  _fmemcpy(buffer,MK_FP(VESABUF_sel,0),size);
  //ExitVBEBuf();
}
uint64_t mapPhysicalToLinear(unsigned int physicalAddress, unsigned int size) {
  union REGS r;
  r.w.ax = 0x800;
  r.w.bx = physicalAddress>>16;
  r.w.cx = physicalAddress & 0xffff;
  r.w.si = size >> 16;
  r.w.di = size & 0xffff;
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return 0;
  return (((uint64_t)r.w.bx)<<16)+r.w.cx;
}
bool unmapPhysical(unsigned int linearAddress) {
  union REGS r;
  r.w.ax = 0x801;
  r.w.bx = linearAddress>>16;
  r.w.cx = linearAddress & 0xffff;
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return false;
  return true;
}
// --------------------------------------
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
// --------------------------------------
void *glVesaPointer(const void *d) {
  int segment = ((unsigned int)d) >> 16;
  int offset = ((unsigned int)d) & 0xffff;
  return (void*)(((unsigned int)segment<<4)+offset);
}
// --------------------------------------
// --------------------------------------
// --------------------------------------
void glSetupMouse();
double glStartClock;
GLboolean glVesa(int xRes,int yRes, int bPP) {
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
  uint16_t *w=new uint16_t[v-u];
  memcpy(w,u,((unsigned int)v)-((unsigned int)u));
  v = w;
  while(*v != 0xffff) {
    int mode = *v;
    regs.ax = 0x4f01;
    regs.cx = mode;
    VBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP) {
      regs.ax = 0x4f02;
      regs.bx = mode|0x4000;
      DPMI_int386(0x10,&regs,&regs);
      glFrameBufferWidth = xRes;
      glFrameBufferHeight = yRes;
      glFrameBufferDedicated = (unsigned int*)mapPhysicalToLinear(modeInfo.linearFrameBuffer,4096*1024-1);
      glFrameBuffer = new unsigned int[glFrameBufferWidth*glFrameBufferHeight];
      glDepthBuffer = new float[glFrameBufferWidth*glFrameBufferHeight];
      memset(glFrameBufferDedicated,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      glFrameBufferWidth0 = glFrameBufferWidth;
      glFrameBufferHeight0 = glFrameBufferHeight;
      glFrameBuffer0 = glFrameBuffer;
      glDepthBuffer0 = glDepthBuffer;
      glSetupMouse();
      glStartClock = clock();
      glViewport(0,0,xRes,yRes);
      return true;
    }
    v++;
  }
  delete[] w;
  return false;
}
static void setBiosCursor(int x, int y) {
    union REGS regs;
    regs.x.eax = 0x200;
    regs.h.bh = 0;
    regs.h.dl = x;
    regs.h.dh = y;
    int386(0x10, &regs, &regs);
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
static unsigned int getPalette(int index) {
  outp(0x3c8,index);
  int r = inp(0x3c9);
  int g = inp(0x3c9);
  int b = inp(0x3c9);
  return 0xff000000|(r<<2)|(g<<10)|(b<<18);
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
GLboolean glVGA() {
  glGraphicsModeToRestore = getBiosGraphicsMode();
  setBiosGraphicsMode(0x13);
  setModeX();
  set400Lines();
  setHiColorPalette();
  hiColor = true;
  otherFrame = false;
  unsigned char *vga = (unsigned char*)(0xa0000);
  memset(vga,0,0x10000);
  setVGABufferStart(otherFrame ? 0x8000 : 0x0000);
  int xRes = 320;
  int yRes = 200;
  glFrameBufferWidth = xRes;
  glFrameBufferHeight = yRes;
  glFrameBuffer = new unsigned int[glFrameBufferWidth*glFrameBufferHeight];
  glDepthBuffer = new float[glFrameBufferWidth*glFrameBufferHeight];
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
  memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
  glFrameBufferWidth0 = glFrameBufferWidth;
  glFrameBufferHeight0 = glFrameBufferHeight;
  glFrameBuffer0 = glFrameBuffer;
  glDepthBuffer0 = glDepthBuffer;
  glSetupMouse();
  glStartClock = clock();
  glViewport(0,0,xRes,yRes);
  for (int i = 0; i < 256; i++) {
    hiRedTab[i] = 128+(((i>>4)&15)<<3);
  }
  return true;
}
GLboolean glDirect(GLuint *frameBuffer, GLfloat *depthBuffer, GLuint width, GLuint height) {
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
  glStartClock = clock();
  glViewport(0,0,width,height);
  return true;
}
void glRefreshHiColor() {
  unsigned char *vga = (unsigned char*)(0xa0000+(otherFrame?0x8000:0x00));
  for (int p = 0; p < 4; p++) {
    outp(0x3c4,0x02);
    outp(0x3c5,1<<p);
    unsigned char *s = (unsigned char*)&glFrameBuffer[p];
    for (int y = 0; y < 200; y++) {
      unsigned char *w1 = &vga[y*(320/4*2)];
      unsigned char *w2 = &vga[y*(320/4*2)+320/4];
      for (int x = 320/4; x > 0; x--) {
        *w2++ = s[1]>>2;
        *w1++ = (s[2]>>5)|hiRedTab[s[0]];
        s += 16;
      }
    }
  }
  setVGABufferStart(otherFrame ? 0x8000 : 0x0000);
  otherFrame = !otherFrame;   
}
void glRefresh() {
  if (hiColor) {glRefreshHiColor();return;}
  unsigned int rgba;
  for (int i = 0; i < glFrameBufferWidth0*glFrameBufferHeight0; i++) {
    rgba = glFrameBuffer0[i];
    glFrameBufferDedicated[i] = (rgba & 0xff00ff00)|((rgba>>16)&0xff)|((rgba<<16)&0x00ff0000);
  }
}
void glDone() {
  if (glFrameBuffer0!=NULL) delete[] glFrameBuffer0;
  if (glDepthBuffer0!=NULL) delete[] glDepthBuffer0;
  //if (glSencilBuffer0!=NULL) delete[] glStencilBuffer0;
  if (!hiColor) unmapPhysical((unsigned int)glFrameBufferDedicated);
  setBiosGraphicsMode(glGraphicsModeToRestore);
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
  return glContext.zoomX;
}

float glGetZoomY() {
  return glContext.zoomY;
}

void glExplicitAlpha(bool useExplicitAlpha, GLfloat alpha) {
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
    glPixelLastSX = v.sx;
    glPixelLastSY = v.sy;
  }
  if (glPixelLastValid)
  {
    int glClipRectX0 = 0;
    int glClipRectY0 = 0;
    int glClipRectX1 = glFrameBufferWidth;
    int glClipRectY1 = glFrameBufferHeight;
    combineIntoWindow(glClipRectX0,glClipRectY0,glClipRectX1,glClipRectY1,glContext.viewportX0,glContext.viewportY0,glContext.viewportX1,glContext.viewportY1);
    if (glContext.isEnabled(GL_SCISSOR_TEST)) {
      combineIntoWindow(glClipRectX0,glClipRectY0,glClipRectX1,glClipRectY1,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
    }
    int px = glPixelLastSX+x;
    int py = glPixelLastSY+y;
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


int glKey = 0;

unsigned short glNextKey() {
  union REGS regs;
  regs.x.eax = 0x0600;
  regs.x.edx = 0xff;
  int386(0x21, &regs, &regs);
  glKey = regs.x.eax & 0xff;
  if (glKey == 0) {
    regs.x.eax = 0x0600;
    regs.x.edx = 0xff;
    int386(0x21, &regs, &regs);
    glKey += (regs.x.eax & 0xff)<<8;
  }
  return glKey;
}

int glLastMouseX = 0;
int glLastMouseY = 0;
int glLastMouseB = 0;
int glMouseX = 0;
int glMouseY = 0;
int glMouseB = 0;

void glSetMousePos(int x, int y) {
  union REGS regs;
  regs.x.eax = 0x04;
  regs.x.ecx = x;
  regs.x.edx = y;
  int386(0x33, &regs, &regs);
}

void glNextMouseDelta(double *mouseX, double *mouseY) {
  union REGS regs;
  regs.x.eax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseX = glMouseX;
  glLastMouseY = glMouseY;
  glMouseX = regs.x.ecx & 0xffff;
  glMouseY = regs.x.edx & 0xffff;
  *mouseX = glMouseX - glLastMouseX;
  *mouseY = glMouseY - glLastMouseY;
  if (true) {
    glSetMousePos(glFrameBufferWidth0/2, glFrameBufferHeight0/2);
    // update lastMouseX this way
    glMouseX = glFrameBufferWidth0/2;
    glMouseY = glFrameBufferHeight0/2;
  }
}


char glMouseButtons() {
  union REGS regs;
  regs.x.eax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseB = glMouseB;
  glMouseB = regs.x.ebx & 0xffff;
  return glMouseB & 0xff;
}

void glSetupMouse() {
    union REGS regs;
    regs.x.eax = 0x07;
    regs.x.ecx = 0;
    regs.x.edx = glFrameBufferWidth0-1;
    int386(0x33, &regs, &regs);
    regs.x.eax = 0x08;
    regs.x.ecx = 0;
    regs.x.edx = glFrameBufferHeight0-1;
    int386(0x33, &regs, &regs);
    glMouseX = glFrameBufferWidth0/2;
    glMouseY = glFrameBufferHeight0/2;
    glSetMousePos(glMouseX, glMouseY);
}

void glSpecialKeys(bool *shiftKey, bool *ctrlKey, bool *altKey) {
  int keyState = *((char *)0x417);
  *shiftKey = (keyState & 0x03) != 0;
  *ctrlKey = (keyState & 0x04) != 0;
  *altKey = (keyState & 0x08) != 0;
}

double glSeconds() {
  return (double)(clock() - glStartClock) / CLOCKS_PER_SEC;
}

void glSetTime(double seconds) {
  glStartClock = clock() - seconds * CLOCKS_PER_SEC;
}

void glTexturePointer(int width, int height, unsigned int *textureData) {
  if (glContext.boundTextures[glContext.activeTexture]==0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    delete[] t->data;
    t->data = NULL;
  }
  t->data = textureData;
  t->width = width;
  t->height = height;
}

void glSetRenderTarget(GLuint *frameBufferOrNULL, GLfloat *depthBuffer, GLuint width, GLuint height) {
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
}

GLuint glGetTextureWidth(GLuint textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return 0;
  return t->width;

}

GLuint glGetTextureHeight(GLuint textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return 0;
  return t->height;
}

GLuint *glGetTexturePointer(GLuint textureId) {
  glTexture *t = &glTextures[textureId];
  if (t->name == 0x00) return NULL;
  return t->data;
}

// --------------------------------------
// --------------------------------------
// --------------------------------------

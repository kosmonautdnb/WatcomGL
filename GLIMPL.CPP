#include "gl.h"
#include <string.h>
#include <math.h>
#include <stdio.h> //!

int glFrameBufferWidth = 0;
int glFrameBufferHeight = 0;
unsigned int *glFrameBuffer = NULL;
unsigned int *glFrameBufferDedicated = NULL;
int *glStencilBuffer = NULL;
float *glDepthBuffer = NULL;

GLfloat identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};

#define __GL_MAX_LIGHTS__ (2)
#define __GL_MAX_TEXTURE_SIZE__ (1024)
#define GLMAXTEXTURES 1024
#define GLMAXTEXTUREUNITS 8

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

class _GLContext {
public:
  _GLContext() {
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
    cullFaceMode = 0;
    depthFunc = GL_LESS;
    depthMask = true;
    depthRangeZNear = 0;
    depthRangeZFar = 0;
    pointSize = 0;
    polygonOffsetFactor = 0;
    polygonOffsetUnits = 0;
    memcpy(matrixForMode[0],identityMatrix,sizeof(matrixForMode));
    memcpy(matrixForMode[1],identityMatrix,sizeof(matrixForMode));
    memcpy(matrix,identityMatrix,sizeof(matrixForMode));
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
    normalX = 0.f;
    normalY = 0.f;
    normalZ = 0.f;
    vertexX = 1.f;
    vertexY = 1.f;
    vertexZ = 1.f;
    vertexW = 1.f;
    textureX = 0.f;
    textureY = 0.f;
    textureZ = 0.f;
    textureW = 0.f;
    beginMode = 0;
    colorMaterial = GL_AMBIENT_AND_DIFFUSE;

    // ambient
    materialRed[0] = 0;
    materialGreen[0] = 0;
    materialBlue[0] = 0;
    materialAlpha[0] = 0;
    // diffuse
    materialRed[1] = 1;
    materialGreen[1] = 1;
    materialBlue[1] = 1;
    materialAlpha[1] = 1;
    // specular
    materialRed[2] = 0;
    materialGreen[2] = 0;
    materialBlue[2] = 0;
    materialAlpha[2] = 0;
    // emission
    materialRed[3] = 0;
    materialGreen[3] = 0;
    materialBlue[3] = 0;
    materialAlpha[3] = 0;
    // shininess
    materialRed[4] = 1;
    materialGreen[4] = 1;
    materialBlue[4] = 1;
    materialAlpha[4] = 1;

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
    }
    for (i = 0; i < GLMAXTEXTUREUNITS; i++) {
      boundTextures[i] = 0;
    }
    blendColorRed = 0;
    blendColorGreen = 0;
    blendColorBlue = 0;
    blendColorAlpha = 1;
    blendEquation = GL_FUNC_ADD;
  }
  void enable(GLenum prop, bool enable) {enabledCaps[prop&255]=enable;}
  GLboolean isEnabled(GLenum prop) {return enabledCaps[prop&255];}
  void enableClientState(GLenum prop, bool enable) {;}
  GLboolean getBoolean(GLenum prop) {return true;}
  GLubyte *getString(GLenum prop) {return "";}
  GLenum error;
  GLint viewportX0;
  GLint viewportY0;
  GLsizei viewportX1;
  GLsizei viewportY1;
  GLclampf clearRed;
  GLclampf clearGreen;
  GLclampf clearBlue;
  GLclampf clearAlpha;
  GLclampf clearDepth;
  GLint clearStencil;
  GLenum activeTexture;
  GLenum alphaFunc;
  GLenum alphaFuncRef;
  GLenum blendFuncSFactor;
  GLenum blendFuncDFactor;
  GLenum cullFaceMode;
  GLenum depthFunc;
  GLboolean depthMask;
  GLclampf depthRangeZNear;
  GLclampf depthRangeZFar;
  GLfloat pointSize;
  GLfloat polygonOffsetFactor;
  GLfloat polygonOffsetUnits;
  GLfloat matrixForMode[2][4*4];
  GLfloat matrix[4*4];
  GLint matrixModeNr;
  GLfloat lineStippleFactor;
  GLshort lineStipplePattern;
  GLfloat lineWidth;
  GLint scissorX0;
  GLint scissorY0;
  GLint scissorX1;
  GLint scissorY1;
  GLenum shadeMode;
  GLenum stencilFunc;
  GLint stencilFuncRef;
  GLuint stencilFuncMask;
  GLuint stencilMask;
  GLenum stencilOpFail;
  GLenum stencilOpZFail;
  GLenum stencilOpZPass;
  GLfloat colorRed;
  GLfloat colorGreen;
  GLfloat colorBlue;
  GLfloat colorAlpha;
  GLfloat normalX;
  GLfloat normalY;
  GLfloat normalZ;
  GLfloat vertexX;
  GLfloat vertexY;
  GLfloat vertexZ;
  GLfloat vertexW;
  GLfloat textureX;
  GLfloat textureY;
  GLfloat textureZ;
  GLfloat textureW;
  GLenum beginMode;
  GLboolean enabledCaps[256];
  GLfloat lightRed[4][__GL_MAX_LIGHTS__];
  GLfloat lightGreen[4][__GL_MAX_LIGHTS__];
  GLfloat lightBlue[4][__GL_MAX_LIGHTS__];
  GLfloat lightAlpha[4][__GL_MAX_LIGHTS__];
  GLenum colorMaterial;
  GLfloat materialRed[5];
  GLfloat materialGreen[5];
  GLfloat materialBlue[5];
  GLfloat materialAlpha[5];
  GLuint boundTextures[GLMAXTEXTUREUNITS];
  GLfloat blendColorRed;
  GLfloat blendColorGreen;
  GLfloat blendColorBlue;
  GLfloat blendColorAlpha;
  GLenum blendEquation;
};

_GLContext glContext;
GLfloat glMatrixStack[16][4*4];
GLint glMatrixStackPos = 0;

typedef struct glVertex {
  GLfloat colorRed;
  GLfloat colorGreen;
  GLfloat colorBlue;
  GLfloat colorAlpha;
  GLfloat normalX;
  GLfloat normalY;
  GLfloat normalZ;
  GLfloat vertexX;
  GLfloat vertexY;
  GLfloat vertexZ;
  GLfloat vertexW;
  GLfloat textureX;
  GLfloat textureY;
  GLfloat textureZ;
  GLfloat textureW;
  float sx,sy,sz,sw;
} glVertex;

glVertex glVertices[4];
int glCurrentVertexElement = 0;

void glDrawTriangle(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2);
void glDrawQuad(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3);

typedef struct glTexture {
  GLuint name;
  GLuint width;
  GLuint height;
  GLuint *data;
  GLuint baseLevel;
  GLuint lodBias;
  GLuint magFilter;
  GLuint maxLevel;
  GLuint maxLod;
  GLuint minFilter;
  GLuint minLod;
  GLuint wrapS;
  GLuint wrapT;
  GLuint wrapR;
  GLfloat borderColorRed;
  GLfloat borderColorGreen;
  GLfloat borderColorBlue;
  GLfloat borderColorAlpha;
  GLuint texEnvMode;
} glTexture;
glTexture glTextures[GLMAXTEXTURES] = {
  {0,0,0,NULL,0,0,GL_LINEAR,1000,1000,GL_NEAREST,0,GL_REPEAT,GL_REPEAT,GL_REPEAT,1.f,0.f,1.f,1.f,GL_MODULATE}
};

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

void combineIntoWindow(GLint &x0, GLint &y0, GLint &x1, GLint &y1,GLint nx0, GLint ny0, GLint nx1, GLint ny1) {
  x0 = nx0<x0?x0:nx0;
  y0 = ny0<y0?y0:ny0;
  x1 = nx1>x1?x1:nx1;
  y1 = ny1>y1?y1:ny1;
}

void glMatMul(const GLfloat *ma, const GLfloat *mb, GLfloat *r) {
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

void glUpdateMatrix() {
  glMatMul(glContext.matrixForMode[GL_PROJECTION & 1],glContext.matrixForMode[GL_MODELVIEW & 1],glContext.matrix);
}

int glTransformVertex(_GLContext *context, glVertex *v, bool clip) {
  float x = v->vertexX * context->matrix[0*4+0] + v->vertexY * context->matrix[1*4+0] + v->vertexZ * context->matrix[2*4+0] + v->vertexW * context->matrix[3*4+0];
  float y = v->vertexX * context->matrix[0*4+1] + v->vertexY * context->matrix[1*4+1] + v->vertexZ * context->matrix[2*4+1] + v->vertexW * context->matrix[3*4+1];
  float z = v->vertexX * context->matrix[0*4+2] + v->vertexY * context->matrix[1*4+2] + v->vertexZ * context->matrix[2*4+2] + v->vertexW * context->matrix[3*4+2];
  float w = v->vertexX * context->matrix[0*4+3] + v->vertexY * context->matrix[1*4+3] + v->vertexZ * context->matrix[2*4+3] + v->vertexW * context->matrix[3*4+3];

  if (clip) {
    if (z < -w) {
      return 1;
    }
  }
  if (w == 0) 
    return 1; // should never happen
     
  v->sx = x/w;
  v->sy = y/w;
  v->sz = z/w;
  v->sw = w;

  v->sx *= (context->viewportX1-context->viewportX0)*0.5;
  v->sy *= (context->viewportY1-context->viewportY0)*-0.5;
  v->sx += (context->viewportX0+context->viewportX1)*0.5;
  v->sy += (context->viewportY0+context->viewportY1)*0.5;
  return 0;
}

void glLightVertex(_GLContext *context, glVertex *v) {
  if (context->isEnabled(GL_LIGHTING)) {
    float *matrix = context->matrixForMode[GL_MODELVIEW & 1];
    float x = v->vertexX * matrix[0*4+0] + v->vertexY * matrix[1*4+0] + v->vertexZ * matrix[2*4+0] + v->vertexW * matrix[3*4+0];
    float y = v->vertexX * matrix[0*4+1] + v->vertexY * matrix[1*4+1] + v->vertexZ * matrix[2*4+1] + v->vertexW * matrix[3*4+1];
    float z = v->vertexX * matrix[0*4+2] + v->vertexY * matrix[1*4+2] + v->vertexZ * matrix[2*4+2] + v->vertexW * matrix[3*4+2];
    float w = v->vertexX * matrix[0*4+3] + v->vertexY * matrix[1*4+3] + v->vertexZ * matrix[2*4+3] + v->vertexW * matrix[3*4+3];
    if (w != 0) { // should never happen
      x /= w;
      y /= w;
      z /= w;
    }
    float ambientRed = 0;
    float ambientGreen = 0;
    float ambientBlue = 0;
    float diffuseRed = 0;
    float diffuseGreen = 0;
    float diffuseBlue = 0;
    float specularRed = 0;
    float specularGreen = 0;
    float specularBlue = 0;

    float materialAmbientRed = context->materialRed[0];
    float materialAmbientGreen = context->materialGreen[0];
    float materialAmbientBlue = context->materialBlue[0];
    float materialDiffuseRed = context->materialRed[1];
    float materialDiffuseGreen = context->materialGreen[1];
    float materialDiffuseBlue = context->materialBlue[1];
    float materialSpecularRed = context->materialRed[2];
    float materialSpecularGreen = context->materialGreen[2];
    float materialSpecularBlue = context->materialBlue[2];
    float materialEmissionRed = context->materialRed[3];
    float materialEmissionGreen = context->materialGreen[3];
    float materialEmissionBlue = context->materialBlue[3];
    float materialShininessRed = context->materialRed[4];
    float materialShininessGreen = context->materialGreen[4];
    float materialShininessBlue = context->materialBlue[4];


    if (context->isEnabled(GL_COLOR_MATERIAL)) {
      switch(glContext.colorMaterial) {
        case GL_AMBIENT: {
          materialAmbientRed = v->colorRed;
          materialAmbientGreen = v->colorGreen;
          materialAmbientBlue = v->colorBlue;
        } break;
        case GL_DIFFUSE: {
          materialDiffuseRed = v->colorRed;
          materialDiffuseGreen = v->colorGreen;
          materialDiffuseBlue = v->colorBlue;
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
        } break;
      }
    }

    float normalX = v->normalX;
    float normalY = v->normalY;
    float normalZ = v->normalZ;
    if (context->isEnabled(GL_NORMALIZE)) {
      float l = sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
      if (l>0.f) {
        normalX/=l;
        normalY/=l;
        normalZ/=l;
      }
    }

    for (int i = 0; i < __GL_MAX_LIGHTS__; i++) {
      if (context->isEnabled(GL_LIGHT0+i)) {
        float lightAmbientRed = context->lightRed[0][i];
        float lightAmbientGreen = context->lightGreen[0][i];
        float lightAmbientBlue = context->lightBlue[0][i];
        float lightDiffuseRed = context->lightRed[1][i];
        float lightDiffuseGreen = context->lightGreen[1][i];
        float lightDiffuseBlue = context->lightBlue[1][i];
        float lightSpecularRed = context->lightRed[2][i];
        float lightSpecularGreen = context->lightGreen[2][i];
        float lightSpecularBlue =  context->lightBlue[2][i];
        float lightPositionX = context->lightRed[3][i];
        float lightPositionY = context->lightGreen[3][i];
        float lightPositionZ =  context->lightBlue[3][i];
        float lightPositionW =  context->lightAlpha[3][i];
        float lVecX = lightPositionX;
        float lVecY = lightPositionY;
        float lVecZ = lightPositionZ;
        if (fabs(lightPositionW)>0.0) {
          lVecX-=x;
          lVecY-=y;
          lVecZ-=z;
        }
        float l = sqrt(lVecX*lVecX+lVecY*lVecY+lVecZ*lVecZ);
        if (l>0.f) {lVecX/=l;  lVecY/=l; lVecZ/=l;}
        float diffuse = lVecX * normalX + lVecY * normalY + lVecZ * normalZ;
        if (diffuse<0) diffuse = 0;
        float viewX = -x;
        float viewY = -y;
        float viewZ = -z;
        l = sqrt(viewX*viewX+viewY*viewY+viewZ*viewZ);
        if (l>0.f) {viewX/=l;viewY/=l;viewZ/=l;}
        float dot2NI = 2.0*(viewX*normalX+viewY*normalY+viewZ*normalZ);
        float reflectionX = viewX - dot2NI * normalX;
        float reflectionY = viewY - dot2NI * normalY;
        float reflectionZ = viewZ - dot2NI * normalZ;
        l = sqrt(reflectionX*reflectionX+reflectionY*reflectionY+reflectionZ*reflectionZ);
        if (l>0.f) {reflectionX/=l; reflectionY /= l; reflectionZ /= l;}
        float specular = lVecX * reflectionX + lVecY * reflectionY + lVecZ * reflectionZ;
        if (specular<0.0) specular = 0.0;
        ambientRed += materialAmbientRed*lightAmbientRed;
        ambientGreen += materialAmbientGreen*lightAmbientGreen;
        ambientBlue += materialAmbientBlue*lightAmbientBlue;
        diffuseRed += materialDiffuseRed*lightDiffuseRed*diffuse;
        diffuseGreen += materialDiffuseGreen*lightDiffuseGreen*diffuse;
        diffuseBlue += materialDiffuseBlue*lightDiffuseBlue*diffuse;
        float shininess = materialShininessRed;
        if (shininess < 0.001) shininess = 0.001;
        specularRed += materialSpecularRed*lightSpecularRed*pow(specular,shininess);
        specularGreen += materialSpecularGreen*lightSpecularGreen*pow(specular,shininess);
        specularBlue += materialSpecularBlue*lightSpecularBlue*pow(specular,shininess);
      }
    }
    v->colorRed = materialEmissionRed + ambientRed + diffuseRed + specularRed;
    v->colorGreen = materialEmissionGreen + ambientGreen + diffuseGreen + specularGreen;
    v->colorBlue = materialEmissionBlue + ambientBlue + diffuseBlue + specularBlue;
  }
}

__inline int glClipVertex(_GLContext *context, glVertex *v) {
  int clipFlags = (v->sx < context->viewportX0) ? 1 : 0;
  clipFlags |= (v->sy < context->viewportY0) ? 2 : 0;
  clipFlags |= (v->sx > context->viewportX1) ? 4 : 0;
  clipFlags |= (v->sy > context->viewportY1) ? 8 : 0;
  return clipFlags;
}

void interpolateVertex(glVertex *dest, glVertex *v0, glVertex *v1, float f) {
  dest->colorRed = v0->colorRed + f * (v1->colorRed-v0->colorRed);
  dest->colorGreen = v0->colorGreen + f * (v1->colorGreen-v0->colorGreen);
  dest->colorBlue = v0->colorBlue + f * (v1->colorBlue-v0->colorBlue);
  dest->colorAlpha = v0->colorAlpha + f * (v1->colorAlpha-v0->colorAlpha);

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
  float *matrix = context->matrix;
  glVertex poly[8];
  int count = 0;
  int clipFlags = 1|2|4|8;
  for(int i = 0; i < vertexCount; i++) {
    glVertex *v0 = vertices[i];
    glVertex *v1 = vertices[(i+1) % vertexCount];
    float z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
    float w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
    float z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
    float w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
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

  float *matrix = context->matrix;
  glVertex *v0 = &vertices[0];
  glVertex *v1 = &vertices[1];
  float z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
  float w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
  float z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
  float w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
  bool currentInside = z0 >= -w0;
  bool nextInside = z1 >= -w1;
  if ((!currentInside)&&(!nextInside))
    return;
  if ((!currentInside)||(!nextInside)) {
    float f = (w0+z0) / ((z0-z1)+(w0-w1));
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
  float dx = vertices[1].sx-vertices[0].sx;
  float dy = vertices[1].sy-vertices[0].sy;
  float l = sqrt(dx*dx+dy*dy);
  if (l>0.f) {dx/=l;dy/=l;};
  float nx = dy;
  float ny = -dx;
  float d = context->lineWidth*0.5;
  vertices[0].sx += nx*d;
  vertices[0].sy += ny*d;
  vertices[1].sx += nx*d;
  vertices[1].sy += ny*d;
  vertices[2].sx -= nx*d;
  vertices[2].sy -= ny*d;
  vertices[3].sx -= nx*d;
  vertices[3].sy -= ny*d;
  int clipFlags = glClipVertex(&glContext,&vertices[0]);
  clipFlags &= glClipVertex(&glContext,&vertices[1]);
  if (clipFlags == 0)
    glDrawQuad(context,&vertices[0],&vertices[1],&vertices[2],&vertices[3]);
}

void glEmitVertex() {
  glVertex *v = &glVertices[glCurrentVertexElement];
  _GLContext *w = &glContext;
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
            float dx = glVertices[1].sx-glVertices[0].sx;
            float dy = glVertices[1].sy-glVertices[0].sy;
            float l = sqrt(dx*dx+dy*dy);
            if (l>0.f) {dx/=l;dy/=l;};
            float nx = dy;
            float ny = -dx;
            float d = glContext.lineWidth*0.5;
            glVertices[0].sx += nx*d;
            glVertices[0].sy += ny*d;
            glVertices[1].sx += nx*d;
            glVertices[1].sy += ny*d;
            glVertices[2].sx -= nx*d;
            glVertices[2].sy -= ny*d;
            glVertices[3].sx -= nx*d;
            glVertices[3].sy -= ny*d;
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
          }
        } else {
          if (a != 3) {
            drawClippedLine(&glContext,&glVertices[0],&glVertices[1]);
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
            float dx = glVertices[1].sx-glVertices[0].sx;
            float dy = glVertices[1].sy-glVertices[0].sy;
            float l = sqrt(dx*dx+dy*dy);
            if (l>0.f) {dx/=l;dy/=l;};
            float nx = dy;
            float ny = -dx;
            float d = glContext.lineWidth*0.5;
            glVertices[0].sx += nx*d;
            glVertices[0].sy += ny*d;
            glVertices[1].sx += nx*d;
            glVertices[1].sy += ny*d;
            glVertices[2].sx -= nx*d;
            glVertices[2].sy -= ny*d;
            glVertices[3].sx -= nx*d;
            glVertices[3].sy -= ny*d;
            glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
          }
        } else {
          if (a != 3) {
            drawClippedLine(&glContext,&glVertices[0],&glVertices[1]);
          }
        }
        glVertices[0]=glVertices[1];
        glCurrentVertexElement = 1;
      }
    } break;
    case GL_TRIANGLES: {
      if (glCurrentVertexElement==3) {
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
    int r = (int)floor(glContext.clearRed*255.f);
    int g = (int)floor(glContext.clearGreen*255.f);
    int b = (int)floor(glContext.clearBlue*255.f);
    int a = (int)floor(glContext.clearAlpha*255.f);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    unsigned int rgba = r|(g<<8)|(b<<16)|(a<<24);
    for (int y = minY; y < maxY; y++)
      for (int x = minX; x < maxX; x++)
        glFrameBuffer[x+y*glFrameBufferWidth] = rgba;
  }
  if (mask & GL_DEPTH_BUFFER_BIT) {
    for (int y = minY; y < maxY; y++)
      for (int x = minX; x < maxX; x++)
        glDepthBuffer[x+y*glFrameBufferWidth] = glContext.clearDepth; // clamping?
  }
  if (mask & GL_STENCIL_BUFFER_BIT) {
    for (int y = minY; y < maxY; y++)
      for (int x = minX; x < maxX; x++)
        glStencilBuffer[x+y*glFrameBufferWidth] = glContext.clearStencil;
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

void glClearStencil(GLint s) {
  glContext.clearStencil = s;
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

void glColorMaterial(GLenum face, GLenum pname) {
  glContext.colorMaterial = pname;
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
  glContext.enableClientState(array, false);
}

void glEnable(GLenum cap) {
  glContext.enable(cap, true);
}

void glEnableClientState(GLenum array) {
  glContext.enableClientState(array, true);
}

void glEnd() {
  glContext.beginMode = 0;
}

void glFinish() {
}

void glFlush() {
}

void glGenTextures (GLsizei n, GLuint *textures) {
  for(int i = 0; i < n; i++) {
    textures[i] = glNewTexture();
  }
}

void glGetBooleanv(GLenum pname, GLboolean *params) {
  *params = glContext.getBoolean(pname);
}

void glGetFloatv (GLenum pname, GLfloat *params) {
  int i;
  switch(pname) {
    case GL_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][i];} break;
    case GL_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
    case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
    case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i<16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  }
}

void glGetIntegerv (GLenum pname, GLint *params) {
  int i;
  switch(pname) {
  case GL_VIEWPORT: {params[0]=glContext.viewportX0;params[1]=glContext.viewportY0;params[2]=glContext.viewportX1-glContext.viewportX0;params[3]=glContext.viewportY1-glContext.viewportY0;} break;
  }
}

GLenum glGetError() {
  return glContext.error;
}

const GLubyte *glGetString(GLenum name) {
  return glContext.getString(name);
}

GLboolean glIsEnabled (GLenum cap) {
  return glContext.isEnabled(cap);
}  

void glLightfv (GLenum light, GLenum pname, const GLfloat *params) {
  glContext.lightRed[pname & 3][light - GL_LIGHT0] = params[0];
  glContext.lightGreen[pname & 3][light - GL_LIGHT0] = params[1];
  glContext.lightBlue[pname & 3][light - GL_LIGHT0] = params[2];
  glContext.lightAlpha[pname & 3][light - GL_LIGHT0] = params[3];
}

void glLineStipple(GLint factor, GLushort pattern) {
  glContext.lineStippleFactor = factor;
  glContext.lineStipplePattern = pattern;
}

void glLineWidth(GLfloat width) {
  glContext.lineWidth = width;
}

void glLoadIdentity() {
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], identityMatrix, 4*4*sizeof(GLfloat));
  glUpdateMatrix();
}

void glLoadMatrixf (const GLfloat *m) {
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], m, 4*4*sizeof(GLfloat));
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
  switch(pname) {
    case GL_AMBIENT: {
      glContext.materialRed[0] = params[0];
      glContext.materialGreen[0] = params[1];
      glContext.materialBlue[0] = params[2];
    } break;
    case GL_DIFFUSE: {
      glContext.materialRed[1] = params[0];
      glContext.materialGreen[1] = params[1];
      glContext.materialBlue[1] = params[2];
    } break;
    case GL_SPECULAR: {
      glContext.materialRed[2] = params[0];
      glContext.materialGreen[2] = params[1];
      glContext.materialBlue[2] = params[2];
    } break;
    case GL_EMISSION: {
      glContext.materialRed[3] = params[0];
      glContext.materialGreen[3] = params[1];
      glContext.materialBlue[3] = params[2];
    } break;
    case GL_SHININESS: {
      glContext.materialRed[4] = params[0];
      glContext.materialGreen[4] = params[1];
      glContext.materialBlue[4] = params[2];
    } break;
  }
}

void glMatrixMode (GLenum mode) {
  glContext.matrixModeNr = mode & 0x01;
}

void glMultMatrixf (const GLfloat *m) {
  glMatMul(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

void glPointSize (GLfloat size) {
  glContext.pointSize = size;
}

void glPolygonOffset (GLfloat factor, GLfloat units) {
  glContext.polygonOffsetFactor = factor;
  glContext.polygonOffsetUnits = units;
}

void glPopMatrix() {
  memcpy(glMatrixStack[glMatrixStackPos], glContext.matrixForMode[glContext.matrixModeNr], 4*4*sizeof(GLfloat));
  glMatrixStackPos++;
  glUpdateMatrix();
}

void glPushMatrix() {
  glMatrixStackPos--;
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], glMatrixStack[glMatrixStackPos], 4*4*sizeof(GLfloat));
}

void glhRotate2f(GLfloat *matrix, GLfloat angleInRadians, GLfloat x, GLfloat y, GLfloat z) {
  float m[16], rotate[16];
  float OneMinusCosAngle, CosAngle, SinAngle;
  float A_OneMinusCosAngle, C_OneMinusCosAngle;
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
  float *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glhRotate2f(matrix,angle*2.f*3.1415927f/360.f,x,y,z);
  glUpdateMatrix();
}

void glScalef(GLfloat x, GLfloat y, GLfloat z) {
  float *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  matrix[0] *= x;
  matrix[4] *= y;
  matrix[8] *= z;
  matrix[1] *= x;
  matrix[5] *= y;
  matrix[9] *= z;
  matrix[2] *= x;
  matrix[6] *= y;
  matrix[10] *= z;
  matrix[3] *= x;
  matrix[7] *= y;
  matrix[11] *= z;
  glUpdateMatrix();
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
  switch(format) {
  case GL_RED:{rIn=0;} break;
  case GL_RG:{rIn=0;gIn=1;} break;
  case GL_RGB:{rIn=0;gIn=1;bIn=2;} break;
  case GL_BGR:{rIn=2;gIn=1;bIn=0;} break;
  case GL_RGBA:{rIn=0;gIn=1;bIn=2;aIn=3;} break;
  case GL_BGRA:{rIn=2;gIn=1;bIn=0;aIn=3;} break;
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
  for (int i = 0; i < width*height; i++) {
    unsigned int inputrgba = 0xffffffff;
    if (pixels != NULL) inputrgba = ((unsigned int*)pixels)[i];
    input[0] = inputrgba & 255;
    input[1] = (inputrgba>>8) & 255;
    input[2] = (inputrgba>>16) & 255;
    input[3] = (inputrgba>>24) & 255;
    unsigned int rgba = 0x00000000;
    if (rIn != -1) rgba |= input[rIn];
    if (gIn != -1) rgba |= input[gIn]<<8;
    if (bIn != -1) rgba |= input[bIn]<<16;
    if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
    t->data[i] = rgba;
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
  switch(format) {
  case GL_RED:{rIn=0;} break;
  case GL_RG:{rIn=0;gIn=1;} break;
  case GL_RGB:{rIn=0;gIn=1;bIn=2;} break;
  case GL_BGR:{rIn=2;gIn=1;bIn=0;} break;
  case GL_RGBA:{rIn=0;gIn=1;bIn=2;aIn=3;} break;
  case GL_BGRA:{rIn=2;gIn=1;bIn=0;aIn=3;} break;
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
  for (int i = 0; i < width*height; i++) {
    unsigned int inputrgba = 0xffffffff;
    if (pixels != NULL) inputrgba = ((unsigned int*)pixels)[i];
    input[0] = inputrgba & 255;
    input[1] = (inputrgba>>8) & 255;
    input[2] = (inputrgba>>16) & 255;
    input[3] = (inputrgba>>24) & 255;
    unsigned int rgba = 0x00000000;
    if (rIn != -1) rgba |= input[rIn];
    if (gIn != -1) rgba |= input[gIn]<<8;
    if (bIn != -1) rgba |= input[bIn]<<16;
    if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
    int x = i % width;
    int y = i / width;
    x += xoffset;
    y += yoffset;
    if (x >= 0 && y >= 0 && x < t->width && y < t->height)
      t->data[x+y*t->width] = rgba;
  }
}


void glhTranslatef2(GLfloat *matrix, GLfloat x, GLfloat y, GLfloat z) {
  matrix[12]=matrix[0]*x+matrix[4]*y+matrix[8]*z+matrix[12];
  matrix[13]=matrix[1]*x+matrix[5]*y+matrix[9]*z+matrix[13];
  matrix[14]=matrix[2]*x+matrix[6]*y+matrix[10]*z+matrix[14];
  matrix[15]=matrix[3]*x+matrix[7]*y+matrix[11]*z+matrix[15];
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  float *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glhTranslatef2(matrix,x,y,z);
  glUpdateMatrix();
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

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  glContext.viewportX0 = x;
  glContext.viewportY0 = glFrameBufferHeight-y-height;
  glContext.viewportX1 = x+width;
  glContext.viewportY1 = glFrameBufferHeight-y;
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
  glVertex3f(x,y,z);
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
  glTexCoord1f(x);
}

void glTexCoord2d(GLdouble x, GLdouble y) {
  glTexCoord2f(x,y);
}

void glTexCoord3d(GLdouble x, GLdouble y, GLdouble z) {
  glTexCoord3f(x,y,z);
}

void glTexCoord4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glTexCoord4f(x,y,z,w);
}

void glPushAttrib(GLbitfield mask) {
}

void glPopAttrib() {
}

void glFogf(GLenum pname, GLfloat param) {
}

void glFogfv(GLenum pname, GLfloat *params) {
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

GLAPI void APIENTRY glBlendCOlor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
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
void NormalizeVector(float *v) {
  float l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if (l>0) {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

void ComputeNormalOfPlane(float *normal, float *pvector1, float *pvector2) {
  normal[0]=(pvector1[1]*pvector2[2])-(pvector1[2]*pvector2[1]);
  normal[1]=(pvector1[2]*pvector2[0])-(pvector1[0]*pvector2[2]);
  normal[2]=(pvector1[0]*pvector2[1])-(pvector1[1]*pvector2[0]);
}

void glhFrustumf2(float *matrix, float left, float right, float bottom, float top, float znear, float zfar) {
  float temp, temp2, temp3, temp4;
  temp = 2.0 * znear;
  temp2 = right - left;
  temp3 = top - bottom;
  temp4 = zfar - znear;
  matrix[0] = temp / temp2;
  matrix[1] = 0;
  matrix[2] = 0;
  matrix[3] = 0;
  matrix[4] = 0;
  matrix[5] = temp/temp3;
  matrix[6] = 0;
  matrix[7] = 0;
  matrix[8] = (right + left)/temp2;
  matrix[9] = (top + bottom)/temp3;
  matrix[10] = (-zfar-znear)/temp4;
  matrix[11] = -1;
  matrix[12] = 0;
  matrix[13] = 0;
  matrix[14] = (-temp*zfar)/temp4;
  matrix[15] = 0.0;
}

void glhPerspectivef2(float *matrix, float fovyInDegrees, float aspectRatio, float znear, float zfar) {
  float ymax, xmax;
  float temp, temp2, temp3, temp4;
  ymax = znear * tan(fovyInDegrees*3.1415927f/360.f);
  xmax = ymax * aspectRatio;
  glhFrustumf2(matrix,-xmax,xmax,-ymax,ymax,znear,zfar);
}

void glhLookAtf2(float *matrix, float *eyePosition3D, float *center3D, float *upVector3D) {
  float forward[3], side[3], up[3];
  float matrix2[16], resultMatrix[16];
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
  glhTranslatef2(resultMatrix,-eyePosition3D[0],-eyePosition3D[1],-eyePosition3D[2]);
  memcpy(matrix,resultMatrix,16*sizeof(float));
}


int glhInvertMatrix(float *m, float *dest) {
  float inv[16], det;    
  int i;    

  inv[0] = m[5]  * m[10] * m[15] -
  m[5]  * m[11] * m[14] -
  m[9]  * m[6]  * m[15] +              
  m[9]  * m[7]  * m[14] +             
  m[13] * m[6]  * m[11] -             
  m[13] * m[7]  * m[10];    

  inv[4] = -m[4]  * m[10] * m[15] +               
  m[4]  * m[11] * m[14] +               
  m[8]  * m[6]  * m[15] -               
  m[8]  * m[7]  * m[14] -               
  m[12] * m[6]  * m[11] +               
  m[12] * m[7]  * m[10];    
  
  inv[8] = m[4]  * m[9] * m[15] -              
  m[4]  * m[11] * m[13] -              
  m[8]  * m[5] * m[15] +              
  m[8]  * m[7] * m[13] +              
  m[12] * m[5] * m[11] -              
  m[12] * m[7] * m[9];    
  
  inv[12] = -m[4]  * m[9] * m[14] +                
  m[4]  * m[10] * m[13] +               
  m[8]  * m[5] * m[14] -                
  m[8]  * m[6] * m[13] -                
  m[12] * m[5] * m[10] +                
  m[12] * m[6] * m[9];    
  
  inv[1] = -m[1]  * m[10] * m[15] +              
  m[1]  * m[11] * m[14] +               
  m[9]  * m[2] * m[15] -               
  m[9]  * m[3] * m[14] -               
  m[13] * m[2] * m[11] +               
  m[13] * m[3] * m[10];    
  
  inv[5] = m[0]  * m[10] * m[15] -              
  m[0]  * m[11] * m[14] -              
  m[8]  * m[2] * m[15] +              
  m[8]  * m[3] * m[14] +              
  m[12] * m[2] * m[11] -              
  m[12] * m[3] * m[10];    
  
  inv[9] = -m[0]  * m[9] * m[15] +               
  m[0]  * m[11] * m[13] +               
  m[8]  * m[1] * m[15] -                 
  m[8]  * m[3] * m[13] -               
  m[12] * m[1] * m[11] +               
  m[12] * m[3] * m[9];    
  
  inv[13] = m[0]  * m[9] * m[14] -               
  m[0]  * m[10] * m[13] -               
  m[8]  * m[1] * m[14] +               
  m[8]  * m[2] * m[13] +               
  m[12] * m[1] * m[10] -               
  m[12] * m[2] * m[9];    
  
  inv[2] = m[1]  * m[6] * m[15] -              
  m[1]  * m[7] * m[14] -              
  m[5]  * m[2] * m[15] +              
  m[5]  * m[3] * m[14] +              
  m[13] * m[2] * m[7] -              
  m[13] * m[3] * m[6];    
  
  inv[6] = -m[0]  * m[6] * m[15] +               
  m[0]  * m[7] * m[14] +               
  m[4]  * m[2] * m[15] -               
  m[4]  * m[3] * m[14] -               
  m[12] * m[2] * m[7] +               
  m[12] * m[3] * m[6];    
  
  inv[10] = m[0]  * m[5] * m[15] -               
  m[0]  * m[7] * m[13] -               
  m[4]  * m[1] * m[15] +               
  m[4]  * m[3] * m[13] +               
  m[12] * m[1] * m[7] -               
  m[12] * m[3] * m[5];    
  
  inv[14] = -m[0]  * m[5] * m[14] +                
  m[0]  * m[6] * m[13] +                
  m[4]  * m[1] * m[14] -                
  m[4]  * m[2] * m[13] -                
  m[12] * m[1] * m[6] +                
  m[12] * m[2] * m[5];    
  
  inv[3] = -m[1] * m[6] * m[11] +               
  m[1] * m[7] * m[10] +               
  m[5] * m[2] * m[11] -               
  m[5] * m[3] * m[10] -               
  m[9] * m[2] * m[7] +               
  m[9] * m[3] * m[6];    
  
  inv[7] = m[0] * m[6] * m[11] -              
  m[0] * m[7] * m[10] -              
  m[4] * m[2] * m[11] +              
  m[4] * m[3] * m[10] +              
  m[8] * m[2] * m[7] -              
  m[8] * m[3] * m[6];    
  
  inv[11] = -m[0] * m[5] * m[11] +                
  m[0] * m[7] * m[9] +                
  m[4] * m[1] * m[11] -                
  m[4] * m[3] * m[9] -                
  m[8] * m[1] * m[7] +                
  m[8] * m[3] * m[5];    
  
  inv[15] = m[0] * m[5] * m[10] -               
  m[0] * m[6] * m[9] -               
  m[4] * m[1] * m[10] +               
  m[4] * m[2] * m[9] +               
  m[8] * m[1] * m[6] -               
  m[8] * m[2] * m[5];    

  det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];    
  if (det == 0)        
    return 0;    
  
  det = 1.f / det;    
  for (i = 0; i < 16; i++)        
    dest[i] = inv[i] * det;
  
  return 1;
}

void MultiplyMatrixByVector4By4OpenGL_FLOAT(float *dest, float *m, float *v) {
  float x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  float y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  float z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  float w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

void MultiplyMatrices4by4OpenGL_FLOAT(float *dest, float *m1, float *m2) {
  glMatMul(m1,m2,dest);
}

int glhProjectf(float objx, float objy, float objz, float *modelView, float *projection, int *viewport, float *windowCoordinate) {
  float fTempo[8];
  fTempo[0] = modelView[0]*objx+modelView[4]*objy+modelView[8]*objz+modelView[12];
  fTempo[1] = modelView[1]*objx+modelView[5]*objy+modelView[9]*objz+modelView[13];
  fTempo[2] = modelView[2]*objx+modelView[6]*objy+modelView[10]*objz+modelView[14];
  fTempo[3] = modelView[3]*objx+modelView[7]*objy+modelView[11]*objz+modelView[15];
  fTempo[4] = projection[0]*fTempo[0]+projection[4]*fTempo[1]+projection[8]*fTempo[2]+projection[12]*fTempo[3];
  fTempo[5] = projection[1]*fTempo[0]+projection[5]*fTempo[1]+projection[9]*fTempo[2]+projection[13]*fTempo[3];
  fTempo[6] = projection[2]*fTempo[0]+projection[6]*fTempo[1]+projection[10]*fTempo[2]+projection[14]*fTempo[3];
  fTempo[7] = -fTempo[2];
  if (fTempo[7]==0.0)
    return 0;
  fTempo[7]=1.0/fTempo[7];
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  windowCoordinate[0]=(fTempo[4]*0.5+0.5)*viewport[2]+viewport[0];
  windowCoordinate[1]=(fTempo[5]*0.5+0.5)*viewport[3]+viewport[1];
  windowCoordinate[2]=(1.0+fTempo[6])*0.5;
  return 1;
}

int glhUnProjectf(float winx, float winy, float winz, float *modelView, float *projection, int *viewport, float *objectCoordinate) {
  float m[16],A[16];
  float in[4],out[4];
  MultiplyMatrices4by4OpenGL_FLOAT(A,projection,modelView);
  if (glhInvertMatrix(A,m)==0)
    return 0;
  in[0] = (winx-(float)viewport[0])/(float)viewport[2]*2.0-1.0;
  in[1] = (winy-(float)viewport[1])/(float)viewport[3]*2.0-1.0;
  in[2] = 2.0*winz-1.0;
  in[3] = 1.0;
  MultiplyMatrixByVector4By4OpenGL_FLOAT(out,m,in);
  if (out[3]==0.0)
    return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

void glOrtho(float left, float right, float bottom, float top, float znear, float zfar) {
  GLfloat x,y,z;
  GLfloat tx,ty,tz;
  GLfloat m[16];
  x = 2.0 / (right-left);
  y = 2.0 / (top-bottom);
  z = -2.0 / (zfar - znear);
  tx = -(right+left)/(right-left);
  ty = -(top+bottom)/(top-bottom);
  tz = -(zfar+znear)/(zfar-znear);
#define M(row,col) m[col*4+row]  
  M(0,0)=x; M(0,1)=0; M(0,2) = 0.f; M(0,3) = tx;
  M(1,0)=0; M(1,1)=y; M(1,2) = 0.f; M(1,3) = ty;
  M(2,0)=0; M(2,1)=0; M(2,2) = z; M(2,3) = tz;
  M(3,0)=0; M(3,1)=0; M(3,2) = 0; M(3,3) = 1;
  glMultMatrixf(m);
}

void glFrustum(float left, float right, float bottom, float top, float znear, float zfar) {
  glhFrustumf2(glContext.matrixForMode[glContext.matrixModeNr],left,right,bottom,top,znear,zfar);
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
  glhPerspectivef2(glContext.matrixForMode[glContext.matrixModeNr],fov,aspect,nearPlane,farPlane);
  glUpdateMatrix();
}

void gluLookAt(GLfloat cx,GLfloat cy,GLfloat cz,GLfloat ox,GLfloat oy,GLfloat oz,GLfloat ux,GLfloat uy,GLfloat uz) {
  float eye[3];
  float center[3];
  float up[3];
  eye[0] = cx;
  eye[1] = cy;
  eye[2] = cz;
  center[0] = ox;
  center[1] = oy;
  center[2] = oz;
  up[0] = ux;
  up[1] = uy;
  up[2] = uz;
  glhLookAtf2(glContext.matrixForMode[glContext.matrixModeNr],eye,center,up);
  glUpdateMatrix();
}

GLint gluProject(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  float d[3];
  const GLint r = glhProjectf(objX, objY, objZ, model, projection, view, d);
  *winX = d[0];
  *winY = d[1];
  *winZ = d[2];
  return r;
}

GLint gluUnProject(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *objX, GLfloat *objY, GLfloat *objZ) {
  float d[3];
  const GLint r = glhUnProjectf(winX, winY, winZ, model, projection, view, d);
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
#include <stdlib.h>
float glSign(float v) {
  return v < 0 ? -1 : (v > 0 ? 1 : 0);
}
double getBary(double x, double y, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  double nx = v2->sy - v1->sy;
  double ny = -(v2->sx - v1->sx);
  double l = (v0->sx-v1->sx)*nx + (v0->sy-v1->sy)*ny;
  double p = (x-v1->sx)*nx + (y-v1->sy)*ny;
  return (p/l); // actually it must have been g/l and nx and ny been normalized
}

#define __POLYMINMAX__\
  int pminx,pminy,pmaxx,pmaxy;\
  pminx=(int)floor(v0->sx);\
  pminy=(int)floor(v0->sy);\
  pmaxx=(int)floor(v0->sx);\
  pmaxy=(int)floor(v0->sy);\
  if ((int)floor(v1->sx)<pminx) pminx=(int)floor(v1->sx);\
  if ((int)floor(v1->sy)<pminy) pminy=(int)floor(v1->sy);\
  if ((int)floor(v1->sx)>pmaxx) pmaxx=(int)floor(v1->sx);\
  if ((int)floor(v1->sy)>pmaxy) pmaxy=(int)floor(v1->sy);\
  if ((int)floor(v2->sx)<pminx) pminx=(int)floor(v2->sx);\
  if ((int)floor(v2->sy)<pminy) pminy=(int)floor(v2->sy);\
  if ((int)floor(v2->sx)>pmaxx) pmaxx=(int)floor(v2->sx);\
  if ((int)floor(v2->sy)>pmaxy) pmaxy=(int)floor(v2->sy);\
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
  
#define __PAINTPOLYQUAD_BEGINX__\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth];\
  int dminx = pminx;\
  int dmaxx = pmaxx;\
  for (x = dminx;x<dmaxx;x++) {


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
  int t = (int)floor(tx);
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

void glDrawTriangle(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2) {
  if (context->isEnabled(GL_CULL_FACE)) {
    bool backFacing = glCCW(v0,v1,v2);
    if (context->cullFaceMode == GL_FRONT && backFacing) return;
    if (context->cullFaceMode == GL_BACK && (!backFacing)) return;
    if (context->cullFaceMode == GL_FRONT_AND_BACK) return;
  }   
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

  bool stencil = context->isEnabled(GL_STENCIL_TEST);
  bool blending = context->isEnabled(GL_BLEND);

  double v0w = 1.0/(v0->sw);
  double v1w = 1.0/(v1->sw);
  double v2w = 1.0/(v2->sw);
  double v0z = 1.0/(v0->sz);
  double v1z = 1.0/(v1->sz);
  double v2z = 1.0/(v2->sz);

  glTexture *t = &glTextures[context->boundTextures[context->activeTexture]];
  bool textured = true && context->isEnabled(GL_TEXTURE_2D) && t->data != NULL && t->name != 0;
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

    tx0 = v0->textureX*v0w*twidth0;
    ty0 = v0->textureY*v0w*theight0;
  
    tx1 = v1->textureX*v1w*twidth0;
    ty1 = v1->textureY*v1w*theight0;
  
    tx2 = v2->textureX*v2w*twidth0;
    ty2 = v2->textureY*v2w*theight0;

    borderColor = ((int)floor(glClampf(t->borderColorRed*255.f,0.f,255.f)));
    borderColor |= ((int)floor(glClampf(t->borderColorGreen*255.f,0.f,255.f)))<<8;
    borderColor |= ((int)floor(glClampf(t->borderColorBlue*255.f,0.f,255.f)))<<16;
    borderColor |= ((int)floor(glClampf(t->borderColorAlpha*255.f,0.f,255.f)))<<24;
  }
  int blendFuncS = context->blendFuncSFactor;
  int blendFuncD = context->blendFuncDFactor;
  int blendEquation = context->blendEquation;
  unsigned int constantColor = ((int)floor(glClampf(context->blendColorRed*255.f,0.f,255.f)));
  constantColor |= ((int)floor(glClampf(context->blendColorGreen*255.f,0.f,255.f)))<<8;
  constantColor |= ((int)floor(glClampf(context->blendColorBlue*255.f,0.f,255.f)))<<16;
  constantColor |= ((int)floor(glClampf(context->blendColorAlpha*255.f,0.f,255.f)))<<24;
  bool writeDepth = context->depthMask;  
  int depthFunction = context->depthFunc;
  bool depthTest = context->isEnabled(GL_DEPTH_TEST);

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
   
  r0 = r0*255.0*v0w;
  g0 = g0*255.0*v0w;
  b0 = b0*255.0*v0w;
  a0 = a0*255.0*v0w;

  r1 = r1*255.0*v1w;
  g1 = g1*255.0*v1w;
  b1 = b1*255.0*v1w;
  a1 = a1*255.0*v1w;

  r2 = r2*255.0*v2w;
  g2 = g2*255.0*v2w;
  b2 = b2*255.0*v2w;
  a2 = a2*255.0*v2w;

  __PAINTPOLYQUAD_BEGINY__
  __PAINTPOLYQUAD_INITBARYFORX__
  __PAINTPOLYQUAD_BEGINX__
      if (bary0>=0&&bary1>=0&&bary2>=0) {
        double iz = 1.0/(__BARY0__(v0z)+__BARY1__(v1z)+__BARY2__(v2z));
        if ((!depthTest)||glCheckDepthFunction(*zDest,iz,depthFunction)) {
          double iw = 1.0/(__BARY0__(v0w)+__BARY1__(v1w)+__BARY2__(v2w));
          int r = (__BARY0__(r0)+__BARY1__(r1)+__BARY2__(r2))*iw;
          int g = (__BARY0__(g0)+__BARY1__(g1)+__BARY2__(g2))*iw;
          int b = (__BARY0__(b0)+__BARY1__(b1)+__BARY2__(b2))*iw;
          int a = (__BARY0__(a0)+__BARY1__(a1)+__BARY2__(a2))*iw;
          if (textured) {
            float tx = (__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw;
            float ty = (__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw;
            int tix0 = textureWrap(tx, twidth0, t->wrapS);
            int tiy0 = textureWrap(ty, theight0, t->wrapT);
            unsigned int rgba = 0xffffffff;
            if (filtering) {
              int tix1 = textureWrap(tx+1, twidth0, t->wrapS);
              int tiy1 = textureWrap(ty+1, theight0, t->wrapT);
              int txf = (int)floor(fmod(tx*256.f,256.f));
              int tyf = (int)floor(fmod(ty*256.f,256.f));
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
                tiy0 *= twidth0;
                rgba  = tdata0[tix0+tiy0];
              } else {
                rgba = borderColor;
              }
            }
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
          if (writeDepth) 
            *zDest=iz;
          if (!blending)
            *pDest=r|(g<<8)|(b<<16)|(a<<24);
          else {
            *pDest=doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
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
#include <stdlib.h>
#include <stdio.h>
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
GLboolean glVesa(int xRes,int yRes, int bPP) {
  glGraphicsModeToRestore = getBiosGraphicsMode();
  glVbeInfoBlock vbeInfo;
  glVbeModeInfo modeInfo;
  RMREGS regs;
  regs.ax = 0x4f00;
  VBE_callESDI(&regs,&vbeInfo,sizeof(glVbeInfoBlock));
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
      glStencilBuffer = new int[glFrameBufferWidth*glFrameBufferHeight];
      glDepthBuffer = new float[glFrameBufferWidth*glFrameBufferHeight];
      memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glStencilBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      memset(glDepthBuffer,0,glFrameBufferWidth*glFrameBufferHeight*4);
      return true;
    }
    v++;
  }
  delete[] w;
  return false;
}
void glRefresh() {
  unsigned int rgba;
  for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) {
    rgba = glFrameBuffer[i];
    glFrameBufferDedicated[i] = (rgba & 0xff00ff00)|((rgba>>16)&0xff)|((rgba<<16)&0x00ff0000);
  }
}
void glDone() {
  delete[] glFrameBuffer;
  delete[] glDepthBuffer;
  delete[] glStencilBuffer;
  unmapPhysical((unsigned int)glFrameBufferDedicated);
  setBiosGraphicsMode(glGraphicsModeToRestore);
}
// --------------------------------------
// --------------------------------------
// --------------------------------------

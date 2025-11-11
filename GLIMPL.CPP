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
// - On problems with Vesa on DOS, maybe UniVBE (or similar) may help. Not tested that, though.
// --
#define WATCOMGLAPI
#include "GL.H"
#include <math.h>
#include <string.h>
#include <stdlib.h>

//#define __GLDISABLEDOSFUNCTIONS__ 1
#if (!defined(__WATCOMC__))&&(!defined(__DJGPP__))
#define __GLDISABLEDOSFUNCTIONS__ 1 // use glDirect()
#endif

#ifndef __GLDISABLEDOSFUNCTIONS__ 
#include <time.h>
#ifdef __WATCOMC__
#include <i86.h>
#include <conio.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#endif // __DJGPP__
#endif // __GLDISABLEDOSFUNCTIONS__ 


#ifdef __cplusplus
  extern "C" {GLvoid _GLContext_init(); GLvoid constructGL() {}}
  class GLConstructor {public: GLConstructor() {_GLContext_init();}};
  GLConstructor glConstruct;
#else // __cplusplus
  GLvoid _GLContext_init();
  GLvoid constructGL() {_GLContext_init();}
#endif // __cplusplus

#ifndef PI
#define PI 3.14159265358979323846
#endif // PI

#ifdef __WATCOMC__
#define INLINE static __inline
#define __FASTTEXTURING__ 1
#endif // __WATCOMC__

#ifdef __DJGPP__
#define INLINE static  inline
#define __FASTTEXTURING__ 1
#endif // __DJGPP__

#ifndef INLINE
#define INLINE static inline
#define __FASTTEXTURING__ 1
#endif // INLINE

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define __MALLOC malloc
#define __FREE free
#define __UNUSED(__x__) (void)(__x__)

#define __MATRIX_STACK_SIZE__ 16
#define __ATTRIB_STACK_SIZE__ 4
#define SWAP_ROWS_DOUBLE(a, b) { GLdouble *_tmp = a; (a)=(b); (b)=_tmp; }
#define SWAP_ROWS_FLOAT(a, b) { GLfloat *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]
#define FLIPRB(__v__) (((__v__)&0xff00ff00)|(((__v__)>>16) & 0x000000ff)|(((__v__)<<16) & 0x00ff0000))
#define SEAMPREVENT -0.0000001 // landing directly on 0.5 screen (real screen) pixels caused seams on DJGPP
#define FLOOR(__v__) (__v__) // not needed since conversion to int truncates (floor() is extremely slow)
#define DIVE255 /= 255 // >>= 8
#define DIVQ255 / 255 // >> 8

glTexture glTextures[GLMAXTEXTURES];

GLdouble glPixelCenterX = -0.5+SEAMPREVENT;
GLdouble glPixelCenterY = -0.5+SEAMPREVENT;
GLdouble glTexelCenterX = -0.5;
GLdouble glTexelCenterY = -0.5;
GLuint *glFrameBufferDedicated = NULL; // linear framebuffer
GLint glFrameBufferWidth = 0;
GLint glFrameBufferHeight = 0;
GLint glFrameBufferMultiSample = 1;
GLint glFrameBufferBytesPerPixel = 4;
GLuint *glFrameBuffer = NULL;
GLfloat *glDepthBuffer = NULL;
GLboolean useNearPointers = GL_TRUE;
GLboolean glUseHalveVector = GL_FALSE; // OSMesa seems to use the halvevector, instead of the "real" phong described in the docs of OpenGL
GLboolean glFastTexturing = GL_FALSE; // only with #define __FASTTEXTURING__
GLboolean glVGACheckered = GL_FALSE;
GLboolean glWaitVSync = GL_FALSE; // Vesa function 0x4f07 and 0x4f0a are missing here, sorry. Use this with care, since this is a VGA function and not Vesa.
GLint glFastTextureSpanWidth = 16;
GLint glFrameBufferWidth0 = 0;
GLint glFrameBufferHeight0 = 0;
GLuint *glFrameBuffer0 = NULL;
GLfloat *glDepthBuffer0 = NULL;
GLdouble identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};
GLboolean currentBackFacing = GL_FALSE;
_GLContext glContext;
_GLContext glAttribStack[__ATTRIB_STACK_SIZE__];
GLint glAttribStackPos = 0;
GLdouble glMatrixStack[__MATRIX_STACK_SIZE__][4*4];
GLint glMatrixStackPos = 0;
glVertex glVertices[4];
GLint glCurrentVertexElement = 0;
GLdouble additionalPointSpriteXStretch = 1.0;
TriangleDrawer glDrawTriangle = glDrawTrianglePrecise;
GLenum glError = GL_NO_ERROR;

GLubyte *glStencilBuffer = NULL; // strange, by placing these pointers not above it gets a lot faster (maybe be pack alignment?)
GLubyte *glStencilBuffer0 = NULL;

INLINE GLfloat glClampf(GLfloat a, GLfloat n, GLfloat x) {
  return a < n ? n : (a > x ? x : a);
}

INLINE GLint glClampi(GLint a, GLint n, GLint x) {
  return a < n ? n : (a > x ? x : a);
}

INLINE GLint glMini(GLint a, GLint n) {
  return a < n ? a : n;
}

INLINE GLint glMaxi(GLint a, GLint x) {
  return a > x ? a : x;
}

GLvoid glSetError(GLenum error) {
  glError = error;
}

GLvoid initTexture(glTexture *t) {
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

#pragma pack(push)
#pragma pack(1)
typedef struct GLBuffer {
  GLuint name;
  GLvoid *data;
  GLsizei dataSize;
  GLuint *colorPointer;
  GLuint colorWidth;
  GLuint colorHeight;
  GLfloat *depthPointer;
  GLuint depthWidth;
  GLuint depthHeight;
  GLubyte *stencilPointer;
  GLuint stencilWidth;
  GLuint stencilHeight;
} GLBuffer;
#pragma pack(pop)

GLBuffer glBuffers[GLMAXBUFFERS];
GLBuffer *glBoundBuffer=NULL;
GLBuffer *glBoundElementBuffer=NULL;
GLBuffer *glBoundFrameBuffer=NULL;
GLBuffer glStandardBuffer;
GLBuffer glStandardElementBuffer;

GLBuffer *glGetCurrentBuffer() {
  if (glBoundBuffer==NULL) {
    return &glStandardBuffer;
  }
  return glBoundBuffer;
}

GLBuffer *glGetCurrentElementBuffer() {
  if (glBoundElementBuffer==NULL) {
    return &glStandardElementBuffer;
  }
  return glBoundElementBuffer;
}

GLuint glNewBuffer() {
  for (GLint i = 1; i < GLMAXBUFFERS; i++) {
    if (glBuffers[i].name == 0x00) {
      memset(&glBuffers[i],0,sizeof(GLBuffer));
      glBuffers[i].name = i;
      return i;
    }
  }
  glSetError(GL_OUT_OF_MEMORY);
  return 0;
}

GLvoid glDeleteBuffer(GLuint i) {
  if (i > 0 && glBuffers[i].name != 0) {
    glBuffers[i].name = 0;
  }
}

GLvoid _GLContext_init() {
  GLint i;
  memset(glContext.enabledCaps,0,sizeof(glContext.enabledCaps));
  memset(&glStandardBuffer,0,sizeof(glStandardBuffer));
  memset(&glStandardElementBuffer,0,sizeof(glStandardElementBuffer));
  memset(glBuffers,0,sizeof(glBuffers));
  glBoundBuffer=NULL;
  glBoundElementBuffer=NULL;
  glBoundFrameBuffer=NULL;
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
  glContext.depthMask = GL_TRUE;
  glContext.depthRangeZNear = 0;
  glContext.depthRangeZFar = 1;
  glContext.pointSize = 0;
  glContext.polygonOffsetFactor = 0;
  glContext.polygonOffsetUnits = 0;
  memcpy(glContext.modelViewMatrix,identityMatrix,4*4*sizeof(GLdouble));
  memcpy(glContext.projectionMatrix,identityMatrix,4*4*sizeof(GLdouble));
  for (i = 0; i < GLMAXTEXTUREUNITS; i++) memcpy(glContext.textureMatrix[i],identityMatrix,4*4*sizeof(GLdouble));
  glContext.matrixForMode[GL_MODELVIEW & 1] = glContext.modelViewMatrix;
  glContext.matrixForMode[GL_PROJECTION & 1] = glContext.projectionMatrix;
  glContext.matrixForMode[2] = glContext.textureMatrix[0];
  memcpy(glContext.inverseMatrixForMode[0],identityMatrix,4*4*sizeof(GLdouble));
  memcpy(glContext.inverseMatrixForMode[1],identityMatrix,4*4*sizeof(GLdouble));
  memcpy(glContext.inverseMatrixForMode[2],identityMatrix,4*4*sizeof(GLdouble)); // not used
  memcpy(glContext.matrix,identityMatrix,4*4*sizeof(GLdouble));
  glContext.matrixModeNr = 0;
  glContext.lineStippleFactor = 0;
  glContext.lineStipplePattern = 0;
  glContext.lineWidth = 1;
  glContext.scissorX0 = 0;
  glContext.scissorY0 = 0;
  glContext.scissorX1 = 0;
  glContext.scissorY1 = 0;
  glContext.shadeMode = 0;
  glContext.stencilFunc = GL_ALWAYS;
  glContext.stencilFuncRef = 0;
  glContext.stencilFuncMask = 0xff;
  glContext.stencilMask = 0xff;
  glContext.stencilOpFail = GL_KEEP;
  glContext.stencilOpZFail = GL_KEEP;
  glContext.stencilOpZPass = GL_KEEP;
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
  glContext.textureW = 1.0;
  glContext.beginMode = 0;

  glContext.colorMaterialFace = GL_FRONT;
  for (i = 0; i < 2; i++) {
    glContext.colorMaterial[i] = GL_AMBIENT_AND_DIFFUSE;
    // ambient
    glContext.materialRed[i][0] = 0.2;
    glContext.materialGreen[i][0] = 0.2;
    glContext.materialBlue[i][0] = 0.2;
    glContext.materialAlpha[i][0] = 1;
    // diffuse
    glContext.materialRed[i][1] = 0.8;
    glContext.materialGreen[i][1] = 0.8;
    glContext.materialBlue[i][1] = 0.8;
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
    glContext.wireframe[i] = GL_FALSE;
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
    glContext.spotCutOff[i] = (GLfloat)cos(180.0*PI/180.0);
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
  glContext.maskRed = GL_TRUE;
  glContext.maskGreen = GL_TRUE;
  glContext.maskBlue = GL_TRUE;
  glContext.maskAlpha = GL_TRUE;
  glContext.forceNoCull = 0;
  glContext.zoomX = 1.0;
  glContext.zoomY = 1.0;
  glContext.pushAttribBitsHere = 0;
  glContext.explicitAlpha = 0.f;
  glContext.useExplicitAlpha = GL_FALSE;
  glContext.separateSpecular = GL_FALSE;
  glContext.texGenS = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  glContext.texGenT = GL_SPHERE_MAP_ATAN2; // actually GL_EYE_LINEAR
  glContext.needNewInverseModelView = GL_FALSE;
  glContext.twoSidedLighting = GL_FALSE;

  glContext.fogStart = 0.0;
  glContext.fogEnd = 1.0;
  glContext.fogColor[0] = 0;
  glContext.fogColor[1] = 0;
  glContext.fogColor[2] = 0;
  glContext.fogColor[3] = 0;
  glContext.fogDensity = 1.0;
  glContext.fogMode = GL_EXP;

  glContext.vertexEnabledBuffer = GL_FALSE;
  glContext.vertexSizeBuffer = 0;
  glContext.vertexTypeBuffer = 0;
  glContext.vertexStrideBuffer = 0;
  glContext.vertexPointerBuffer = NULL;
  glContext.normalEnabledBuffer = GL_FALSE;
  glContext.normalSizeBuffer = 0;
  glContext.normalTypeBuffer = 0;
  glContext.normalStrideBuffer = 0;
  glContext.normalPointerBuffer = NULL;
  glContext.texEnabledBuffer = GL_FALSE;
  for (i = 0; i < GLMAXTEXTUREUNITS; i++) {
    glContext.texSizeBuffer[i] = 0;
    glContext.texTypeBuffer[i] = 0;
    glContext.texStrideBuffer[i] = 0;
    glContext.texPointerBuffer[i] = NULL;
  }
  glContext.colorEnabledBuffer = GL_FALSE;
  glContext.colorSizeBuffer = 0;
  glContext.colorTypeBuffer = 0;
  glContext.colorStrideBuffer = 0;
  glContext.colorPointerBuffer = NULL;
  glContext.indexEnabledBuffer = GL_TRUE; // seems to be default on?
  glContext.beginPrimitiveIndex = 0;
  glContext.clientActiveTexture = GL_TEXTURE0;

  glContext.ambientColorRed = 0.2f;
  glContext.ambientColorGreen = 0.2f;
  glContext.ambientColorBlue = 0.2f;
  glContext.ambientColorAlpha = 1.f;

  glContext.pointSizeMin = 0.f;
  glContext.pointSizeMax = 10000.f; // spec says 1.f is needed
  glContext.pointAttenuation[0] = 1.f;
  glContext.pointAttenuation[1] = 0.f;
  glContext.pointAttenuation[2] = 0.f;
  memset(glContext.clipPlanes,0,sizeof(glContext.clipPlanes));

  for (i = 0; i < GLMAXTEXTURES; i++) {
    glTexture *t=&glTextures[i];
    initTexture(t);
  }
}

GLvoid _GLContext_enable(GLenum prop, GLboolean enable) {
  glContext.enabledCaps[prop&255]=enable;
}

GLboolean _GLContext_isEnabled(GLenum prop) {
  return glContext.enabledCaps[prop&255];
}

INLINE GLboolean glIsEnabled2(_GLContext *context, GLenum prop) {
  return context->enabledCaps[prop&255];
}

GLvoid glSetTriangleDrawer(TriangleDrawer drawer) {
  glDrawTriangle = drawer;
}

GLuint glNewTexture() {
  for (GLint i = 1; i < GLMAXTEXTURES; i++) {
    if (glTextures[i].name == 0x00) {
      initTexture(&glTextures[i]);
      glTextures[i].name = i;
      glTextures[i].width = 0;
      glTextures[i].height = 0;
      glTextures[i].data = NULL;
      return i;
    }
  }
  glSetError(GL_OUT_OF_MEMORY);
  return 0;
}

GLvoid glDeleteTexture(GLuint i) {
  if (i > 0 && glTextures[i].name != 0) {
    glTextures[i].name = 0;
    if (glTextures[i].data != NULL) {
      __FREE(glTextures[i].data);
      glTextures[i].data = NULL;
    }
  }
}

GLdouble *glGetInverseModelView(_GLContext *context) {
  if (context->needNewInverseModelView) {
    context->needNewInverseModelView = GL_FALSE;
    memcpy(glContext.inverseMatrixForMode[GL_MODELVIEW & 1],glContext.matrixForMode[GL_MODELVIEW & 1],4*4*sizeof(GLdouble));
    gluInvertMatrix(glContext.inverseMatrixForMode[GL_MODELVIEW & 1],glContext.inverseMatrixForMode[GL_MODELVIEW & 1]);
  }
  return glContext.inverseMatrixForMode[GL_MODELVIEW & 1];
}

GLvoid combineIntoWindow(GLint *x0, GLint *y0, GLint *x1, GLint *y1,GLint nx0, GLint ny0, GLint nx1, GLint ny1) {
  *x0 = nx0 < *x0 ? *x0 : nx0;
  *y0 = ny0 < *y0 ? *y0 : ny0;
  *x1 = nx1 > *x1 ? *x1 : nx1;
  *y1 = ny1 > *y1 ? *y1 : ny1;
}

GLvoid glMatMulf(const GLfloat *ma, const GLfloat *mb, GLfloat *r) {
  GLfloat result[16];
  for (GLint i = 0; i < 4; i++) {
    for (GLint j = 0; j < 4; j++) {
      GLfloat a = 0.f;
      for (GLint k = 0; k < 4; k++) {
        a += ma[i+k*4] * mb[j*4+k];
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(GLfloat));
}

GLvoid glMatMulf2(const GLdouble *ma, const GLfloat *mb, GLdouble *r) { // double output
  GLdouble result[16];
  for (GLint i = 0; i < 4; i++) {
    for (GLint j = 0; j < 4; j++) {
      GLdouble a = 0.0;
      for (GLint k = 0; k < 4; k++) {
        a += ((GLdouble)ma[i+k*4] * mb[j*4+k]);
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(GLdouble));
}

GLvoid glMatMul(const GLdouble *ma, const GLdouble *mb, GLdouble *r) {
  GLdouble result[16];
  for (GLint i = 0; i < 4; i++) {
    for (GLint j = 0; j < 4; j++) {
      GLdouble a = 0.0;
      for (GLint k = 0; k < 4; k++) {
        a += ma[i+k*4] * mb[j*4+k];
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(GLdouble));
}

GLvoid glUpdateMatrix() {
  glMatMul(glContext.matrixForMode[GL_PROJECTION & 1],glContext.matrixForMode[GL_MODELVIEW & 1],glContext.matrix);
  glContext.needNewInverseModelView = GL_TRUE;
}

INLINE GLint glTransformVertex(_GLContext *context, glVertex *v, GLboolean clip) {
  const GLdouble *cm = context->matrix;
  const GLdouble vx = v->vertexX;
  const GLdouble vy = v->vertexY;
  const GLdouble vz = v->vertexZ;
  const GLdouble vw = v->vertexW;
  const GLdouble x = vx * cm[0*4+0] + vy * cm[1*4+0] + vz * cm[2*4+0] + vw * cm[3*4+0];
  const GLdouble y = vx * cm[0*4+1] + vy * cm[1*4+1] + vz * cm[2*4+1] + vw * cm[3*4+1];
  const GLdouble z = vx * cm[0*4+2] + vy * cm[1*4+2] + vz * cm[2*4+2] + vw * cm[3*4+2];
  const GLdouble w = vx * cm[0*4+3] + vy * cm[1*4+3] + vz * cm[2*4+3] + vw * cm[3*4+3];

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

GLvoid fixSphereMapUV(_GLContext *context, GLfloat *tx0, GLfloat *ty0, GLfloat *tx1, GLfloat *ty1) {
  if (glIsEnabled2(context,GL_TEXTURE_GEN_S) && (context->texGenS == GL_SPHERE_MAP_DUAL_PARABOLOID || context->texGenS == GL_SPHERE_MAP_ATAN2||context->texGenS == GL_SPHERE_MAP)) {
    if (*tx1-*tx0 > 0.5) *tx1-=1.0;
    if (*tx1-*tx0 < -0.5) *tx1+=1.0;
  }
  if (glIsEnabled2(context,GL_TEXTURE_GEN_T) && (context->texGenT == GL_SPHERE_MAP_DUAL_PARABOLOID || context->texGenT == GL_SPHERE_MAP_ATAN2||context->texGenT == GL_SPHERE_MAP)) {
    if (*ty1-*ty0 > 0.5) *ty1-=1.0;
    if (*ty1-*ty0 < -0.5) *ty1+=1.0;
  }
}

GLvoid glTexGen_(_GLContext *context, glVertex *v, GLdouble x, GLdouble y, GLdouble z, GLdouble normalX, GLdouble normalY, GLdouble normalZ) {
  // glTexGen is done per vertex what most likely is wrong
  GLdouble vx = x;
  GLdouble vy = y;
  GLdouble vz = z;
  const GLdouble l = sqrt(vx*vx+vy*vy+vz*vz);
  if (l > 0.0) {                               
    vx /= l;
    vy /= l;
    vz /= l;
  }
  const GLdouble l2 = sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
  if (l2 > 0.0) {
    normalX /= l2;
    normalY /= l2;
    normalZ /= l2;
  }

  const GLdouble dot2NI = 2.0*(vx*normalX+vy*normalY+vz*normalZ);
  GLdouble reflectionX = vx - dot2NI * normalX;
  GLdouble reflectionY = vy - dot2NI * normalY;
  GLdouble reflectionZ = vz - dot2NI * normalZ;

  switch(context->texGenS) {
  case GL_SPHERE_MAP: {v->textureX = reflectionX*0.5+0.5;} break;
  case GL_SPHERE_MAP_ATAN2: {v->textureX = atan2(reflectionZ,reflectionX)/(2.0*PI)+0.5;} break;
  case GL_SPHERE_MAP_DUAL_PARABOLOID: {v->textureX = reflectionX/(fabs(reflectionZ)+1.0)*0.5+0.5;} break;
  }
  switch(context->texGenT) {
  case GL_SPHERE_MAP: {v->textureY = -reflectionY*0.5+0.5;} break;
  case GL_SPHERE_MAP_ATAN2: {v->textureY = -acos((GLdouble)(-reflectionY))/PI;} break;
  case GL_SPHERE_MAP_DUAL_PARABOLOID: {v->textureY = -reflectionY/(fabs(reflectionZ)+1.0)*0.5+0.5;} break;
  }
}

GLvoid glLightVertex(_GLContext *context, glVertex *v) {
  if (glIsEnabled2(context,GL_LIGHTING)) {
    const GLdouble *matrix = context->matrixForMode[GL_MODELVIEW & 1];
    GLdouble x = v->vertexX * matrix[0*4+0] + v->vertexY * matrix[1*4+0] + v->vertexZ * matrix[2*4+0] + v->vertexW * matrix[3*4+0];
    GLdouble y = v->vertexX * matrix[0*4+1] + v->vertexY * matrix[1*4+1] + v->vertexZ * matrix[2*4+1] + v->vertexW * matrix[3*4+1];
    GLdouble z = v->vertexX * matrix[0*4+2] + v->vertexY * matrix[1*4+2] + v->vertexZ * matrix[2*4+2] + v->vertexW * matrix[3*4+2];
    GLdouble w = v->vertexX * matrix[0*4+3] + v->vertexY * matrix[1*4+3] + v->vertexZ * matrix[2*4+3] + v->vertexW * matrix[3*4+3];
    if (w != 0) { // should never happen
      x /= w;
      y /= w;
      z /= w;
    }
    // rotate normal (inversetranspose would be better, or?)
    const GLdouble vnx = v->vertexX+v->normalX;
    const GLdouble vny = v->vertexY+v->normalY;
    const GLdouble vnz = v->vertexZ+v->normalZ;
    const GLdouble vnw = v->vertexW;
    GLdouble nx = vnx * matrix[0*4+0] + vny * matrix[1*4+0] + vnz * matrix[2*4+0] + vnw * matrix[3*4+0];
    GLdouble ny = vnx * matrix[0*4+1] + vny * matrix[1*4+1] + vnz * matrix[2*4+1] + vnw * matrix[3*4+1];
    GLdouble nz = vnx * matrix[0*4+2] + vny * matrix[1*4+2] + vnz * matrix[2*4+2] + vnw * matrix[3*4+2];
    const GLdouble nw = vnx * matrix[0*4+3] + vny * matrix[1*4+3] + vnz * matrix[2*4+3] + vnw * matrix[3*4+3];
    if (nw != 0) { // should never happen
      nx /= nw;
      ny /= nw;
      nz /= nw;
    }
    nx -= x;
    ny -= y;
    nz -= z;

    GLfloat ambientRed = 0;
    GLfloat ambientGreen = 0;
    GLfloat ambientBlue = 0;
    GLfloat diffuseRed = 0;
    GLfloat diffuseGreen = 0;
    GLfloat diffuseBlue = 0;
    GLfloat specularRed = 0;
    GLfloat specularGreen = 0;
    GLfloat specularBlue = 0;

    GLint face = glContext.twoSidedLighting ? (currentBackFacing ? 1:0) : 0;
    GLfloat materialAmbientRed = context->materialRed[face][0];
    GLfloat materialAmbientGreen = context->materialGreen[face][0];
    GLfloat materialAmbientBlue = context->materialBlue[face][0];
    GLfloat materialDiffuseRed = context->materialRed[face][1];
    GLfloat materialDiffuseGreen = context->materialGreen[face][1];
    GLfloat materialDiffuseBlue = context->materialBlue[face][1];
    GLfloat materialDiffuseAlpha = context->materialAlpha[face][1];
    GLfloat materialSpecularRed = context->materialRed[face][2];
    GLfloat materialSpecularGreen = context->materialGreen[face][2];
    GLfloat materialSpecularBlue = context->materialBlue[face][2];
    GLfloat materialEmissionRed = context->materialRed[face][3];
    GLfloat materialEmissionGreen = context->materialGreen[face][3];
    GLfloat materialEmissionBlue = context->materialBlue[face][3];
    GLfloat materialShininessRed = context->materialRed[face][4];

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

    GLdouble normalX = nx;
    GLdouble normalY = ny;
    GLdouble normalZ = nz;

    GLfloat l = (GLfloat)sqrt(normalX*normalX+normalY*normalY+normalZ*normalZ);
    if (l > 0.f) {
      normalX/=l;
      normalY/=l;
      normalZ/=l;
    }

    for (GLint i = 0; i < GLMAXLIGHTS; i++) {
      if (glIsEnabled2(context,GL_LIGHT0+i)) {
        const GLfloat lightAmbientRed = (GLfloat)context->lightRed[0][i];
        const GLfloat lightAmbientGreen = (GLfloat)context->lightGreen[0][i];
        const GLfloat lightAmbientBlue = (GLfloat)context->lightBlue[0][i];
        const GLfloat lightDiffuseRed = (GLfloat)context->lightRed[1][i];
        const GLfloat lightDiffuseGreen = (GLfloat)context->lightGreen[1][i];
        const GLfloat lightDiffuseBlue = (GLfloat)context->lightBlue[1][i];
        const GLfloat lightSpecularRed = (GLfloat)context->lightRed[2][i];
        const GLfloat lightSpecularGreen = (GLfloat)context->lightGreen[2][i];
        const GLfloat lightSpecularBlue = (GLfloat)context->lightBlue[2][i];
        const GLdouble lightPositionX = context->lightRed[3][i];
        const GLdouble lightPositionY = context->lightGreen[3][i];
        const GLdouble lightPositionZ =  context->lightBlue[3][i];
  
        const GLdouble lightPositionW =  context->lightAlpha[3][i];
        GLdouble lVecX = lightPositionX;
        GLdouble lVecY = lightPositionY;
        GLdouble lVecZ = lightPositionZ;
        GLdouble attenuation = context->constantAttenuation[i]; // directionals have no distance based attenuation
        if (fabs(lightPositionW) > 0.0) {
          lVecX-=x;
          lVecY-=y;
          lVecZ-=z;
          const GLdouble d = sqrt(lVecX*lVecX+lVecY*lVecY+lVecZ*lVecZ);
          attenuation = context->constantAttenuation[i]+context->linearAttenuation[i]*d+context->quadraticAttenuation[i]*d*d;
        }
        if (attenuation > 0.0) 
          attenuation = 1.0/attenuation; 
        else 
          attenuation = 0.0;
        GLdouble l = sqrt(lVecX*lVecX+lVecY*lVecY+lVecZ*lVecZ);
        if (l>0.0) {lVecX/=l;  lVecY/=l; lVecZ/=l;}
        GLdouble diffuse = lVecX * normalX + lVecY * normalY + lVecZ * normalZ;
        if (context->twoSidedLighting) {
          diffuse = fabs(diffuse);
        }
        if (diffuse < 0) diffuse = 0;
        GLdouble viewX = x;
        GLdouble viewY = y;
        GLdouble viewZ = z;
        l = sqrt(viewX*viewX+viewY*viewY+viewZ*viewZ);
        if (l > 0.f) {viewX/=l;viewY/=l;viewZ/=l;}
        GLdouble specular;
        const GLboolean halveVector = glUseHalveVector;
        if (halveVector) {
          GLdouble reflectionX = -viewX + lVecX; // halveVector
          GLdouble reflectionY = -viewY + lVecY;
          GLdouble reflectionZ = -viewZ + lVecZ;
          l = sqrt(reflectionX*reflectionX+reflectionY*reflectionY+reflectionZ*reflectionZ);
          if (l > 0.f) {reflectionX/=l; reflectionY /= l; reflectionZ /= l;}
          specular = (normalX * reflectionX + normalY * reflectionY + normalZ * reflectionZ);
        } else {
          GLdouble dot2NI = 2.0*(-lVecX*normalX+-lVecY*normalY+-lVecZ*normalZ);
          GLdouble reflectionX = -lVecX - dot2NI * normalX;
          GLdouble reflectionY = -lVecY - dot2NI * normalY;
          GLdouble reflectionZ = -lVecZ - dot2NI * normalZ;
          l = sqrt(reflectionX*reflectionX+reflectionY*reflectionY+reflectionZ*reflectionZ);
          if (l > 0.f) {reflectionX/=l; reflectionY /= l; reflectionZ /= l;}
          specular = -viewX * reflectionX + -viewY * reflectionY + -viewZ * reflectionZ;
        }
        if (specular < 0.0) specular = 0.0;

        // spot calculations
        const GLdouble spotCutOff = context->spotCutOff[i];
        if (spotCutOff > -0.9999) { // is it a spot light?
          // light position needs w other than 0, denoting no directional
          GLdouble spotDirectionX = context->lightRed[4][i];
          GLdouble spotDirectionY = context->lightGreen[4][i];
          GLdouble spotDirectionZ =  context->lightBlue[4][i];
          GLdouble spotAngleDot = lVecX*spotDirectionX+lVecY*spotDirectionY+lVecZ*spotDirectionZ;
          if (spotAngleDot > spotCutOff) { // spotCutOff goes from 1(0degrees) to -1(180degrees)
            const GLdouble spotExponent = context->spotExponent[i];
            const GLdouble d = pow(spotAngleDot < 0 ? 0 : spotAngleDot,spotExponent);
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
        diffuseRed += (GLfloat)(materialDiffuseRed*lightDiffuseRed*diffuse);
        diffuseGreen += (GLfloat)(materialDiffuseGreen*lightDiffuseGreen*diffuse);
        diffuseBlue += (GLfloat)(materialDiffuseBlue*lightDiffuseBlue*diffuse);
        GLdouble shininess = materialShininessRed;
        if (shininess < 0.001) shininess = 0.001;
        specular = pow(specular,shininess) * attenuation;
        specularRed += (GLfloat)(materialSpecularRed*lightSpecularRed*specular);
        specularGreen += (GLfloat)(materialSpecularGreen*lightSpecularGreen*specular);
        specularBlue += (GLfloat)(materialSpecularBlue*lightSpecularBlue*specular);
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
    ambientRed += materialAmbientRed*context->ambientColorRed;
    ambientGreen += materialAmbientGreen*context->ambientColorGreen;
    ambientBlue += materialAmbientBlue*context->ambientColorBlue;

    v->colorRed = materialEmissionRed + ambientRed + diffuseRed + specularRed;
    v->colorGreen = materialEmissionGreen + ambientGreen + diffuseGreen + specularGreen;
    v->colorBlue = materialEmissionBlue + ambientBlue + diffuseBlue + specularBlue;
    v->colorAlpha = materialDiffuseAlpha;
  }
  if (glIsEnabled2(context,GL_TEXTURE_GEN_S) || glIsEnabled2(context,GL_TEXTURE_GEN_T)) {
    const GLdouble *matrix2 = glGetInverseModelView(context);
    glTexGen_(context,v,v->vertexX-matrix2[3*4+0],v->vertexY-matrix2[3*4+1],v->vertexZ-matrix2[3*4+2],v->normalX,v->normalY,v->normalZ);
  }
}

INLINE GLint glClipVertex(_GLContext *context, glVertex *v) {
  GLint clipFlags = (v->sx < context->viewportX0) ? 1 : 0;
  clipFlags |= (v->sy < context->viewportY0) ? 2 : 0;
  clipFlags |= (v->sx > context->viewportX1) ? 4 : 0;
  clipFlags |= (v->sy > context->viewportY1) ? 8 : 0;
  return clipFlags;
}

GLvoid interpolateVertex(glVertex *dest, glVertex *v0, glVertex *v1, GLdouble f) {
  dest->colorRed = v0->colorRed + (GLfloat)(f * (v1->colorRed-v0->colorRed));
  dest->colorGreen = v0->colorGreen + (GLfloat)(f * (v1->colorGreen-v0->colorGreen));
  dest->colorBlue = v0->colorBlue + (GLfloat)(f * (v1->colorBlue-v0->colorBlue));
  dest->colorAlpha = v0->colorAlpha + (GLfloat)(f * (v1->colorAlpha-v0->colorAlpha));

  if (glContext.separateSpecular) {
    dest->additionalSpecularColorRed = v0->additionalSpecularColorRed + (GLfloat)(f * (v1->additionalSpecularColorRed-v0->additionalSpecularColorRed));
    dest->additionalSpecularColorGreen = v0->additionalSpecularColorGreen + (GLfloat)(f * (v1->additionalSpecularColorGreen-v0->additionalSpecularColorGreen));
    dest->additionalSpecularColorBlue = v0->additionalSpecularColorBlue + (GLfloat)(f * (v1->additionalSpecularColorBlue-v0->additionalSpecularColorBlue));
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

#define GLMAXVERTEXCLIPSTACK (1<<GLMAXCLIPPLANES)
glVertex glClipStack[GLMAXVERTEXCLIPSTACK*3];
GLint glClipStackPos = 0;

GLvoid glTransposeMatrix(GLdouble *m) {
  GLdouble t[4*4];
  memcpy(t,m,sizeof(t));
  for (int x = 0; x < 4; x++)
    for (int y = 0; y < 4; y++)
      m[x+y*4]=t[x*4+y];
}

GLdouble glWorldClipPlanes[GLMAXCLIPPLANES][4];
GLvoid glWorldSpaceClipPlanes() {
  const GLdouble *inverse = glContext.matrixForMode[GL_MODELVIEW & 1]; // we need the transpose here glGetInverseModelView(&glContext);
  GLdouble inverseTranspose[4*4];
  memcpy(inverseTranspose,inverse,sizeof(inverseTranspose));
  glTransposeMatrix(inverseTranspose);
  const GLdouble *matrix = inverseTranspose;
  for (GLint i = 0; i < GLMAXCLIPPLANES; i++) {
    if (glIsEnabled(GL_CLIP_PLANE0+i)) {
      const GLdouble vx = glContext.clipPlanes[i][0];
      const GLdouble vy = glContext.clipPlanes[i][1];
      const GLdouble vz = glContext.clipPlanes[i][2];
      const GLdouble vw = glContext.clipPlanes[i][3];
      GLdouble x2 = vx * matrix[0*4+0] + vy * matrix[1*4+0] + vz * matrix[2*4+0] + vw * matrix[3*4+0];
      GLdouble y2 = vx * matrix[0*4+1] + vy * matrix[1*4+1] + vz * matrix[2*4+1] + vw * matrix[3*4+1];
      GLdouble z2 = vx * matrix[0*4+2] + vy * matrix[1*4+2] + vz * matrix[2*4+2] + vw * matrix[3*4+2];
      GLdouble w2 = vx * matrix[0*4+3] + vy * matrix[1*4+3] + vz * matrix[2*4+3] + vw * matrix[3*4+3];
      glWorldClipPlanes[i][0] = x2;
      glWorldClipPlanes[i][1] = y2;
      glWorldClipPlanes[i][2] = z2;
      glWorldClipPlanes[i][3] = w2;
    }
  }
}

GLvoid glPutOnClipStack(glVertex *v0,glVertex *v1,glVertex *v2) {
  if (glClipStackPos == -1) glClipStackPos=0;
  if (glClipStackPos >= GLMAXVERTEXCLIPSTACK*3) return;
  glClipStack[glClipStackPos+0] = *v0;
  glClipStack[glClipStackPos+1] = *v1;
  glClipStack[glClipStackPos+2] = *v2;
  glClipStackPos += 3;
}

GLint glProceedClip(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2,GLdouble *plane) {
//  const GLdouble *matrix = context->matrixForMode[GL_MODELVIEW & 1];
  glVertex *v[3];
  v[0] = v0; v[1] = v1; v[2] = v2;
  GLdouble x[3],y[3],z[3],w[3],clip[3];
  GLint clipFlags = 0;
  for (GLint j = 0; j < 3; j++) {
//    x[j] = v[j]->vertexX * matrix[0*4+0] + v[j]->vertexY * matrix[1*4+0] + v[j]->vertexZ * matrix[2*4+0] + v[j]->vertexW * matrix[3*4+0];
//    y[j] = v[j]->vertexX * matrix[0*4+1] + v[j]->vertexY * matrix[1*4+1] + v[j]->vertexZ * matrix[2*4+1] + v[j]->vertexW * matrix[3*4+1];
//    z[j] = v[j]->vertexX * matrix[0*4+2] + v[j]->vertexY * matrix[1*4+2] + v[j]->vertexZ * matrix[2*4+2] + v[j]->vertexW * matrix[3*4+2];
//    w[j] = v[j]->vertexX * matrix[0*4+3] + v[j]->vertexY * matrix[1*4+3] + v[j]->vertexZ * matrix[2*4+3] + v[j]->vertexW * matrix[3*4+3];
    x[j] =  v[j]->vertexX;
    y[j] =  v[j]->vertexY;
    z[j] =  v[j]->vertexZ;
    w[j] =  v[j]->vertexW;
    if (w[j]!=0) {
      x[j]/=w[j];
      y[j]/=w[j];
      z[j]/=w[j];
      w[j]/=w[j];
    }
    clip[j] = x[j]*plane[0]+y[j]*plane[1]+z[j]*plane[2]+plane[3];
    clipFlags |= (clip[j]>=0.0)?(1<<j):0;
  }
  if (clipFlags == 0) return 0; // not visible
  if (clipFlags == (1|2|4)) return 1; // all visible

  glVertex poly[8];
  GLint count = 0;
  const GLint vertexCount = 3;
  for(GLint i = 0; i < vertexCount; i++) {
    glVertex *v0 = v[i];
    glVertex *v1 = v[(i+1) % vertexCount];
    GLdouble ci0 = clip[i];
    GLdouble ci1 = clip[(i+1) % vertexCount];
    GLboolean currentInside = (ci0>=0.0) ? GL_TRUE : GL_FALSE;
    GLboolean nextInside = (ci1>=0.0) ? GL_TRUE : GL_FALSE;
    if (currentInside && nextInside) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      count++;
    } else
    if(currentInside && (!nextInside)) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      count++;
      GLdouble f = (ci0) / (ci0-ci1);
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else 
    if((!currentInside) && nextInside) {
      GLdouble f = (ci0) / (ci0-ci1);
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      count++;
    }
  }
  for(GLint k = 1; k < count-1; k++) glPutOnClipStack(&poly[0],&poly[k],&poly[k+1]);

  return 2; // partially visible
}

GLvoid glClipOnPlane(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2,GLdouble *plane) {
  if(glClipStackPos == -1) glPutOnClipStack(v0,v1,v2);
  for (GLint i = glClipStackPos-3; i >= 0 ; i -= 3) {
    GLint clipped = glProceedClip(context,&glClipStack[i+0],&glClipStack[i+1],&glClipStack[i+2],plane);
    switch(clipped) {
    case 2: // partially visible the current one needs also to be deleted
    case 0: {for (GLint j=i+3;j<glClipStackPos;j+=3) glClipStack[j-3]=glClipStack[j];glClipStackPos -= 3;} break; // not visible
    case 1: break; // fully visible
    }
  } 
}

GLvoid glClipPlaneTriangle(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2) {
  glClipStackPos=-1;
  for (GLint i = 0; i < GLMAXCLIPPLANES; i++) {
    if (glIsEnabled2(context, GL_CLIP_PLANE0+i)) glClipOnPlane(context,v0,v1,v2,glWorldClipPlanes[i]);//context->clipPlanes[i]);
  }
  if (glClipStackPos==-1)
    glDrawTriangle(context,v0,v1,v2);
  else {
    glVertex *v = glClipStack;
    for (int i = 0; i < glClipStackPos/3; i++) {
      glDrawTriangle(context,&v[0],&v[1],&v[2]);
      v+=3;
    }
  }
}

GLvoid glDrawQuad(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3) {
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
  glClipPlaneTriangle(context,v0,v1,v2);
  glClipPlaneTriangle(context,v2,v3,v0);
}

GLvoid drawClippedNgon(_GLContext *context, glVertex *vertices[], GLint vertexCount) {
  GLdouble *matrix = context->matrix;
  glVertex poly[8];
  GLint count = 0;
  GLint clipFlags = 1|2|4|8;
  for(GLint i = 0; i < vertexCount; i++) {
    glVertex *v0 = vertices[i];
    glVertex *v1 = vertices[(i+1) % vertexCount];
    GLdouble z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
    GLdouble w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
    GLdouble z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
    GLdouble w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
    GLboolean currentInside = (z0 >= -w0) ? GL_TRUE : GL_FALSE;
    GLboolean nextInside = (z1 >= -w1) ? GL_TRUE : GL_FALSE;
    if (currentInside && nextInside) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else
    if(currentInside && (!nextInside)) {
      poly[count]=*v0;
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
      GLdouble f = (w0+z0) / ((z0-z1)+(w0-w1));
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    } else 
    if((!currentInside) && nextInside) {
      GLdouble f = (w0+z0) / ((z0-z1)+(w0-w1));
      interpolateVertex(&poly[count],v0,v1,f);
      glTransformVertex(context,&poly[count],GL_FALSE);
      glLightVertex(context,&poly[count]);
      clipFlags &= glClipVertex(&glContext,&poly[count]);
      count++;
    }
  }
  if (clipFlags == 0) for(GLint j = 1; j < count-1; j++) glClipPlaneTriangle(context, &poly[0],&poly[j],&poly[j+1]);
}

GLvoid drawClippedQuad(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2, glVertex *v3) {
  glVertex *vertices[4];
  vertices[0] = v0;
  vertices[1] = v1;
  vertices[2] = v2;
  vertices[3] = v3;
  drawClippedNgon(context, vertices, 4);
}

GLvoid drawClippedTriangle(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2) {
  glVertex *vertices[3];
  vertices[0] = v0;
  vertices[1] = v1;
  vertices[2] = v2;
  drawClippedNgon(context, vertices, 3);
}

GLvoid drawClippedLine(_GLContext *context, glVertex *v0a, glVertex *v1a)  {
  glVertex vertices[4];
  vertices[0] = *v0a;
  vertices[1] = *v1a;

  GLdouble *matrix = context->matrix;
  glVertex *v0 = &vertices[0];
  glVertex *v1 = &vertices[1];
  GLdouble z0 = v0->vertexX * matrix[0*4+2] + v0->vertexY * matrix[1*4+2] + v0->vertexZ * matrix[2*4+2] + v0->vertexW * matrix[3*4+2];
  GLdouble w0 = v0->vertexX * matrix[0*4+3] + v0->vertexY * matrix[1*4+3] + v0->vertexZ * matrix[2*4+3] + v0->vertexW * matrix[3*4+3];
  GLdouble z1 = v1->vertexX * matrix[0*4+2] + v1->vertexY * matrix[1*4+2] + v1->vertexZ * matrix[2*4+2] + v1->vertexW * matrix[3*4+2];
  GLdouble w1 = v1->vertexX * matrix[0*4+3] + v1->vertexY * matrix[1*4+3] + v1->vertexZ * matrix[2*4+3] + v1->vertexW * matrix[3*4+3];
  GLboolean currentInside = (z0 >= -w0) ? GL_TRUE : GL_FALSE;
  GLboolean nextInside = (z1 >= -w1) ? GL_TRUE : GL_FALSE;
  if ((!currentInside) && (!nextInside))
    return;
  if ((!currentInside) || (!nextInside)) {
    GLdouble f = (w0+z0) / ((z0-z1)+(w0-w1));
    if (!currentInside)
      interpolateVertex(&vertices[0],v0,v1,f);
    else
      interpolateVertex(&vertices[1],v0,v1,f);
  }
  glTransformVertex(context,&vertices[0],GL_FALSE);
  glTransformVertex(context,&vertices[1],GL_FALSE);
  glLightVertex(context,&vertices[0]);
  glLightVertex(context,&vertices[1]);
  memcpy(&vertices[2],&vertices[1],sizeof(glVertex));
  memcpy(&vertices[3],&vertices[0],sizeof(glVertex));
  GLdouble dx = vertices[1].sx-vertices[0].sx;
  GLdouble dy = vertices[1].sy-vertices[0].sy;
  GLdouble l = sqrt(dx*dx+dy*dy);
  if (l > 0.f) {dx/=l;dy/=l;};
  GLdouble nx = dy;
  GLdouble ny = -dx;
  GLdouble d = context->lineWidth*0.5;
  vertices[3].sx += nx*d;
  vertices[3].sy += ny*d;
  vertices[2].sx += nx*d;
  vertices[2].sy += ny*d;
  vertices[1].sx -= nx*d;
  vertices[1].sy -= ny*d;
  vertices[0].sx -= nx*d;
  vertices[0].sy -= ny*d;
  GLint clipFlags = glClipVertex(&glContext,&vertices[0]);
  clipFlags &= glClipVertex(&glContext,&vertices[1]);
  if (clipFlags == 0)
    glDrawQuad(context,&vertices[0],&vertices[1],&vertices[2],&vertices[3]);
}

GLvoid glSetVertex(glVertex *w) {
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

GLboolean backFaceCalc(const glVertex *v0,const glVertex *v1,const glVertex *v2) {
  currentBackFacing = GL_FALSE;
  if (glContext.twoSidedLighting || glContext.wireframe[0] || glContext.wireframe[1]) { // used only for two sided coloring/wireframe currently
    const GLdouble ax = v1->vertexX-v0->vertexX;
    const GLdouble ay = v1->vertexY-v0->vertexY;
    const GLdouble az = v1->vertexZ-v0->vertexZ;
    const GLdouble bx = v2->vertexX-v0->vertexX;
    const GLdouble by = v2->vertexY-v0->vertexY;
    const GLdouble bz = v2->vertexZ-v0->vertexZ;
    const GLdouble nx = ay*bz - az*by;
    const GLdouble ny = az*bx - ax*bz;
    const GLdouble nz = ax*by - ay*bx;
    const GLdouble *o = glContext.matrix;
    if (o[0*4+3] == 0 && o[1*4+3] == 0 && o[2*4+3] == 0) { // check if orthographic projection by x,y,z not influencing w
      const GLdouble *m = glGetInverseModelView(&glContext);
      const GLdouble d = m[4*2+0]*nx+m[4*2+1]*ny+m[4*2+2]*nz;
      currentBackFacing = (d < 0) ? GL_TRUE : GL_FALSE;
      if (glContext.frontFace != GL_CCW) currentBackFacing = !currentBackFacing;

    } else {
      const GLdouble *m = glGetInverseModelView(&glContext);
      const GLdouble vx = v0->vertexX-m[4*3+0];
      const GLdouble vy = v0->vertexY-m[4*3+1];
      const GLdouble vz = v0->vertexZ-m[4*3+2];
      const GLdouble d = vx*nx+vy*ny+vz*nz;
      currentBackFacing = (d > 0) ? GL_TRUE : GL_FALSE;
      if (glContext.frontFace != GL_CCW) currentBackFacing = !currentBackFacing;
    }
  }
  return GL_FALSE;
}

INLINE GLboolean glCCW(const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  const GLfloat dx0 = (GLfloat)(v1->sx - v0->sx);
  const GLfloat dy0 = (GLfloat)(v1->sy - v0->sy);
  const GLfloat dx1 = (GLfloat)(v2->sx - v0->sx);
  const GLfloat dy1 = (GLfloat)(v2->sy - v0->sy);
  return ((dx0*dy1 - dy0*dx1) >= 0.f) ? GL_TRUE : GL_FALSE;
}


INLINE GLboolean isBackFaceCulled(_GLContext *context, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  if (glIsEnabled2(context,GL_CULL_FACE)) {
    if (context->forceNoCull == 0) {
      GLboolean backFacing = glCCW(v0,v1,v2);
      if (context->frontFace == GL_CCW) backFacing = (!backFacing) ? GL_TRUE : GL_FALSE;
      if (context->cullFaceMode == GL_FRONT && backFacing) return GL_TRUE;
      if (context->cullFaceMode == GL_BACK && (!backFacing)) return GL_TRUE;
      if (context->cullFaceMode == GL_FRONT_AND_BACK) return GL_TRUE;
    }
  }   
  return GL_FALSE;
}

GLvoid glTriangleFanVertices() {
  glCurrentVertexElement = 2;
  glVertices[1] = glVertices[2];
  // p0,2,..
}

GLvoid glTriangleStripVertices() {
  glCurrentVertexElement = 2;
  glVertices[0] = glVertices[1];
  glVertices[1] = glVertices[2];
  // 2,1,..
}

GLvoid glQuadStripVertices() {
  glCurrentVertexElement = 2;
  glVertices[0] = glVertices[2];
  glVertices[1] = glVertices[3];
  //2,3,.. to be tested
}

GLvoid glEmitVertex() {
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
      if (glCurrentVertexElement == 2) {
        GLint a = glTransformVertex(&glContext,&glVertices[0],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],GL_TRUE);
        if (a == 0) {
          GLint clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            memcpy(&glVertices[2],&glVertices[1],sizeof(glVertex));
            memcpy(&glVertices[3],&glVertices[0],sizeof(glVertex));
            GLdouble dx = glVertices[1].sx-glVertices[0].sx;
            GLdouble dy = glVertices[1].sy-glVertices[0].sy;
            GLdouble l = sqrt(dx*dx+dy*dy);
            if (l > 0.f) {dx/=l;dy/=l;};
            const GLdouble nx = dy * glContext.zoomX*additionalPointSpriteXStretch;
            const GLdouble ny = -dx * glContext.zoomY;
            const GLdouble d = glContext.lineWidth*0.5;
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
        glContext.beginPrimitiveIndex++;
      }
    } break;
    case GL_LINE_STRIP: {
      if (glCurrentVertexElement == 2) {
        GLint a = glTransformVertex(&glContext,&glVertices[0],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],GL_TRUE);
        if (a == 0) {
          GLint clipFlags = glClipVertex(&glContext,&glVertices[0]);
          clipFlags &= glClipVertex(&glContext,&glVertices[1]);
          if (clipFlags == 0) {
            glLightVertex(&glContext,&glVertices[0]);
            glLightVertex(&glContext,&glVertices[1]);
            memcpy(&glVertices[2],&glVertices[1],sizeof(glVertex));
            memcpy(&glVertices[3],&glVertices[0],sizeof(glVertex));
            GLdouble dx = glVertices[1].sx-glVertices[0].sx;
            GLdouble dy = glVertices[1].sy-glVertices[0].sy;
            GLdouble l = sqrt(dx*dx+dy*dy);
            if (l > 0.f) {dx/=l;dy/=l;};
            const GLdouble nx = dy*glContext.zoomX*additionalPointSpriteXStretch;
            const GLdouble ny = -dx*glContext.zoomY;
            const GLdouble d = glContext.lineWidth*0.5;
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
        glContext.beginPrimitiveIndex++;
      }
    } break;
    case GL_TRIANGLE_FAN:
    case GL_TRIANGLE_STRIP:
    case GL_TRIANGLES: {
      if (glCurrentVertexElement == 3) {
        GLint c0 = 0;
        GLint c1 = 1;
        GLint c2 = 2;
        if (glContext.beginMode==GL_TRIANGLE_STRIP) {
          if (glContext.beginPrimitiveIndex & 1) {
            c0 = 1;
            c1 = 0;
            c2 = 2;
          }
        }
        if (backFaceCalc(&glVertices[0],&glVertices[1],&glVertices[2])) {glCurrentVertexElement = 0;currentBackFacing = GL_FALSE;break;}
        if (glContext.wireframe[currentBackFacing ? 1 : 0]) {
          glVertex vs[3];
          vs[0] = glVertices[0];
          vs[1] = glVertices[1];
          vs[2] = glVertices[2];
          GLint modeBefore = glContext.beginMode;
          GLint indexBefore = glContext.beginPrimitiveIndex;
          glContext.beginMode = GL_LINES; glCurrentVertexElement = 0;
          glSetVertex(&vs[c0]);glEmitVertex();
          glSetVertex(&vs[c1]);glEmitVertex();
          glSetVertex(&vs[c1]);glEmitVertex();
          glSetVertex(&vs[c2]);glEmitVertex();
          glSetVertex(&vs[c2]);glEmitVertex();
          glSetVertex(&vs[c0]);glEmitVertex();
          glSetVertex(&vs[c2]);
          glContext.beginMode = modeBefore; glCurrentVertexElement = 0; currentBackFacing = GL_FALSE;
          glContext.beginPrimitiveIndex = indexBefore;
          if (glContext.beginMode==GL_TRIANGLE_STRIP) glTriangleStripVertices();
          break;
        }
        GLint a = glTransformVertex(&glContext,&glVertices[0],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[2],GL_TRUE);
        if (a == 0) {
          if (!isBackFaceCulled(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2])) {
            GLint clipFlags = glClipVertex(&glContext,&glVertices[0]);
            clipFlags &= glClipVertex(&glContext,&glVertices[1]);
            clipFlags &= glClipVertex(&glContext,&glVertices[2]);
            if (clipFlags == 0) {
              glLightVertex(&glContext,&glVertices[0]);
              glLightVertex(&glContext,&glVertices[1]);
              glLightVertex(&glContext,&glVertices[2]);
              glClipPlaneTriangle(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2]);
            }
          }
        } else {
          if (a != 7) {
            drawClippedTriangle(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2]);
          }
        }
        glCurrentVertexElement = 0; currentBackFacing = GL_FALSE;
        if (glContext.beginMode==GL_TRIANGLE_STRIP) glTriangleStripVertices();
        if (glContext.beginMode==GL_TRIANGLE_FAN) glTriangleFanVertices();
        glContext.beginPrimitiveIndex++;
      }
    } break;
    case GL_QUAD_STRIP:
    case GL_QUADS: {
      if (glCurrentVertexElement == 4) {
        GLint c0 = 0;
        GLint c1 = 1;
        GLint c2 = 2;
        GLint c3 = 3;
        if (glContext.beginMode==GL_QUAD_STRIP) {
          c0 = 0;
          c1 = 2;
          c2 = 3;
          c3 = 1;
        }
        if (backFaceCalc(&glVertices[0],&glVertices[1],&glVertices[2])) {glCurrentVertexElement = 0;currentBackFacing = GL_FALSE;break;}
        if (glContext.wireframe[currentBackFacing ? 1 : 0]) {
          glVertex vs[4];
          vs[0] = glVertices[0];
          vs[1] = glVertices[1];
          vs[2] = glVertices[2];
          vs[3] = glVertices[3];
          GLint modeBefore = glContext.beginMode;
          GLint indexBefore = glContext.beginPrimitiveIndex;
          glContext.beginMode = GL_LINES; glCurrentVertexElement = 0;
          glSetVertex(&vs[c0]);glEmitVertex();
          glSetVertex(&vs[c1]);glEmitVertex();
          glSetVertex(&vs[c1]);glEmitVertex();
          glSetVertex(&vs[c2]);glEmitVertex();
          glSetVertex(&vs[c2]);glEmitVertex();
          glSetVertex(&vs[c3]);glEmitVertex();
          glSetVertex(&vs[c3]);glEmitVertex();
          glSetVertex(&vs[c0]);glEmitVertex();
          glSetVertex(&vs[c3]);
          glContext.beginMode = modeBefore; glCurrentVertexElement = 0; currentBackFacing = GL_FALSE;
          glContext.beginPrimitiveIndex = indexBefore;
          if (glContext.beginMode==GL_QUAD_STRIP) glQuadStripVertices();
          break;
        }
        GLint a = glTransformVertex(&glContext,&glVertices[0],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[1],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[2],GL_TRUE);
        a *= 2;
        a |= glTransformVertex(&glContext,&glVertices[3],GL_TRUE);
        if (a == 0) {
          if (!isBackFaceCulled(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2])) {
            GLint clipFlags = glClipVertex(&glContext,&glVertices[0]);
            clipFlags &= glClipVertex(&glContext,&glVertices[1]);
            clipFlags &= glClipVertex(&glContext,&glVertices[2]);
            clipFlags &= glClipVertex(&glContext,&glVertices[3]);
            if (clipFlags == 0) {
              glLightVertex(&glContext,&glVertices[0]);
              glLightVertex(&glContext,&glVertices[1]);
              glLightVertex(&glContext,&glVertices[2]);
              glLightVertex(&glContext,&glVertices[3]);
              glDrawQuad(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2],&glVertices[c3]);
            }
          }
        } else {
          if (a != 15) {
            drawClippedQuad(&glContext,&glVertices[c0],&glVertices[c1],&glVertices[c2],&glVertices[c3]);
          }
        }
        glCurrentVertexElement = 0; currentBackFacing = GL_FALSE;
        if (glContext.beginMode==GL_QUAD_STRIP) glQuadStripVertices();
        glContext.beginPrimitiveIndex++;
      }
    } break;
    case GL_POINTS: {
      if (glCurrentVertexElement == 1) {
        GLint a = glTransformVertex(&glContext,&glVertices[0],GL_TRUE);
        if (a == 0) {
          glVertices[1] = glVertices[0];
          glVertices[2] = glVertices[0];
          glVertices[3] = glVertices[0];
          GLdouble pointSize=glContext.pointSize;
          if (glContext.pointAttenuation[0]!=1.f||glContext.pointAttenuation[1]!=0.f||glContext.pointAttenuation[2]!=0.f) {
            const GLdouble *cm = glContext.matrixForMode[GL_MODELVIEW & 1];
            const glVertex *v = &glVertices[0];
            const GLdouble vx = v->vertexX;
            const GLdouble vy = v->vertexY;
            const GLdouble vz = v->vertexZ;
            const GLdouble vw = v->vertexW;
            GLdouble z = vx * cm[0*4+2] + vy * cm[1*4+2] + vz * cm[2*4+2] + vw * cm[3*4+2];
            const GLdouble w = vx * cm[0*4+3] + vy * cm[1*4+3] + vz * cm[2*4+3] + vw * cm[3*4+3];
            if (w != 0) 
              z/=w;
            const GLdouble d = fabs(z);
            GLdouble s = glContext.pointAttenuation[0]+glContext.pointAttenuation[1]*d+glContext.pointAttenuation[2]*d*d;
            if (s != 0) s = 1.0 / s;
            pointSize *= sqrt(fabs(s));
          }
          if (pointSize<glContext.pointSizeMin) pointSize=glContext.pointSizeMin;
          if (pointSize>glContext.pointSizeMax) pointSize=glContext.pointSizeMax;
          const GLdouble pointSizeX = pointSize*0.5*glContext.zoomX*additionalPointSpriteXStretch;
          const GLdouble pointSizeY = pointSize*0.5*glContext.zoomY;
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
          GLint clipFlags = glClipVertex(&glContext,&glVertices[0]);
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
        glContext.beginPrimitiveIndex++;
      }
    } break;
    default: {
        glCurrentVertexElement = 0;
    } break;
  }
}

GLvoid glActiveTexture(GLenum texture) {
  glContext.activeTexture = texture - GL_TEXTURE0;
  glContext.matrixForMode[2] = glContext.textureMatrix[glContext.activeTexture];
}              

GLvoid glAlphaFunc(GLenum func, GLclampf ref) {
  glContext.alphaFunc = func;
  glContext.alphaFuncRef = ref;
}

GLboolean glTextureMatrixIsSet[GLMAXTEXTUREUNITS];

GLvoid glBegin(GLenum mode) {
  //if (glContext.beginMode != 0) {glDone();exit(0);}
  for (int i = 0; i < GLMAXTEXTUREUNITS; i++) glTextureMatrixIsSet[i] = memcmp(glContext.textureMatrix[i],identityMatrix,4*4*sizeof(GLdouble))==0?GL_FALSE:GL_TRUE;
  glWorldSpaceClipPlanes();
  glContext.beginMode = mode;
  glCurrentVertexElement = 0;
  glContext.beginPrimitiveIndex = 0;
}

GLvoid glBindBuffer(GLenum target, GLuint buffer) {
  if (buffer >= GLMAXBUFFERS) {
    glSetError(GL_INVALID_VALUE);
    return;
  }
  switch(target) {
  case GL_ARRAY_BUFFER:{glBoundBuffer=buffer==0?NULL:&glBuffers[buffer];} break;
  case GL_ELEMENT_ARRAY_BUFFER:{glBoundElementBuffer=buffer==0?NULL:&glBuffers[buffer];} break;
  }
}

GLvoid glBindFramebuffer(GLenum target, GLuint buffer) {
  __UNUSED(target);
  if (buffer >= GLMAXBUFFERS) {
    glSetError(GL_INVALID_VALUE);
    return;
  }
  glBoundFrameBuffer=buffer==0?NULL:&glBuffers[buffer];
  if (glBoundFrameBuffer==NULL||glBoundFrameBuffer->colorPointer==NULL) {
    glFrameBufferWidth = glFrameBufferWidth0;
    glFrameBufferHeight = glFrameBufferHeight0;
    glFrameBuffer = glFrameBuffer0;
    glDepthBuffer = glDepthBuffer0;
    glStencilBuffer = glStencilBuffer0;
    return;
  }
  glFrameBuffer = glBoundFrameBuffer->colorPointer;
  glFrameBufferWidth = glBoundFrameBuffer->colorWidth;
  glFrameBufferHeight = glBoundFrameBuffer->colorHeight;
  glDepthBuffer = glBoundFrameBuffer->depthPointer; // must be same width/height like color (or NULL)
  glStencilBuffer = glBoundFrameBuffer->stencilPointer; // must be same width/height like color (or NULL)
}

GLvoid glBindTexture(GLenum target, GLuint texture) {
  __UNUSED(target);
  if (texture >= GLMAXTEXTURES) {
    glSetError(GL_INVALID_VALUE);
    return;
  }
  glContext.boundTextures[glContext.activeTexture] = texture;
}

GLvoid glBlendFunc(GLenum sfactor, GLenum dfactor) {
  glContext.blendFuncSFactor = sfactor;
  glContext.blendFuncDFactor = dfactor;
}

GLvoid glBufferData(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage) {
  __UNUSED(usage);
  GLBuffer *c = NULL;
  switch(target) {
  case GL_ARRAY_BUFFER: {c = glGetCurrentBuffer();} break;
  case GL_ELEMENT_ARRAY_BUFFER: {c = glGetCurrentElementBuffer();} break;
  }
  if (c == NULL) {
    glSetError(GL_INVALID_VALUE);
    return;
  }
  if (c->data != NULL) {__FREE(c->data); c->data = NULL;}
  c->data = __MALLOC(size);
  memset(c->data,0,size);
  c->dataSize = size;
  glBufferSubData(target,0,size,data);
}

GLvoid glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data) {
  GLBuffer *c = NULL;
  switch(target) {
  case GL_ARRAY_BUFFER: {c = glGetCurrentBuffer();} break;
  case GL_ELEMENT_ARRAY_BUFFER: {c = glGetCurrentElementBuffer();} break;
  }
  if ((c == NULL) || (c->data==NULL) || (offset+size>c->dataSize) || (offset<0)) {
    glSetError(GL_INVALID_VALUE);
    return;
  }
  if (data != NULL) 
    memcpy((GLvoid*)((GLuint)c->data+offset),data,size);
  else
    memset((GLvoid*)((GLuint)c->data+offset),0,size);
}

GLenum glCheckFramebufferStatus(GLuint buffer) {
  if (buffer >= GLMAXBUFFERS) {
    glSetError(GL_INVALID_VALUE);
    return GL_ERROR;
  }
  GLBuffer *c = &glBuffers[buffer];
  if (c->name != buffer) return GL_ERROR;
  if (c->colorPointer == NULL) return GL_ERROR;
  if (c->depthPointer == NULL) return GL_NO_ERROR;
  if ((c->colorWidth != c->depthWidth)||(c->colorHeight != c->depthHeight)) return GL_ERROR;
  if (c->stencilPointer == NULL) return GL_NO_ERROR;
  if ((c->colorWidth != c->stencilWidth)||(c->colorHeight != c->stencilHeight)) return GL_ERROR;
  return GL_NO_ERROR;
}

GLvoid glClear(GLbitfield mask) {
  GLint minX = 0;
  GLint minY = 0;
  GLint maxX = glFrameBufferWidth;
  GLint maxY = glFrameBufferHeight;
  if (glIsEnabled(GL_SCISSOR_TEST)) {
    combineIntoWindow(&minX,&minY,&maxX,&maxY,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
  }
  if ((mask & GL_COLOR_BUFFER_BIT) && (glFrameBuffer != NULL)) {
    GLint r = (GLint)FLOOR(glContext.clearRed*255.f);
    GLint g = (GLint)FLOOR(glContext.clearGreen*255.f);
    GLint b = (GLint)FLOOR(glContext.clearBlue*255.f);
    GLint a = (GLint)FLOOR(glContext.clearAlpha*255.f);
    if (r < 0) r = 0;
    if (g < 0) g = 0;
    if (b < 0) b = 0;
    if (a < 0) a = 0;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    if (glFrameBufferBytesPerPixel == 4) {
      GLuint rgba = r|(g<<8)|(b<<16)|(a<<24);
      for (GLint s = 0; s < glFrameBufferMultiSample; s++) {
        for (GLint y = minY; y < maxY; y++) {
          const int k = y*glFrameBufferWidth+s*glFrameBufferWidth*glFrameBufferHeight;
          for (GLint x = minX; x < maxX; x++)
            glFrameBuffer[x+k] = rgba;
        }
      }
    }
  }    
  if ((mask & GL_DEPTH_BUFFER_BIT) && (glDepthBuffer != NULL)) {
    for (GLint s = 0; s < glFrameBufferMultiSample; s++) {
      for (GLint y = minY; y < maxY; y++) {
        const int k = y*glFrameBufferWidth+s*glFrameBufferWidth*glFrameBufferHeight;
        for (GLint x = minX; x < maxX; x++)
          glDepthBuffer[x+k] = glContext.clearDepth; // clamping?
      }
    }
  }
  if ((mask & GL_STENCIL_BUFFER_BIT) && (glStencilBuffer != NULL)) {
    GLubyte cl = (GLubyte)(glContext.clearStencil & 255);
    for (GLint s = 0; s < glFrameBufferMultiSample; s++) {
      for (GLint y = minY; y < maxY; y++) {
        const int k = y*glFrameBufferWidth+s*glFrameBufferWidth*glFrameBufferHeight;
        for (GLint x = minX; x < maxX; x++)
          glStencilBuffer[x+k] = cl;
      }
    }
  }
}

GLvoid glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glContext.clearRed = red;
  glContext.clearGreen = green;
  glContext.clearBlue = blue;
  glContext.clearAlpha = alpha;
}

GLvoid glClearDepthf(GLclampf depth) {
  glContext.clearDepth = depth;
}

GLvoid glClearDepth(GLclampf depth) {
  glContext.clearDepth = depth;
}

GLvoid glClearStencil(GLint s) {
  glContext.clearStencil = s;
}

GLvoid glClipPlane(GLenum plane, const GLdouble *equation) {
    const GLint p = plane - GL_CLIP_PLANE0;

    const GLdouble *inverse = glGetInverseModelView(&glContext);
    GLdouble inverseTranspose[4*4];
    memcpy(inverseTranspose,inverse,sizeof(inverseTranspose));
    glTransposeMatrix(inverseTranspose);

    const GLdouble *matrix = inverseTranspose;
    const GLdouble vx = equation[0];
    const GLdouble vy = equation[1];
    const GLdouble vz = equation[2];
    const GLdouble vw = equation[3];
    GLdouble x2 = vx * matrix[0*4+0] + vy * matrix[1*4+0] + vz * matrix[2*4+0] + vw * matrix[3*4+0];
    GLdouble y2 = vx * matrix[0*4+1] + vy * matrix[1*4+1] + vz * matrix[2*4+1] + vw * matrix[3*4+1];
    GLdouble z2 = vx * matrix[0*4+2] + vy * matrix[1*4+2] + vz * matrix[2*4+2] + vw * matrix[3*4+2];
    GLdouble w2 = vx * matrix[0*4+3] + vy * matrix[1*4+3] + vz * matrix[2*4+3] + vw * matrix[3*4+3];
    glContext.clipPlanes[p][0] = x2;
    glContext.clipPlanes[p][1] = y2;
    glContext.clipPlanes[p][2] = z2;
    glContext.clipPlanes[p][3] = w2;
}

GLvoid glClipPlanef(GLenum plane, const GLfloat *equation) {
  GLdouble t[4];
  t[0] = (GLdouble)equation[0];
  t[1] = (GLdouble)equation[1];
  t[2] = (GLdouble)equation[2];
  t[3] = (GLdouble)equation[3];
  glClipPlane(plane,t);
}

GLvoid glClientActiveTexture(GLenum texture) {
  glContext.clientActiveTexture = texture;
}

GLvoid glColor1d(GLdouble red) {
  glContext.colorRed = (GLfloat)red;
}

GLvoid glColor1dv(const GLdouble *v) {
  glContext.colorRed = (GLfloat)v[0];
}

GLvoid glColor2d(GLdouble red, GLdouble green) {
  glContext.colorRed = (GLfloat)red;
  glContext.colorGreen = (GLfloat)green;
}

GLvoid glColor2dv(const GLdouble *v) {
  glContext.colorRed = (GLfloat)v[0];
  glContext.colorGreen = (GLfloat)v[1];
}

GLvoid glColor3d(GLdouble red, GLdouble green, GLdouble blue) {
  glContext.colorRed = (GLfloat)red;
  glContext.colorGreen = (GLfloat)green;
  glContext.colorBlue = (GLfloat)blue;
}

GLvoid glColor3dv(const GLdouble *v) {
  glContext.colorRed = (GLfloat)v[0];
  glContext.colorGreen = (GLfloat)v[1];
  glContext.colorBlue = (GLfloat)v[2];
}

GLvoid glColor4d (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha) {
  glContext.colorRed = (GLfloat)red;
  glContext.colorGreen = (GLfloat)green;
  glContext.colorBlue = (GLfloat)blue;
  glContext.colorAlpha = (GLfloat)alpha;
}

GLvoid glColor4dv (const GLdouble *v) {
  glContext.colorRed = (GLfloat)v[0];
  glContext.colorGreen = (GLfloat)v[1];
  glContext.colorBlue = (GLfloat)v[2];
  glContext.colorAlpha = (GLfloat)v[3];
}

GLvoid glColor1f(GLfloat red) {
  glContext.colorRed = red;
}

GLvoid glColor1fv(const GLfloat *v) {
  glContext.colorRed = v[0];
}

GLvoid glColor2f(GLfloat red, GLfloat green) {
  glContext.colorRed = red;
  glContext.colorGreen = green;
}

GLvoid glColor2fv(const GLfloat *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
}

GLvoid glColor3f(GLfloat red, GLfloat green, GLfloat blue) {
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
}

GLvoid glColor3fv(const GLfloat *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
}

GLvoid glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha) {
  glContext.colorRed = red;
  glContext.colorGreen = green;
  glContext.colorBlue = blue;
  glContext.colorAlpha = alpha;
}

GLvoid glColor4fv(const GLfloat *v) {
  glContext.colorRed = v[0];
  glContext.colorGreen = v[1];
  glContext.colorBlue = v[2];
  glContext.colorAlpha = v[3];
}

GLvoid glColor1ub(GLubyte red) {
  glContext.colorRed = (GLfloat)red / 255.f;
}

GLvoid glColor2ub(GLubyte red, GLubyte green) {
  glContext.colorRed = (GLfloat)red / 255.f;
  glContext.colorGreen = (GLfloat)green / 255.f;
}

GLvoid glColor3ub(GLubyte red, GLubyte green, GLubyte blue) {
  glContext.colorRed = (GLfloat)red / 255.f;
  glContext.colorGreen = (GLfloat)green / 255.f;
  glContext.colorBlue = (GLfloat)blue / 255.f;
}

GLvoid glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha) {
  glContext.colorRed = (GLfloat)red / 255.f;
  glContext.colorGreen = (GLfloat)green / 255.f;
  glContext.colorBlue = (GLfloat)blue / 255.f;
  glContext.colorAlpha = (GLfloat)alpha / 255.f;
}

GLvoid glColor1ubv(const GLubyte *v) {
  glContext.colorRed = (GLfloat)v[0] / 255.f;
}

GLvoid glColor2ubv(const GLubyte *v) {
  glContext.colorRed = (GLfloat)v[0] / 255.f;
  glContext.colorGreen = (GLfloat)v[1] / 255.f;
}

GLvoid glColor3ubv(const GLubyte *v) {
  glContext.colorRed = (GLfloat)v[0] / 255.f;
  glContext.colorGreen = (GLfloat)v[1] / 255.f;
  glContext.colorBlue = (GLfloat)v[2] / 255.f;
}

GLvoid glColor4ubv(const GLubyte *v) {
  glContext.colorRed = (GLfloat)v[0] / 255.f;
  glContext.colorGreen = (GLfloat)v[1] / 255.f;
  glContext.colorBlue = (GLfloat)v[2] / 255.f;
  glContext.colorAlpha = (GLfloat)v[3] / 255.f;
}

GLvoid glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha) {
  glContext.maskRed = red;
  glContext.maskGreen = green;
  glContext.maskBlue = blue;
  glContext.maskAlpha = alpha;
}

GLvoid glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  glContext.colorSizeBuffer = size;
  glContext.colorTypeBuffer = type;
  glContext.colorStrideBuffer = stride;
  glContext.colorPointerBuffer = pointer;
}

GLvoid glColorMaterial(GLenum face, GLenum pname) {
  GLint f0 = 0; GLint f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  glContext.colorMaterialFace = face;
  for (GLint f = f0; f < f1; f++)
    glContext.colorMaterial[f] = pname;
}

GLvoid glCullFace(GLenum mode) {
  glContext.cullFaceMode = mode;
}

GLvoid glDeleteBuffers (GLsizei n, GLuint *buffers) {
  for (GLint i = 0; i < n; i++)
    glDeleteBuffer(buffers[i]);
}

GLvoid glDeleteFramebuffers (GLsizei n, GLuint *buffers) {
  for (GLint i = 0; i < n; i++)
    glDeleteBuffer(buffers[i]);
}

GLvoid glDeleteTextures (GLsizei n, GLuint *textures) {
  for (GLint i = 0; i < n; i++)
    glDeleteTexture(textures[i]);
}

GLvoid glDepthFunc(GLenum func) {
  glContext.depthFunc = func;
}

GLvoid glDepthMask(GLboolean flag) {
  glContext.depthMask = flag;
}

GLvoid glDepthRangef(GLclampf zNear, GLclampf zFar) {
  glContext.depthRangeZNear = zNear;
  glContext.depthRangeZFar = zFar;
}

GLvoid glDisable(GLenum cap) {
  _GLContext_enable(cap, GL_FALSE);
}

GLvoid glDisableClientState(GLenum array) {
  switch(array) {
  case GL_VERTEX_ARRAY: {glContext.vertexEnabledBuffer = GL_FALSE;} break;
  case GL_NORMAL_ARRAY: {glContext.normalEnabledBuffer = GL_FALSE;} break;
  case GL_COLOR_ARRAY: {glContext.colorEnabledBuffer = GL_FALSE;} break;
  case GL_TEXTURE_COORD_ARRAY: {glContext.texEnabledBuffer = GL_FALSE;} break;
  case GL_INDEX_ARRAY: {glContext.indexEnabledBuffer = GL_FALSE;} break;
  }
}

GLvoid glGetDoubles(GLdouble *dest,GLsizei size, GLenum type,GLsizei stride,const GLvoid *pointer,GLint index, GLboolean normalized) {
  GLint siz=0;
  switch(type) {
  case GL_UNSIGNED_BYTE: siz = 1; break;
  case GL_BYTE: siz = 1; break;
  case GL_SHORT: siz = 2; break;
  case GL_UNSIGNED_SHORT: siz = 2; break;
  case GL_INT: siz = 4; break;
  case GL_UNSIGNED_INT: siz = 4; break;
  case GL_FLOAT: siz = 4; break;
  case GL_2_BYTES: siz = 2; break;
  case GL_3_BYTES: siz = 3; break;
  case GL_4_BYTES: siz = 4; break;
  case GL_DOUBLE: siz = 8; break;
  }
  if (stride == 0) {
    stride = siz*size;
  }
  GLBuffer *c = glGetCurrentBuffer();
  GLuint ptr = (GLuint)pointer + (GLuint)c->data;
  if (ptr < 256) {*dest = 0; return;} // pointer sanity check (this can not happen in sane environments)
  pointer = (const GLvoid*)(ptr + index*stride); // c->data may be NULL
  for (GLint i = 0; i < size; i++) {
    GLdouble a = 0;
    switch(type) {
      case GL_UNSIGNED_BYTE: a = (GLdouble)(*((GLubyte*)pointer)); if (normalized) a /= 255.0; break;
      case GL_BYTE: a = (GLdouble)(*((GLbyte*)pointer));  if (normalized) a /= 127.0; break;
      case GL_SHORT: a = (GLdouble)(*((GLshort*)pointer));  if (normalized) a /= 65535.0/2.0; break;
      case GL_UNSIGNED_SHORT: a = (GLdouble)(*((GLushort*)pointer)); if (normalized) a /= 65535.0; break;
      case GL_INT:  a = (GLdouble)(*((GLint*)pointer));  if (normalized) a /= (GLdouble)(0x7fffffff); break;
      case GL_UNSIGNED_INT: a = (GLdouble)(*((GLuint*)pointer)); a /= (GLdouble)(0xffffffff); break;
      case GL_FLOAT: a = (GLdouble)(*((GLfloat*)pointer)); break;
      case GL_2_BYTES: break;
      case GL_3_BYTES: break;
      case GL_4_BYTES: break;
      case GL_DOUBLE: a = (GLdouble)(*((GLdouble*)pointer)); break;
    }
    pointer = (const GLvoid*)((GLuint)pointer + siz);
    dest[i] = a;
  }
}

GLvoid glBufferedVertex(GLint i) {
  GLdouble a[4];

  if (glContext.colorEnabledBuffer) {
    glGetDoubles(a,glContext.colorSizeBuffer,glContext.colorTypeBuffer,glContext.colorStrideBuffer,glContext.colorPointerBuffer,i, GL_TRUE);
    switch(glContext.colorSizeBuffer) {
    case 1: {glColor1d(a[0]);} break;
    case 2: {glColor2dv(a);} break;
    case 3: {glColor3dv(a);} break;
    case 4: {glColor4dv(a);} break;
    }
  }

  if (glContext.texEnabledBuffer) {
    const GLint b = glContext.activeTexture;
    glGetDoubles(a,glContext.texSizeBuffer[b],glContext.texTypeBuffer[b],glContext.texStrideBuffer[b],glContext.texPointerBuffer[b],i, GL_FALSE);
    switch(glContext.texSizeBuffer[b]) {
    case 1: {glTexCoord1d(a[0]);} break;
    case 2: {glTexCoord2dv(a);} break;
    case 3: {glTexCoord3dv(a);} break;
    case 4: {glTexCoord4dv(a);} break;
    }
  }

  if (glContext.normalEnabledBuffer) {
    glGetDoubles(a,glContext.normalSizeBuffer,glContext.normalTypeBuffer,glContext.normalStrideBuffer,glContext.normalPointerBuffer,i, GL_FALSE);
    glNormal3dv(a);
  }

  if (glContext.vertexEnabledBuffer) {
    glGetDoubles(a,glContext.vertexSizeBuffer,glContext.vertexTypeBuffer,glContext.vertexStrideBuffer,glContext.vertexPointerBuffer,i, GL_FALSE);
    switch(glContext.vertexSizeBuffer) {
    case 1: {glVertex1d(a[0]);} break;
    case 2: {glVertex2dv(a);} break;
    case 3: {glVertex3dv(a);} break;
    case 4: {glVertex4dv(a);} break;
    }
  }
}

GLvoid glDrawArrays(GLenum mode, GLint first, GLsizei count) {
  glBegin(mode);
  for (GLint i = 0; i < count; i++) {
    glBufferedVertex(first+i);
  }
  glEnd();
}

GLvoid glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices) {
  GLint siz=0;
  switch(type) {
  case GL_SHORT: siz = 2; break;
  case GL_UNSIGNED_SHORT: siz = 2; break;
  case GL_INT: siz = 4; break;
  case GL_UNSIGNED_INT: siz = 4; break;
  }
  GLBuffer *c = glGetCurrentElementBuffer();
  GLuint cdata = (GLuint)c->data; // c->data may be NULL
  if (!glContext.indexEnabledBuffer) cdata = 0;
  indices = (const GLvoid*)((GLuint)indices + cdata);
  glBegin(mode);
  for (GLint i = 0; i < count; i++) {
    GLint a = 0;
    switch(type) {
      case GL_SHORT: a = (GLint)(*((GLshort*)indices)); break;
      case GL_UNSIGNED_SHORT: a = (GLint)(*((GLushort*)indices)); break;
      case GL_INT:  a = (GLint)(*((GLint*)indices)); break;
      case GL_UNSIGNED_INT: a = (GLint)(*((GLuint*)indices)); break;
    }
    glBufferedVertex(a);
    indices = (const GLvoid*)((GLuint)indices + siz);
  }
  glEnd();
}

GLvoid glEnable(GLenum cap) {
  _GLContext_enable(cap, GL_TRUE);
}

GLvoid glEnableClientState(GLenum array) {
  switch(array) {
  case GL_VERTEX_ARRAY: {glContext.vertexEnabledBuffer = GL_TRUE;} break;
  case GL_NORMAL_ARRAY: {glContext.normalEnabledBuffer = GL_TRUE;} break;
  case GL_COLOR_ARRAY: {glContext.colorEnabledBuffer = GL_TRUE;} break;
  case GL_TEXTURE_COORD_ARRAY: {glContext.texEnabledBuffer = GL_TRUE;} break;
  case GL_INDEX_ARRAY: {glContext.indexEnabledBuffer = GL_TRUE;} break;
  }
}

GLvoid glEnd() {
  glContext.beginMode = 0;
}

GLvoid glFinish() {
}

GLvoid glFlush() {
}

GLvoid glFogfv(GLenum pname, GLfloat *params) {
  switch(pname) {
  case GL_FOG_COLOR: {memcpy(glContext.fogColor,params,4*sizeof(GLfloat));} break;
  case GL_FOG_START: {glContext.fogStart = params[0];} break;
  case GL_FOG_END: {glContext.fogEnd = params[0];} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = params[0];} break;
  case GL_FOG_MODE: {glContext.fogMode = (GLenum)params[0];} break;
  }
}

GLvoid glFogf(GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = param;} break;
  case GL_FOG_END: {glContext.fogEnd = param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = param;} break;
  case GL_FOG_MODE: {glContext.fogMode = (GLenum)param;} break;
  }
}

GLvoid glFogi(GLenum pname, GLint param) {
  switch(pname) {
  case GL_FOG_START: {glContext.fogStart = (GLfloat)param;} break;
  case GL_FOG_END: {glContext.fogEnd = (GLfloat)param;} break;
  case GL_FOG_DENSITY: {glContext.fogDensity = (GLfloat)param;} break;
  case GL_FOG_MODE: {glContext.fogMode = param;} break;
  }
}              

GLvoid glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level) {
  __UNUSED(target);
  __UNUSED(textarget);
  __UNUSED(level);
  if (glBoundFrameBuffer==NULL) return;
  GLBuffer *c = glBoundFrameBuffer;
  glTexture *t = &glTextures[texture];
  switch(attachment) {
  case GL_COLOR_ATTACHMENT0: {c->colorWidth=t->width;c->colorHeight=t->height;c->colorPointer=(GLuint*)t->data;} break;
  case GL_DEPTH_ATTACHMENT: {c->depthWidth=t->width;c->depthHeight=t->height;c->depthPointer=(GLfloat*)t->data;} break;
  case GL_STENCIL_ATTACHMENT: {c->stencilWidth=t->width;c->stencilHeight=t->height;c->stencilPointer=(GLubyte*)t->data;} break;
  }
}

GLvoid glFrontFace(GLenum mode) {
  glContext.frontFace = mode;
}

GLvoid glGenBuffers(GLsizei n, GLuint *buffers) {
  for(GLint i = 0; i < n; i++) {
    buffers[i] = glNewBuffer();
  }
}

GLvoid glGenFramebuffers(GLsizei n, GLuint *buffers) {
  for(GLint i = 0; i < n; i++) {
    buffers[i] = glNewBuffer();
  }
}

GLvoid glGenTextures (GLsizei n, GLuint *textures) {
  for(GLint i = 0; i < n; i++) {
    textures[i] = glNewTexture();
  }
}

GLvoid glGetBooleanv(GLenum pname, GLboolean *params) {
  GLint i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=(GLboolean)glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=(GLboolean)glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params=(GLboolean)glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params=(GLboolean)glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=(GLboolean)(GLint)floor(glContext.clearRed);params[1]=(GLboolean)(GLint)floor(glContext.clearGreen);params[2]=(GLboolean)(GLint)floor(glContext.clearBlue);params[3]=(GLboolean)(GLint)floor(glContext.clearAlpha);} break;
  case GL_COLOR_MATERIAL_FACE: {*params=(GLboolean)glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params=(GLboolean)glContext.colorMaterial[glContext.colorMaterialFace == GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params=(GLboolean)glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]=(GLboolean)(GLint)floor(glContext.normalX);params[1]=(GLboolean)(GLint)floor(glContext.normalY);params[2]=(GLboolean)(GLint)floor(glContext.normalZ);} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]=(GLboolean)(GLint)floor(glContext.textureX);params[1]=(GLboolean)(GLint)floor(glContext.textureY);params[2]=(GLboolean)(GLint)floor(glContext.textureZ);params[3]=(GLboolean)(GLint)floor(glContext.textureW);} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=(GLboolean)(GLint)floor(glContext.clearDepth);} break;
  case GL_DEPTH_FUNC: {*params=(GLboolean)glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=(GLboolean)(GLint)floor(glContext.depthRangeZNear);params[1]=(GLboolean)(GLint)floor(glContext.depthRangeZFar);} break;
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
  case GL_LINE_WIDTH: {*params=(GLboolean)(GLint)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=(GLboolean)glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(GLboolean)(GLint)floor(glContext.pointSize);} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=(GLboolean)glContext.scissorX0;params[1]=(GLboolean)(glFrameBufferHeight-glContext.scissorY1);params[2]=(GLboolean)(glContext.scissorX1-glContext.scissorX0);params[3]=(GLboolean)(glContext.scissorY1-glContext.scissorY0);} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=(GLboolean)glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=(GLboolean)glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=(GLboolean)glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLboolean)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=(GLboolean)glContext.viewportX0;params[1]=(GLboolean)(glFrameBufferHeight-glContext.viewportY1);params[2]=(GLboolean)(glContext.viewportX1-glContext.viewportX0);params[3]=(GLboolean)(glContext.viewportY1-glContext.viewportY0);} break;
  case GL_ZOOM_X: {*params=(GLboolean)(GLint)floor(glContext.zoomX);} break;
  case GL_ZOOM_Y: {*params=(GLboolean)(GLint)floor(glContext.zoomY);;} break;
  }
}

GLvoid glGetDoublev(GLenum pname, GLdouble *params) {
  GLint i;
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
  case GL_COLOR_MATERIAL_PARAMETER: {*params=glContext.colorMaterial[glContext.colorMaterialFace == GL_BACK?1:0];} break;
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
  case GL_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=glContext.pointSize;} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=glContext.scissorX0;params[1]=glFrameBufferHeight-glContext.scissorY1;params[2]=glContext.scissorX1-glContext.scissorX0;params[3]=glContext.scissorY1-glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]=glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]=glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=glContext.viewportX0;params[1]=glFrameBufferHeight-glContext.viewportY1;params[2]=glContext.viewportX1-glContext.viewportX0;params[3]=glContext.viewportY1-glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params=glContext.zoomX;} break;
  case GL_ZOOM_Y: {*params=glContext.zoomY;;} break;
  }
}

GLvoid glGetFloatv(GLenum pname, GLfloat *params) {
  GLint i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=(GLfloat)glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params= (GLfloat)glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params= (GLfloat)glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=glContext.clearRed;params[1]=glContext.clearGreen;params[2]=glContext.clearBlue;params[3]=glContext.clearAlpha;} break;
  case GL_COLOR_MATERIAL_FACE: {*params= (GLfloat)glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params= (GLfloat)glContext.colorMaterial[glContext.colorMaterialFace == GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params= (GLfloat)glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]= (GLfloat)glContext.normalX;params[1]= (GLfloat)glContext.normalY;params[2]= (GLfloat)glContext.normalZ;} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]= (GLfloat)glContext.textureX;params[1]= (GLfloat)glContext.textureY;params[2]= (GLfloat)glContext.textureZ;params[3]= (GLfloat)glContext.textureW;} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=glContext.clearDepth;} break;
  case GL_DEPTH_FUNC: {*params= (GLfloat)glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=glContext.depthRangeZNear;params[1]=glContext.depthRangeZFar;} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]=glContext.fogColor[0];params[1]=glContext.fogColor[1];params[2]=glContext.fogColor[2];params[3]=glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]=glContext.fogStart;} break;
  case GL_FOG_END: { params[0]=glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]=glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]= (GLfloat)glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params= (GLfloat)(glContext.cullFaceMode == GL_BACK ? 1 : 0);} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=glContext.lineWidth;} break;
  case GL_MATRIX_MODE: {*params= (GLfloat)glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLfloat)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=glContext.pointSize;} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLfloat)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]= (GLfloat)glContext.scissorX0;params[1]= (GLfloat)glFrameBufferHeight- (GLfloat)glContext.scissorY1;params[2]= (GLfloat)glContext.scissorX1- (GLfloat)glContext.scissorX0;params[3]= (GLfloat)glContext.scissorY1- (GLfloat)glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params= (GLfloat)glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params= (GLfloat)glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params= (GLfloat)glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLfloat)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLfloat)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]= (GLfloat)glContext.viewportX0;params[1]= (GLfloat)glFrameBufferHeight- (GLfloat)glContext.viewportY1;params[2]= (GLfloat)glContext.viewportX1- (GLfloat)glContext.viewportX0;params[3]= (GLfloat)glContext.viewportY1- (GLfloat)glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params= (GLfloat)glContext.zoomX;} break;
  case GL_ZOOM_Y: {*params= (GLfloat)glContext.zoomY;} break;
  }
}

GLvoid glGetIntegerv(GLenum pname, GLint *params) {
  GLint i;
  switch(pname) {
  case GL_ALPHA_BITS: {*params=8;} break;
  case GL_ALPHA_TEST: {*params=glIsEnabled(GL_ALPHA_TEST);} break;
  case GL_ALPHA_TEST_FUNC: {*params=glContext.alphaFunc;} break;
  case GL_ALPHA_TEST_REF: {*params=(GLint)glContext.alphaFuncRef;} break;
  case GL_BLEND: {*params=glIsEnabled(GL_BLEND);} break;
  case GL_BLEND_DST: {*params=glContext.blendFuncDFactor;} break;
  case GL_BLEND_SRC: {*params=glContext.blendFuncSFactor;} break;
  case GL_BLUE_BITS: {*params=8;} break;
  case GL_COLOR_CLEAR_VALUE: {params[0]=(GLint)floor(glContext.clearRed);params[1]=(GLint)floor(glContext.clearGreen);params[2]=(GLint)floor(glContext.clearBlue);params[3]=(GLint)floor(glContext.clearAlpha);} break;
  case GL_COLOR_MATERIAL_FACE: {*params=glContext.colorMaterialFace;} break;
  case GL_COLOR_MATERIAL_PARAMETER: {*params=glContext.colorMaterial[glContext.colorMaterialFace == GL_BACK?1:0];} break;
  case GL_CULL_FACE: {*params=glIsEnabled(GL_CULL_FACE);} break;
  case GL_CULL_FACE_MODE: {*params=glContext.cullFaceMode;} break;
  case GL_CURRENT_NORMAL: {params[0]=(GLint)floor(glContext.normalX);params[1]=(GLint)floor(glContext.normalY);params[2]=(GLint)floor(glContext.normalZ);} break;
  case GL_CURRENT_TEXTURE_COORDS: {params[0]=(GLint)floor(glContext.textureX);params[1]=(GLint)floor(glContext.textureY);params[2]=(GLint)floor(glContext.textureZ);params[3]=(GLint)floor(glContext.textureW);} break;
  case GL_DEPTH_CLEAR_VALUE: {*params=(GLint)floor(glContext.clearDepth);} break;
  case GL_DEPTH_FUNC: {*params=glContext.depthFunc;} break;
  case GL_DEPTH_RANGE: {params[0]=(GLint)floor(glContext.depthRangeZNear);params[1]=(GLint)floor(glContext.depthRangeZFar);} break;
  case GL_DEPTH_TEST: {*params=glIsEnabled(GL_DEPTH_TEST);} break;
  case GL_DEPTH_WRITEMASK: {*params=glContext.depthMask;} break;
  case GL_FOG_COLOR: {params[0]= (GLint)glContext.fogColor[0];params[1]= (GLint)glContext.fogColor[1];params[2]= (GLint)glContext.fogColor[2];params[3]= (GLint)glContext.fogColor[3];} break;
  case GL_FOG_START: {params[0]= (GLint)glContext.fogStart;} break;
  case GL_FOG_END: { params[0]= (GLint)glContext.fogEnd;} break;
  case GL_FOG_DENSITY: {params[0]= (GLint)glContext.fogDensity;} break;
  case GL_FOG_MODE: {params[0]=glContext.fogMode;} break;
  case GL_FRONT_FACE: {*params=glContext.cullFaceMode == GL_BACK ? 1 : 0;} break;
  case GL_GREEN_BITS: {*params=8;} break;
  case GL_LIGHT0: {*params=glIsEnabled(GL_LIGHT0);} break;
  case GL_LIGHT1: {*params=glIsEnabled(GL_LIGHT1);} break;
  case GL_LIGHTING: {*params=glIsEnabled(GL_LIGHTING);} break;
  case GL_LINE_WIDTH: {*params=(GLint)floor(glContext.lineWidth);} break;
  case GL_MATRIX_MODE: {*params=glContext.matrixModeNr;} break;
  case GL_MAX_LIGHTS: {*params=GLMAXLIGHTS;} break;
  case GL_MAX_MODELVIEW_STACK_DEPTH: {*params=__MATRIX_STACK_SIZE__;} break;
  case GL_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLint)glContext.matrixForMode[GL_MODELVIEW&1][i];} break;
  case GL_NORMALIZE: {*params=glIsEnabled(GL_NORMALIZE);} break;
  case GL_POINT_SIZE: {*params=(GLint)floor(glContext.pointSize);} break;
  case GL_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLint)glContext.matrixForMode[GL_PROJECTION&1][i];} break;
  case GL_RED_BITS: {*params=8;} break;
  case GL_SCISSOR_BOX: {params[0]=glContext.scissorX0;params[1]=glFrameBufferHeight-glContext.scissorY1;params[2]=glContext.scissorX1-glContext.scissorX0;params[3]=glContext.scissorY1-glContext.scissorY0;} break;
  case GL_SCISSOR_TEST: {*params=glIsEnabled(GL_SCISSOR_TEST);} break;
  case GL_STENCIL_BITS: {*params=8;} break;
  case GL_STENCIL_CLEAR_VALUE: {*params=glContext.clearStencil;} break;
  case GL_STENCIL_FUNC: {*params=glContext.stencilFunc;} break;
  case GL_STENCIL_REF: {*params=glContext.stencilFuncRef;} break;
  case GL_STENCIL_TEST: {*params=glIsEnabled(GL_STENCIL_TEST);} break;
  case GL_TEXTURE_2D: {*params=glIsEnabled(GL_TEXTURE_2D);} break;
  case GL_TRANSPOSE_PROJECTION_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLint)glContext.matrixForMode[GL_PROJECTION&1][(i&3)*4+i/4];} break;
  case GL_TRANSPOSE_MODELVIEW_MATRIX: {for (i=0;i < 16;i++) params[i]= (GLint)glContext.matrixForMode[GL_MODELVIEW&1][(i&3)*4+i/4];} break;
  case GL_VIEWPORT: {params[0]=glContext.viewportX0;params[1]=glFrameBufferHeight-glContext.viewportY1;params[2]=glContext.viewportX1-glContext.viewportX0;params[3]=glContext.viewportY1-glContext.viewportY0;} break;
  case GL_ZOOM_X: {*params=(GLint)floor(glContext.zoomX);} break;
  case GL_ZOOM_Y: {*params=(GLint)floor(glContext.zoomY);;} break;
  }
}

GLenum glGetError() {
  GLenum k = glError;
  glError = GL_NO_ERROR;
  return k;
}

GLvoid glGetLightfv(GLenum light, GLenum pname, GLfloat *params) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {params[0]=glContext.constantAttenuation[light-GL_LIGHT0];} break;
  case GL_LINEAR_ATTENUATION: {params[0]=glContext.linearAttenuation[light-GL_LIGHT0];} break;
  case GL_QUADRATIC_ATTENUATION: {params[0]=glContext.quadraticAttenuation[light-GL_LIGHT0];} break;
  case GL_SPOT_EXPONENT: {params[0]=glContext.spotExponent[light-GL_LIGHT0];} break;
  case GL_SPOT_CUTOFF: {params[0]=(GLfloat)(acos((GLdouble)(glContext.spotCutOff[light-GL_LIGHT0])*180.0/PI));} break;
  case GL_SPOT_DIRECTION: // spot_direction already normalized
  case GL_POSITION:
  case GL_DIFFUSE:
  case GL_SPECULAR:
  case GL_AMBIENT: {
      params[0] = (GLfloat)glContext.lightRed[pname & 7][light - GL_LIGHT0];
      params[1] = (GLfloat)glContext.lightGreen[pname & 7][light - GL_LIGHT0];
      params[2] = (GLfloat)glContext.lightBlue[pname & 7][light - GL_LIGHT0];
      params[3] = (GLfloat)glContext.lightAlpha[pname & 7][light - GL_LIGHT0];
    }; break;
  }
}

const GLubyte *glGetString(GLenum name) {
  __UNUSED(name);
  return (const GLubyte *)"";
}

GLboolean glIsBuffer(GLuint buffer) {
  if (buffer < GLMAXBUFFERS) {
    return glBuffers[buffer].name != 0x00 ? GL_TRUE : GL_FALSE;
  }
  return GL_FALSE;
}

GLboolean glIsEnabled(GLenum cap) {
  return _GLContext_isEnabled(cap);
}  

GLvoid glLightf(GLenum light, GLenum pname, GLfloat param) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = param; return;}
  case GL_SPOT_EXPONENT: {glContext.spotExponent[light - GL_LIGHT0] = param; return;}
  case GL_SPOT_CUTOFF: {glContext.spotCutOff[light - GL_LIGHT0] = (GLfloat)cos(param*PI/180.0); return;}
  }
}

GLvoid glLightfv(GLenum light, GLenum pname, const GLfloat *params) {
  switch(pname) {
  case GL_CONSTANT_ATTENUATION: {glContext.constantAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_LINEAR_ATTENUATION: {glContext.linearAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_QUADRATIC_ATTENUATION: {glContext.quadraticAttenuation[light - GL_LIGHT0] = params[0]; return;}
  case GL_SPOT_EXPONENT: {glContext.spotExponent[light - GL_LIGHT0] = params[0]; return;}
  case GL_SPOT_CUTOFF: {glContext.spotCutOff[light - GL_LIGHT0] = (GLfloat)cos(params[0]*PI/180); return;}
  }

  if (pname == GL_SPOT_DIRECTION) {
    GLdouble lx = params[0];
    GLdouble ly = params[1];
    GLdouble lz = params[2];
    GLdouble *matrix = glContext.matrixForMode[GL_MODELVIEW&1]; 
    GLdouble xl = lx * matrix[0*4+0] + ly * matrix[1*4+0] + lz * matrix[2*4+0];
    GLdouble yl = lx * matrix[0*4+1] + ly * matrix[1*4+1] + lz * matrix[2*4+1];
    GLdouble zl = lx * matrix[0*4+2] + ly * matrix[1*4+2] + lz * matrix[2*4+2];
    GLdouble k = sqrt(xl*xl+yl*yl+zl*zl);
    if (fabs(k) > 0) {xl/=k;yl/=k;zl/=k;}
    glContext.lightRed[pname & 7][light - GL_LIGHT0] = xl;
    glContext.lightGreen[pname & 7][light - GL_LIGHT0] = yl;
    glContext.lightBlue[pname & 7][light - GL_LIGHT0] = zl;
    return;
  }
  if (pname == GL_POSITION) {
    GLdouble lx = params[0];
    GLdouble ly = params[1];
    GLdouble lz = params[2];
    GLdouble lw = params[3];

    GLdouble *matrix = glContext.matrixForMode[GL_MODELVIEW&1]; 
    GLdouble xl = lx * matrix[0*4+0] + ly * matrix[1*4+0] + lz * matrix[2*4+0] + lw * matrix[3*4+0];
    GLdouble yl = lx * matrix[0*4+1] + ly * matrix[1*4+1] + lz * matrix[2*4+1] + lw * matrix[3*4+1];
    GLdouble zl = lx * matrix[0*4+2] + ly * matrix[1*4+2] + lz * matrix[2*4+2] + lw * matrix[3*4+2];
    GLdouble wl = lx * matrix[0*4+3] + ly * matrix[1*4+3] + lz * matrix[2*4+3] + lw * matrix[3*4+3];
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

GLvoid glLightModelfv(GLenum pname, const GLfloat *params) {
  switch(pname) {
  case GL_LIGHT_MODEL_AMBIENT: glContext.ambientColorRed = params[0]; glContext.ambientColorGreen = params[1]; glContext.ambientColorBlue = params[2]; glContext.ambientColorAlpha = params[3]; return;
  }
  glLightModelf(pname, params[0]); // temporary
}

GLvoid glLightModeli(GLenum pname, const GLenum param) {
  switch(pname) {
  case GL_LIGHT_MODEL_COLOR_CONTROL: {
    switch(param) {
    case GL_SEPARATE_SPECULAR_COLOR: {glContext.separateSpecular = GL_TRUE;} break;
    case GL_SINGLE_COLOR: {glContext.separateSpecular = GL_FALSE;} break;
    }
  } break;
  case GL_LIGHT_MODEL_TWO_SIDE: {glContext.twoSidedLighting = ((GLint)param != 0) ? GL_TRUE : GL_FALSE;} break;
  }
}

GLvoid glLightModelf(GLenum pname, const GLfloat param) {
  switch(pname) {
  case GL_LIGHT_MODEL_TWO_SIDE: {glContext.twoSidedLighting = (param != 0.f) ? GL_TRUE : GL_FALSE;} break;
  }
}

GLvoid glLineStipple(GLint factor, GLushort pattern) {
  glContext.lineStippleFactor = (GLfloat)factor;
  glContext.lineStipplePattern = pattern;
}

GLvoid glLineWidth(GLfloat width) {
  glContext.lineWidth = width;
}

GLvoid glLoadIdentity() {
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], identityMatrix, 4*4*sizeof(GLdouble));
  glUpdateMatrix();
}

GLvoid glLoadMatrixf(const GLfloat *m) {
  for (GLint i = 0; i < 4*4; i++) glContext.matrixForMode[glContext.matrixModeNr][i] = m[i];
  glUpdateMatrix();
}

GLvoid glLoadMatrixd(const GLdouble *m) {
  for (GLint i = 0; i < 4*4; i++) glContext.matrixForMode[glContext.matrixModeNr][i] = m[i];
  glUpdateMatrix();
}

GLvoid glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz) {
  glContext.normalX = nx;
  glContext.normalY = ny;
  glContext.normalZ = nz;
}

GLvoid glNormal3fv(const GLfloat *n) {
  glContext.normalX = n[0];
  glContext.normalY = n[1];
  glContext.normalZ = n[2];
}

GLvoid glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer) {
  glContext.normalSizeBuffer = 3;
  glContext.normalTypeBuffer = type;
  glContext.normalStrideBuffer = stride;
  glContext.normalPointerBuffer = pointer;
}

GLvoid glMateriali(GLenum face, GLenum pname, GLint param) {
  glMaterialf(face,pname,(GLfloat)param);
}

GLvoid glMaterialf(GLenum face, GLenum pname, GLfloat param) {
  GLfloat p[3];
  p[0] = param;
  p[1] = param;
  p[2] = param;
  glMaterialfv(face,pname,p);
}

GLvoid glMaterialfv(GLenum face, GLenum pname, const GLfloat *params) {
  GLint f0 = 0; GLint f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  for(GLint f = f0; f < f1; f++) {
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
        glContext.materialAlpha[f][1] = params[3];
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

GLvoid glMatrixMode(GLenum mode) {
  switch(mode) {
  case GL_PROJECTION: {glContext.matrixModeNr = GL_PROJECTION & 1;} break;
  case GL_MODELVIEW: {glContext.matrixModeNr = GL_MODELVIEW & 1;} break;
  case GL_TEXTURE: {glContext.matrixModeNr = 2;} break;
  }
}

GLvoid glMultMatrixf(const GLfloat *m) {
  glMatMulf2(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

GLvoid glMultMatrixd(const GLdouble *m) {
  glMatMul(glContext.matrixForMode[glContext.matrixModeNr],m,glContext.matrixForMode[glContext.matrixModeNr]);
  glUpdateMatrix();
}

GLvoid glPointSize(GLfloat size) {
  glContext.pointSize = size;
}

GLvoid glPolygonMode(GLenum face, GLenum mode) {
  GLint f0 = 0; GLint f1 = 1;
  if (face == GL_FRONT) {f0=0;f1=1;}
  if (face == GL_BACK) {f0=1;f1=2;}
  if (face == GL_FRONT_AND_BACK) {f0=0;f1=2;}
  for (GLint f = f0; f < f1; f++) {
    glContext.wireframe[f] = mode == GL_LINE ? GL_TRUE : GL_FALSE;
  }
}

GLvoid glPixelStorei(GLenum pname, GLint param) {
  __UNUSED(pname);
  __UNUSED(param);
}

GLvoid glPointParameteri(GLenum pname, GLint param) {
  glPointParameteriv(pname, &param);
}

GLvoid glPointParameteriv(GLenum pname, const GLint *param) {
  switch(pname) {
  case GL_POINT_SIZE_MIN: {glContext.pointSizeMin = (GLfloat)param[0];} break;
  case GL_POINT_SIZE_MAX: {glContext.pointSizeMax = (GLfloat)param[0];} break;
  case GL_POINT_DISTANCE_ATTENUATION: {glContext.pointAttenuation[0] = (GLfloat)param[0];glContext.pointAttenuation[1] = (GLfloat)param[1];glContext.pointAttenuation[2] = (GLfloat)param[2];} break;
  }
}

GLvoid glPointParameterf(GLenum pname, GLfloat param) {
  glPointParameterfv(pname, &param);
}

GLvoid glPointParameterfv(GLenum pname, const GLfloat *param) {
  switch(pname) {
  case GL_POINT_SIZE_MIN: {glContext.pointSizeMin = param[0];} break;
  case GL_POINT_SIZE_MAX: {glContext.pointSizeMax = param[0];} break;
  case GL_POINT_DISTANCE_ATTENUATION: {glContext.pointAttenuation[0] = param[0];glContext.pointAttenuation[1] = param[1];glContext.pointAttenuation[2] = param[2];} break;
  }
}

GLvoid glPolygonOffset(GLfloat factor, GLfloat units) {
  glContext.polygonOffsetFactor = factor;
  glContext.polygonOffsetUnits = units;
}

GLvoid glPopMatrix() {
  glMatrixStackPos--;
  memcpy(glContext.matrixForMode[glContext.matrixModeNr], glMatrixStack[glMatrixStackPos], 4*4*sizeof(GLdouble));
  glUpdateMatrix();
}

GLvoid glPushMatrix() {
  memcpy(glMatrixStack[glMatrixStackPos], glContext.matrixForMode[glContext.matrixModeNr], 4*4*sizeof(GLdouble));
  glMatrixStackPos++;
}

GLvoid glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels) {
  for (GLint yp = 0; yp < height; yp++) {
    GLint y2 = yp + glFrameBufferHeight - 1 - y;
    if (y2 < 0 || y2 >= glFrameBufferHeight) continue;
    for (GLint xp = 0; xp < width; xp++) {
      GLint x2 = xp + x;
      if (x2 < 0 || x2 >= glFrameBufferWidth) continue;
      const GLuint rgba = glFrameBuffer[x2+y2*glFrameBufferWidth];
      if (type == GL_UNSIGNED_BYTE || type == GL_BYTE) {
        switch(format) {
        case GL_RGBA: {((GLuint*)pixels)[xp+yp*width] = rgba;} break;
        case GL_BGRA: {((GLuint*)pixels)[xp+yp*width] = FLIPRB(rgba);} break;
        case GL_RED: {((GLubyte*)pixels)[xp+yp*width] = (GLubyte)(rgba & 0xff);} break;
        case GL_GREEN: {((GLubyte*)pixels)[xp+yp*width] = (GLubyte)((rgba>>8)& 0xff);} break;
        case GL_BLUE: {((GLubyte*)pixels)[xp+yp*width] = (GLubyte)((rgba>>16)& 0xff);} break;
        case GL_RGB: {GLubyte*c=&(((GLubyte*)pixels)[(xp+yp*width)*3]);c[0]= (GLubyte)(rgba&255);c[1]= (GLubyte)((rgba>>8)&255);c[2]= (GLubyte)((rgba>>16)&255);} break;
        case GL_BGR: {GLubyte*c=&(((GLubyte*)pixels)[(xp+yp*width)*3]);c[2]= (GLubyte)(rgba&255);c[1]= (GLubyte)((rgba>>8)&255);c[0]= (GLubyte)((rgba>>16)&255);} break;
        }
      }
    }
  }
}

GLvoid glRotate(GLdouble *matrix, GLfloat angleInRadians, GLfloat x, GLfloat y, GLfloat z) {
  GLdouble m[16], rotate[16];
  GLdouble OneMinusCosAngle, CosAngle, SinAngle;
  GLdouble A_OneMinusCosAngle, C_OneMinusCosAngle;
  CosAngle = cos((GLdouble)angleInRadians);
  OneMinusCosAngle = 1.f - CosAngle;
  SinAngle = sin((GLdouble)angleInRadians);
  A_OneMinusCosAngle = x*OneMinusCosAngle;
  C_OneMinusCosAngle = z*OneMinusCosAngle;
  for (GLint i = 0; i < 16; i++) m[i] = matrix[i];
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

GLvoid glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  GLfloat l = (GLfloat)sqrt((GLdouble)(x*x+y*y+z*z));
  if (l > 0) {
    x/=l;
    y/=l;
    z/=l;
  }
  glRotate(matrix,(GLfloat)(angle*2.0*PI/360.0),x,y,z);
  glUpdateMatrix();
}

GLvoid glScalef(GLfloat x, GLfloat y, GLfloat z) {
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  matrix[0] *= (GLdouble)x;
  matrix[4] *= (GLdouble)y;
  matrix[8] *= (GLdouble)z;

  matrix[1] *= (GLdouble)x;
  matrix[5] *= (GLdouble)y;
  matrix[9] *= (GLdouble)z;

  matrix[2] *= (GLdouble)x;
  matrix[6] *= (GLdouble)y;
  matrix[10] *= (GLdouble)z;

  matrix[3] *= (GLdouble)x;
  matrix[7] *= (GLdouble)y;
  matrix[11] *= (GLdouble)z;
  glUpdateMatrix();
}

GLvoid glScalefv(const GLfloat *v) {
  glScalef(v[0],v[1],v[2]);
}

GLvoid glScaled(GLdouble x, GLdouble y, GLdouble z) {
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  matrix[0] *= (GLdouble)x;
  matrix[4] *= (GLdouble)y;
  matrix[8] *= (GLdouble)z;

  matrix[1] *= (GLdouble)x;
  matrix[5] *= (GLdouble)y;
  matrix[9] *= (GLdouble)z;

  matrix[2] *= (GLdouble)x;
  matrix[6] *= (GLdouble)y;
  matrix[10] *= (GLdouble)z;

  matrix[3] *= (GLdouble)x;
  matrix[7] *= (GLdouble)y;
  matrix[11] *= (GLdouble)z;
  glUpdateMatrix();
}

GLvoid glScaledv(const GLdouble *v) {
  glScaled(v[0],v[1],v[2]);
}

GLvoid glScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
  glContext.scissorX0 = x;
  glContext.scissorY0 = glFrameBufferHeight-y-height;
  glContext.scissorX1 = x+width;
  glContext.scissorY1 = glFrameBufferHeight-y;
}

GLvoid glShadeModel(GLenum mode) {
  glContext.shadeMode = mode;
}

GLvoid glStencilFunc(GLenum func, GLint ref, GLuint mask) {
  glContext.stencilFunc = func;
  glContext.stencilFuncRef = ref;
  glContext.stencilFuncMask = mask;
}

GLvoid glStencilMask(GLuint mask) {
  glContext.stencilMask = mask;
}

GLvoid glStencilOp(GLenum fail, GLenum zfail, GLenum zpass) {
  glContext.stencilOpFail = fail;
  glContext.stencilOpZFail = zfail;
  glContext.stencilOpZPass = zpass;
}

GLvoid glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params) {
  __UNUSED(target);
  __UNUSED(pname);
  __UNUSED(params);
}

GLvoid glTexEnvi(GLenum target, GLenum pname, GLint param) {
  __UNUSED(target);
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  switch(pname) {
    case GL_TEXTURE_ENV_MODE: {
      t->texEnvMode = param;
    } break;
  }
}

GLvoid glTexGeni(GLenum coord, GLenum pname, GLint param) {
  __UNUSED(pname);
  switch(coord) {
  case GL_S: {glContext.texGenS = param;} break;
  case GL_T: {glContext.texGenT = param;} break;
  }
}

GLvoid glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels) {
  __UNUSED(target);
  __UNUSED(level);
  __UNUSED(internalformat);
  __UNUSED(border);
  __UNUSED(type);
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    __FREE(t->data);
    t->data = NULL;
  }
  t->data = (GLuint*)__MALLOC(sizeof(GLuint)*width*height);
  if (t->data == NULL) {
    glSetError(GL_OUT_OF_MEMORY);
    return;
  }
  t->width = width;
  t->height = height;
  GLint rIn=-1,gIn=-1,bIn=-1,aIn=-1;
  GLint formatStride = 4;
  GLint formatStride2 = 4;
  switch(format) {
  case GL_ALPHA:{aIn=0;formatStride = 1;} break;
  case GL_LUMINANCE:{rIn=0;gIn=0;bIn=0;formatStride = 1;} break;
  case GL_LUMINANCE_ALPHA:{rIn=0;gIn=0;bIn=0;aIn=1;formatStride = 2;} break;
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
  case GL_STENCIL_INDEX8:{rIn=0;gIn=0;bIn=0;aIn=0;formatStride = 1;formatStride2 = 1;} break;
  }
  GLint input[4];
  input[0]=0xff;
  input[1]=0xff;
  input[2]=0xff;
  input[3]=0xff;
  for (GLint y = 0; y < height; y++) {
    GLint i2 = y*width;
    GLint i = y*width;
    for (GLint x = 0; x < width; x++) {
      if (pixels != NULL) {
        input[0] = ((GLubyte*)pixels)[i2*formatStride];
        if (formatStride >= 2)
          input[1] = ((GLubyte*)pixels)[i2*formatStride+1];
        if (formatStride >= 3)
          input[2] = ((GLubyte*)pixels)[i2*formatStride+2];
        if (formatStride >= 4)
          input[3] = ((GLubyte*)pixels)[i2*formatStride+3];
      }
      GLuint rgba = format == GL_ALPHA ? 0x00ffffff : 0x00000000;
      if (rIn != -1) rgba |= input[rIn];
      if (gIn != -1) rgba |= input[gIn]<<8;
      if (bIn != -1) rgba |= input[bIn]<<16;
      if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
      if (formatStride2==1) ((GLubyte*)t->data)[i] = (GLubyte)(rgba & 255);
      if (formatStride2==4) t->data[i] = rgba;
      i++;
      i2++;
    }
  }
}

GLvoid glTexParameteri(GLenum target, GLenum pname, GLint param) {
  __UNUSED(target);
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
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

GLvoid glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels) {
  __UNUSED(target);
  __UNUSED(level);
  __UNUSED(type);
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data == NULL) return;
  GLint rIn=-1,gIn=-1,bIn=-1,aIn=-1;
  GLint formatStride = 4;
  GLint formatStride2 = 4;
  switch(format) {
  case GL_ALPHA:{aIn=0;formatStride = 1;} break;
  case GL_LUMINANCE:{rIn=0;gIn=0;bIn=0;formatStride = 1;} break;
  case GL_LUMINANCE_ALPHA:{rIn=0;gIn=0;bIn=0;aIn=1;formatStride = 2;} break;
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
  case GL_STENCIL_INDEX8:{rIn=0;gIn=0;bIn=0;aIn=0;formatStride = 1;formatStride2 = 1;} break;
  }
  GLint input[4];
  input[0]=0xff;
  input[1]=0xff;
  input[2]=0xff;
  input[3]=0xff;
  for (GLint y = 0; y < height; y++) {
    GLint i2 = y*width;
    for (GLint x = 0; x < width; x++) {
      if (pixels != NULL) {
        input[0] = ((GLubyte*)pixels)[i2*formatStride];
        if (formatStride >= 2)
          input[1] = ((GLubyte*)pixels)[i2*formatStride+1];
        if (formatStride >= 3)
          input[2] = ((GLubyte*)pixels)[i2*formatStride+2];
        if (formatStride >= 4)
          input[3] = ((GLubyte*)pixels)[i2*formatStride+3];
      }
      GLuint rgba = format == GL_ALPHA ? 0x00ffffff : 0x00000000;
      if (rIn != -1) rgba |= input[rIn];
      if (gIn != -1) rgba |= input[gIn]<<8;
      if (bIn != -1) rgba |= input[bIn]<<16;
      if (aIn != -1) rgba |= input[aIn]<<24; else rgba |=0xff000000;
      GLint x2 = x;
      GLint y2 = y;
      x2 += xoffset;
      y2 += yoffset;
      if (x2 >= 0 && y2 >= 0 && x2 < (GLint)t->width && y2 < (GLint)t->height) {
        if (formatStride2==1) ((GLubyte*)t->data)[x2+y2*t->width] = (GLubyte)(rgba & 255);
        if (formatStride2==4) t->data[x2+y2*t->width] = rgba;
      }
      i2++;
    }
  }
}


GLvoid glTranslate(GLdouble *matrix, GLfloat x, GLfloat y, GLfloat z) {
  matrix[12]=matrix[0]*(GLdouble)x+matrix[4]*(GLdouble)y+matrix[8]*(GLdouble)z+matrix[12];
  matrix[13]=matrix[1]*(GLdouble)x+matrix[5]*(GLdouble)y+matrix[9]*(GLdouble)z+matrix[13];
  matrix[14]=matrix[2]*(GLdouble)x+matrix[6]*(GLdouble)y+matrix[10]*(GLdouble)z+matrix[14];
  matrix[15]=matrix[3]*(GLdouble)x+matrix[7]*(GLdouble)y+matrix[11]*(GLdouble)z+matrix[15];
}

GLvoid glTranslatef(GLfloat x, GLfloat y, GLfloat z) {
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glTranslate(matrix,x,y,z);
  glUpdateMatrix();
}

GLvoid glTranslatefv(const GLfloat *v) {
  glTranslatef(v[0],v[1],v[2]);
}

GLvoid glTranslated(GLdouble x, GLdouble y, GLdouble z) {
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glTranslate(matrix,(GLfloat)x,(GLfloat)y,(GLfloat)z);
  glUpdateMatrix();
}

GLvoid glTranslatedv(const GLdouble *v) {
  glTranslated(v[0],v[1],v[2]);
}

GLvoid glVertex1f(GLfloat x) {
  glContext.vertexX = x;
  glEmitVertex();
}

GLvoid glVertex1fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glEmitVertex();
}

GLvoid glVertex2f(GLfloat x, GLfloat y) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glEmitVertex();
}

GLvoid glVertex2fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glEmitVertex();
}

GLvoid glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glEmitVertex();
}

GLvoid glVertex3fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glContext.vertexZ = v[2];
  glEmitVertex();
}

GLvoid glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glContext.vertexW = w;
  glEmitVertex();
}

GLvoid glVertex4fv(const GLfloat *v) {
  glContext.vertexX = v[0];
  glContext.vertexY = v[1];
  glContext.vertexZ = v[2];
  glContext.vertexW = v[3];
  glEmitVertex();
}

GLvoid glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  glContext.vertexSizeBuffer = size;
  glContext.vertexTypeBuffer = type;
  glContext.vertexStrideBuffer = stride;
  glContext.vertexPointerBuffer = pointer;
}

GLvoid glViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
  glContext.viewportX0 = x;
  glContext.viewportY0 = glFrameBufferHeight-y-height;
  glContext.viewportX1 = x+width;
  glContext.viewportY1 = glFrameBufferHeight-y;
}

GLvoid glNormal3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.normalX = x;
  glContext.normalY = y;
  glContext.normalZ = z;
}

GLvoid glNormal3dv(const GLdouble *p) {
  glContext.normalX = p[0];
  glContext.normalY = p[1];
  glContext.normalZ = p[2];
}

GLvoid glVertex1d(GLdouble x) {
  glContext.vertexX = x;
  glEmitVertex();
}

GLvoid glVertex1dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glEmitVertex();
}

GLvoid glVertex2d(GLdouble x, GLdouble y) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glEmitVertex();
}

GLvoid glVertex2dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glEmitVertex();
}

GLvoid glVertex3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glEmitVertex();
}

GLvoid glVertex3dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glContext.vertexZ = p[2];
  glEmitVertex();
}

GLvoid glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glContext.vertexX = x;
  glContext.vertexY = y;
  glContext.vertexZ = z;
  glContext.vertexW = w;
  glEmitVertex();
}

GLvoid glVertex4dv(const GLdouble *p) {
  glContext.vertexX = p[0];
  glContext.vertexY = p[1];
  glContext.vertexZ = p[2];
  glContext.vertexW = p[3];
  glEmitVertex();
}

GLvoid glTexCoord1f(GLfloat x) {
  glContext.textureX = x;
}

GLvoid glTexCoord2f(GLfloat x, GLfloat y) {
  glContext.textureX = x;
  glContext.textureY = y;
}

GLvoid glTexCoord3f(GLfloat x, GLfloat y, GLfloat z) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
}

GLvoid glTexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
  glContext.textureW = w;
}

GLvoid glTexCoord1d(GLdouble x) {
  glContext.textureX = x;
}

GLvoid glTexCoord2d(GLdouble x, GLdouble y) {
  glContext.textureX = x;
  glContext.textureY = y;
}

GLvoid glTexCoord3d(GLdouble x, GLdouble y, GLdouble z) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
}

GLvoid glTexCoord4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w) {
  glContext.textureX = x;
  glContext.textureY = y;
  glContext.textureZ = z;
  glContext.textureW = w;
}

GLvoid glTexCoord1dv(const GLdouble *v) {
  glTexCoord1d(v[0]);
}

GLvoid glTexCoord1fv(const GLfloat *v) {
  glTexCoord1f(v[0]);
}

GLvoid glTexCoord2dv(const GLdouble *v) {
  glTexCoord2d(v[0],v[1]);
}

GLvoid glTexCoord2fv(const GLfloat *v) {
  glTexCoord2f(v[0],v[1]);
}

GLvoid glTexCoord3dv(const GLdouble *v) {
  glTexCoord3d(v[0],v[1],v[2]);
}

GLvoid glTexCoord3fv(const GLfloat *v) {
  glTexCoord3f(v[0],v[1],v[2]);
}

GLvoid glTexCoord4dv(const GLdouble *v) {
  glTexCoord4d(v[0],v[1],v[2],v[3]);
}

GLvoid glTexCoord4fv(const GLfloat *v) {
  glTexCoord4f(v[0],v[1],v[2],v[3]);
}

GLvoid glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer) {
  GLint b = glContext.clientActiveTexture - GL_TEXTURE0;
  glContext.texSizeBuffer[b] = size;
  glContext.texTypeBuffer[b] = type;
  glContext.texStrideBuffer[b] = stride;
  glContext.texPointerBuffer[b] = pointer;
}

GLvoid glPushAttrib(GLbitfield mask) { // mask not supported and behaves different than glPushAttrib + only small Stack Depth, yet
  glContext.pushAttribBitsHere = mask; // maybe evaluate at glPopAttrib later
  memcpy(&glAttribStack[glAttribStackPos], &glContext, sizeof(_GLContext));
  glContext.pushAttribBitsHere = 0;
  glAttribStackPos++;
}

GLvoid glPopAttrib() {
  glAttribStackPos--;
  memcpy(&glContext,&glAttribStack[glAttribStackPos], sizeof(_GLContext));
  glUpdateMatrix();
}

GLvoid glTexParameterfv(GLenum target, GLenum pname, GLfloat *param) {
  __UNUSED(target);
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
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

GLvoid glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha) {
  glContext.blendColorRed = red;
  glContext.blendColorGreen = green;
  glContext.blendColorBlue = blue;
  glContext.blendColorAlpha = alpha;
}

GLvoid glBlendEquation(GLenum mode) {
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
GLvoid glMulMatVecf(GLfloat *dest, GLfloat *m, GLfloat *v) {
  GLfloat x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  GLfloat y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  GLfloat z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  GLfloat w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

GLvoid glMulMatVecd(GLdouble *dest, GLdouble *m, GLdouble *v) {
  GLdouble x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  GLdouble y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  GLdouble z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  GLdouble w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
}

GLvoid glMulMatMatf(GLfloat *dest, GLfloat *m1, GLfloat *m2) {
  glMatMulf(m1,m2,dest);
}

GLvoid glMulMatMatd(GLdouble *dest, GLdouble *m1, GLdouble *m2) {
  glMatMul(m1,m2,dest);
}

GLvoid glNormalize(GLdouble *v) {
  GLdouble l = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  if (l > 0) {
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
  }
}

GLvoid glPlaneNormal(GLdouble *normal, GLdouble *pvector1, GLdouble *pvector2) {
  normal[0]=(pvector1[1]*pvector2[2])-(pvector1[2]*pvector2[1]);
  normal[1]=(pvector1[2]*pvector2[0])-(pvector1[0]*pvector2[2]);
  normal[2]=(pvector1[0]*pvector2[1])-(pvector1[1]*pvector2[0]);
}

GLvoid __glFrustum(GLdouble *matrix, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble znear, GLdouble zfar) {
  GLdouble matrix2[16];
  GLdouble temp, temp2, temp3, temp4;
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
  glMulMatMatd(matrix, matrix, matrix2);
}

GLvoid __glPerspective(GLdouble *matrix, GLdouble fovyInDegrees, GLdouble aspectRatio, GLdouble znear, GLdouble zfar) {
  GLdouble ymax, xmax;
  ymax = znear * tan(fovyInDegrees*PI/360.f);
  xmax = ymax * aspectRatio;
  __glFrustum(matrix,-xmax,xmax,-ymax,ymax,znear,zfar);
}

GLvoid glLookAt(GLdouble *matrix, GLdouble *eyePosition3D, GLdouble *center3D, GLdouble *upVector3D) {
  GLdouble forward[3], side[3], up[3];
  GLdouble matrix2[16], resultMatrix[16];
  forward[0] = center3D[0]-eyePosition3D[0];
  forward[1] = center3D[1]-eyePosition3D[1];
  forward[2] = center3D[2]-eyePosition3D[2];
  glNormalize(forward);
  glPlaneNormal(side, forward, upVector3D);
  glNormalize(side);
  glPlaneNormal(up, side, forward);
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
  glTranslate(resultMatrix,(GLfloat)-eyePosition3D[0], (GLfloat)-eyePosition3D[1], (GLfloat)-eyePosition3D[2]);
  memcpy(matrix,resultMatrix,16*sizeof(GLdouble));
}

GLint glInvertMatrixf(const GLfloat *m, GLfloat *out)
{
   // https://archive.mesa3d.org/older-versions/9.x/9.0/
   // MesaLib-9.0.zip/Mesa-9.0\src\mesa\math\m_matrix.c
   // (MIT License) The default Mesa license is as follows: 
   // Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   GLfloat wtmp[4][8];
   GLfloat m0, m1, m2, m3, s;
   GLfloat *r0, *r1, *r2, *r3;

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
   if (fabs(r3[0]) > fabs(r2[0])) SWAP_ROWS_FLOAT(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0])) SWAP_ROWS_FLOAT(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0])) SWAP_ROWS_FLOAT(r1, r0);
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
   if (fabs(r3[1]) > fabs(r2[1])) SWAP_ROWS_FLOAT(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1])) SWAP_ROWS_FLOAT(r2, r1);
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
   if (fabs(r3[2]) > fabs(r2[2])) SWAP_ROWS_FLOAT(r3, r2);
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

GLint glInvertMatrix(const GLdouble *m, GLdouble *out)
{
   // https://archive.mesa3d.org/older-versions/9.x/9.0/
   // MesaLib-9.0.zip/Mesa-9.0\src\mesa\math\m_matrix.c
   // (MIT License) The default Mesa license is as follows: 
   // Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   GLdouble wtmp[4][8];
   GLdouble m0, m1, m2, m3, s;
   GLdouble *r0, *r1, *r2, *r3;

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
   if (fabs(r3[0]) > fabs(r2[0])) SWAP_ROWS_DOUBLE(r3, r2);
   if (fabs(r2[0]) > fabs(r1[0])) SWAP_ROWS_DOUBLE(r2, r1);
   if (fabs(r1[0]) > fabs(r0[0])) SWAP_ROWS_DOUBLE(r1, r0);
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
   if (fabs(r3[1]) > fabs(r2[1])) SWAP_ROWS_DOUBLE(r3, r2);
   if (fabs(r2[1]) > fabs(r1[1])) SWAP_ROWS_DOUBLE(r2, r1);
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
   if (fabs(r3[2]) > fabs(r2[2])) SWAP_ROWS_DOUBLE(r3, r2);
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

GLint gluInvertMatrixf(const GLfloat *m, GLfloat *dest) {
  return glInvertMatrixf(m,dest);
}

GLint gluInvertMatrix(const GLdouble *m, GLdouble *dest) {
  return glInvertMatrix(m,dest);
}

GLint glUnProjectf(GLfloat winx, GLfloat winy, GLfloat winz, GLfloat *modelview, GLfloat *projection, GLint *viewport, GLfloat *objectCoordinate)
{
  GLfloat m[16], A[16];
  GLfloat in[4], out[4];
  
  glMulMatMatf(A, projection, modelview);
  if(glInvertMatrixf(A, m) == 0)
    return 0;
  
  in[0]=(GLfloat)(((winx-(GLfloat)viewport[0])/(GLfloat)viewport[2]*2.0-1.0)/glContext.zoomX);
  in[1]=(GLfloat)(((winy-(GLfloat)viewport[1])/(GLfloat)viewport[3]*2.0-1.0)/glContext.zoomY);
  in[2]=(GLfloat)(2.0*winz-1.0);
  in[3]=(GLfloat)(1.0);
  
  glMulMatVecf(out, m, in);
  if(out[3] == 0.0)
    return 0;
  out[3]=(GLfloat)(1.0/out[3]);
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

GLint glUnProject(GLdouble winx, GLdouble winy, GLdouble winz, GLdouble *modelview, GLdouble *projection, GLint *viewport, GLdouble *objectCoordinate)
{
  GLdouble m[16], A[16];
  GLdouble in[4], out[4];
  
  glMulMatMatd(A, projection, modelview);
  if(glInvertMatrix(A, m) == 0)
    return 0;
  
  in[0]=((winx-(GLdouble)viewport[0])/(GLdouble)viewport[2]*2.0-1.0)/glContext.zoomX;
  in[1]=((winy-(GLdouble)viewport[1])/(GLdouble)viewport[3]*2.0-1.0)/glContext.zoomY;
  in[2]=2.0*winz-1.0;
  in[3]=1.0;
  
  glMulMatVecd(out, m, in);
  if(out[3] == 0.0)
    return 0;
  out[3]=1.0/out[3];
  objectCoordinate[0]=out[0]*out[3];
  objectCoordinate[1]=out[1]*out[3];
  objectCoordinate[2]=out[2]*out[3];
  return 1;
}

GLint glProjectfx(GLfloat objx, GLfloat objy, GLfloat objz, GLfloat *modelview, GLfloat *projection, GLint *viewport, GLfloat *windowCoordinate) {
  GLfloat in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  glMulMatVecf(in, modelview, in);
  glMulMatVecf(in, projection, in);
  if (in[3] == 0) return 0;
  in[0]/=in[3];
  in[1]/=in[3];
  in[2]/=in[3];
  in[3] = 1;
  in[0] *= (GLfloat)(viewport[2]*0.5*glContext.zoomX);
  in[1] *= (GLfloat)(viewport[3]*-0.5*glContext.zoomY);
  in[0] += (GLfloat)(viewport[0]+viewport[2]*0.5);
  in[1] += (GLfloat)(viewport[1]+viewport[3]*0.5);
  windowCoordinate[0] = in[0];
  windowCoordinate[1] = in[1];
  windowCoordinate[2] = in[2];
  return 1;
}

GLint glProjectx(GLdouble objx, GLdouble objy, GLdouble objz, GLdouble *modelview, GLdouble *projection, GLint *viewport, GLdouble *windowCoordinate) {
  GLdouble in[4];
  in[0] = objx;
  in[1] = objy;
  in[2] = objz;
  in[3] = 1;
  glMulMatVecd(in, modelview, in);
  glMulMatVecd(in, projection, in);
  if (in[3] == 0) return 0;
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

GLint glProjectf(GLfloat objx, GLfloat objy, GLfloat objz, GLfloat *modelview, GLfloat *projection, GLint *viewport, GLfloat *windowCoordinate) {
  GLfloat fTempo[8];
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
  
  if(fTempo[7] == 0.0)  //The w value
    return 0;
  
  fTempo[7]=(GLfloat)(1.0/fTempo[7]);
  fTempo[4]*=fTempo[7];
  fTempo[5]*=fTempo[7];
  fTempo[6]*=fTempo[7];
  
  windowCoordinate[0]=(GLfloat)((fTempo[4]*0.5*glContext.zoomX+0.5)*viewport[2]+viewport[0]);
  windowCoordinate[1]=(GLfloat)((fTempo[5]*0.5*glContext.zoomY+0.5)*viewport[3]+viewport[1]);
  windowCoordinate[2]=(GLfloat)((1.0+fTempo[6])*0.5);  //Between 0 and 1
  return 1;
}

GLint glProject(GLdouble objx, GLdouble objy, GLdouble objz, GLdouble *modelview, GLdouble *projection, GLint *viewport, GLdouble *windowCoordinate) {
  GLdouble fTempo[8];
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
  
  if(fTempo[7] == 0.0)  //The w value
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

GLvoid glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar) {
  const GLdouble r2 = (right-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  const GLdouble l2 = (left-(right-left)*0.5)*glContext.zoomX+(right-left)*0.5;
  const GLdouble t2 = (top-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;
  const GLdouble b2 = (bottom-(top-bottom)*0.5)*glContext.zoomY+(top-bottom)*0.5;

  right = (GLfloat)r2;
  left = (GLfloat)l2;
  top = (GLfloat)t2;
  bottom = (GLfloat)b2;
  
  GLdouble matrix2[16];
  GLdouble temp2, temp3, temp4;
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
  GLdouble *matrix = glContext.matrixForMode[glContext.matrixModeNr];
  glMulMatMatd(matrix, matrix, matrix2);
  glUpdateMatrix();
}

GLvoid glFrustum(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar) {
  __glFrustum(glContext.matrixForMode[glContext.matrixModeNr],left,right,bottom,top,znear,zfar);
  glUpdateMatrix();
}

GLvoid glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar) {
  glFrustum(left,right,bottom,top,znear,zfar);
}

GLvoid glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar) {
  glOrtho(left, right, bottom, top, znear, zfar);
}

GLvoid gluOrtho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top) {
  glOrtho(left, right, bottom, top, -1, 1);
}

GLvoid gluPerspective(GLfloat fov, GLfloat aspect, GLfloat nearPlane, GLfloat farPlane) {
  __glPerspective(glContext.matrixForMode[glContext.matrixModeNr],fov,aspect,nearPlane,farPlane);
  glUpdateMatrix();
}

GLvoid gluLookAt(GLfloat cx,GLfloat cy,GLfloat cz,GLfloat ox,GLfloat oy,GLfloat oz,GLfloat ux,GLfloat uy,GLfloat uz) {
  GLdouble eye[3];
  GLdouble center[3];
  GLdouble up[3];
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
  GLfloat d[3];
  const GLint r = glProjectfx(objX, objY, objZ, model, projection, view, d);
  *winX = (GLfloat)(d[0]+glPixelCenterX); // actually you need the pixelCenter?
  *winY = (GLfloat)(d[1]-glPixelCenterY); // actually you need the pixelCenter?
  *winZ = (GLfloat)(d[2]);
  return r;
}

GLint gluProjectf(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ) {
  GLfloat d[3];
  const GLint r = glProjectf(objX, objY, objZ, model, projection, view, d);
  *winX = (GLfloat)(d[0]+glPixelCenterX); // actually you need the pixelCenter?
  *winY = (GLfloat)(d[1]-glPixelCenterY); // actually you need the pixelCenter?
  *winZ = (GLfloat)(d[2]);
  return r;
}

GLint gluUnProjectf(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *objX, GLfloat *objY, GLfloat *objZ) {
  GLfloat d[3];
  winX -= (GLfloat)glPixelCenterX; // actually you need the pixelCenter?
  winY += (GLfloat)glPixelCenterY; // actually you need the pixelCenter?
  const GLint r = glUnProjectf(winX, winY, winZ, model, projection, view, d);
  *objX = d[0];
  *objY = d[1];
  *objZ = d[2];
  return r;
}

GLint gluProjectx(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  GLdouble d[3];
  const GLint r = glProjectx(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  GLdouble d[3];
  const GLint r = glProject(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *objX, GLdouble *objY, GLdouble *objZ) {
  GLdouble d[3];
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
GLint glMultiSampleBase = 0;
GLboolean glDontPaint = GL_FALSE;
typedef GLdouble GLraster; // floats are to imprecise (text rendering failed) and not faster (even slower) here
INLINE GLdouble getBary(GLdouble x, GLdouble y, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  GLdouble nx = v2->sy - v1->sy;
  GLdouble ny = -(v2->sx - v1->sx);
  GLdouble l = (v0->sx - v1->sx)*nx + (v0->sy - v1->sy)*ny;
  if (fabs(l) < 0.001) {l = 0.001;glDontPaint=GL_TRUE;}
  GLdouble p = (x - v1->sx)*nx + (y - v1->sy)*ny;
  return (p/l); // actually it must have been g/l and nx and ny been normalized
}

#define __POLYMINMAX__\
  GLint pminx,pminy,pmaxx,pmaxy;\
  pminx=(GLint)FLOOR(v0->sx);\
  pminy=(GLint)FLOOR(v0->sy);\
  pmaxx=(GLint)FLOOR(v0->sx);\
  pmaxy=(GLint)FLOOR(v0->sy);\
  if ((GLint)FLOOR(v1->sx) < pminx) pminx=(GLint)FLOOR(v1->sx);\
  if ((GLint)FLOOR(v1->sy) < pminy) pminy=(GLint)FLOOR(v1->sy);\
  if ((GLint)FLOOR(v1->sx) > pmaxx) pmaxx=(GLint)FLOOR(v1->sx);\
  if ((GLint)FLOOR(v1->sy) > pmaxy) pmaxy=(GLint)FLOOR(v1->sy);\
  if ((GLint)FLOOR(v2->sx) < pminx) pminx=(GLint)FLOOR(v2->sx);\
  if ((GLint)FLOOR(v2->sy) < pminy) pminy=(GLint)FLOOR(v2->sy);\
  if ((GLint)FLOOR(v2->sx) > pmaxx) pmaxx=(GLint)FLOOR(v2->sx);\
  if ((GLint)FLOOR(v2->sy) > pmaxy) pmaxy=(GLint)FLOOR(v2->sy);\
  pmaxx++;\
  pmaxy++;

#define __POLYCLIP__\
  GLboolean fullyClipped = GL_FALSE;\
  if (pminx < glClipRectX0) pminx=glClipRectX0;\
  if (pminy < glClipRectY0) pminy=glClipRectY0;\
  if (pmaxx <= glClipRectX0) {fullyClipped=GL_TRUE;pmaxx=glClipRectX0;}\
  if (pmaxy <= glClipRectY0) {fullyClipped=GL_TRUE;pmaxy=glClipRectY0;}\
  if (pminx >= glClipRectX1) {fullyClipped=GL_TRUE;pminx=glClipRectX1-1;}\
  if (pminy >= glClipRectY1) {fullyClipped=GL_TRUE;pminy=glClipRectY1-1;}\
  if (pmaxx > glClipRectX1) pmaxx=glClipRectX1;\
  if (pmaxy > glClipRectY1) pmaxy=glClipRectY1;

#define __PAINTPOLYQUAD_BEGINY__\
  GLint x,y;\
  GLraster bary0,bary1,bary2;\
  GLraster baryAdd0,baryAdd1,baryAdd2;\
  GLuint *pDest;\
  GLfloat *zDest;\
  GLubyte *sDest;\
  for (y = pminy;y < pmaxy;y++) {

#define __PAINTPOLYQUAD_INITBARYFORX__\
  bary0 = (GLraster)getBary(pminx,y,v0,v1,v2);\
  bary1 = (GLraster)getBary(pminx,y,v1,v2,v0);\
  bary2 = (GLraster)getBary(pminx,y,v2,v0,v1);\
  baryAdd0 = (GLraster)((getBary(pmaxx,y,v0,v1,v2)-bary0)/(pmaxx-pminx));\
  baryAdd1 = (GLraster)((getBary(pmaxx,y,v1,v2,v0)-bary1)/(pmaxx-pminx));\
  baryAdd2 = (GLraster)((getBary(pmaxx,y,v2,v0,v1)-bary2)/(pmaxx-pminx));

#define __PAINTPOLYQUAD_BEGINX__QUADREGION\
  GLint dminx = pminx;\
  GLint dmaxx = pmaxx;\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  sDest = &glStencilBuffer[y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  for (x = dminx;x < dmaxx;x++) {

#define __PAINTPOLYQUAD_BEGINX__TRI\
  GLint dminx = pminx;\
  GLint dmaxx = pmaxx;\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  sDest = &glStencilBuffer[y*glFrameBufferWidth+glMultiSampleBase*glFrameBufferWidth*glFrameBufferHeight];\
  GLint xp[3]; GLint xc=0;\
  if ((y >= vk0->sy) && (y < vk1->sy)) xp[xc++]=(GLint)((vk1->sx-vk0->sx)*(y-vk0->sy)/(vk1->sy-vk0->sy)+vk0->sx);\
  if ((y >= vk0->sy) && (y < vk2->sy)) xp[xc++]=(GLint)((vk2->sx-vk0->sx)*(y-vk0->sy)/(vk2->sy-vk0->sy)+vk0->sx);\
  if ((y >= vk1->sy) && (y < vk2->sy)) xp[xc++]=(GLint)((vk2->sx-vk1->sx)*(y-vk1->sy)/(vk2->sy-vk1->sy)+vk1->sx);\
  if (xc == 2) {\
    if (xp[1] < xp[0]) {GLint k = xp[0]; xp[0] = xp[1]; xp[1] = k;}\
    xp[0]-=2;\
    xp[1]+=2;\
    if (xp[1] < dmaxx) {dmaxx = xp[1];}\
    if (xp[0] > dminx) {\
      GLint addx = xp[0]-dminx;\
      dminx += addx;\
      pDest += addx;\
      zDest += addx;\
      bary0 += (GLraster)(addx * baryAdd0);\
      bary1 += (GLraster)(addx * baryAdd1);\
      bary2 += (GLraster)(addx * baryAdd2);\
    }\
  }\
  for (x = dminx;x < dmaxx;x++) {
// __TRI has some overcoverage on both ends (starty andor maybe endy, but for subpixel/subtexel maybe that's ok)

#define __PAINTPOLYQUAD_BEGINX__ __PAINTPOLYQUAD_BEGINX__TRI // __PAINTPOLYQUAD_BEGINX__QUADREGION (maybe used instead)

#define __BARY0__(__v__) ((__v__)*bary0)
#define __BARY1__(__v__) ((__v__)*bary1)
#define __BARY2__(__v__) ((__v__)*bary2)
#define __BARY0PADD__(__v__) ((__v__)*(bary0+xbaryAdd0))
#define __BARY1PADD__(__v__) ((__v__)*(bary1+xbaryAdd1))
#define __BARY2PADD__(__v__) ((__v__)*(bary2+xbaryAdd2))

INLINE GLboolean glCheckDepthFunction(GLfloat depthBufferValue, GLfloat newValue, GLint depthFunction) {
  switch(depthFunction) {
  case GL_NEVER: {return GL_FALSE;}
  case GL_LESS: {return (newValue < depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_EQUAL: {return (newValue == depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_LEQUAL: {return (newValue <= depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_GREATER: {return (newValue > depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_NOTEQUAL: {return (newValue != depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_GEQUAL: {return (newValue >= depthBufferValue) ? GL_TRUE : GL_FALSE;}
  case GL_ALWAYS: {return GL_TRUE;}
  }
  return GL_TRUE;
}


INLINE GLint textureWrap(GLint t, GLint siz, GLenum filterMode) {
  if ((GLuint)t < (GLuint)siz) return t;
  switch(filterMode) {
  case GL_MIRRORED_REPEAT: {
      t = t < 0 ? (siz*2-1-((-t)%(siz*2))) : (t%(siz*2));
      if (t < siz) return t;
      return siz - 1 - (t - siz);
    }
  case GL_CLAMP_TO_EDGE: {
      return t < 0 ? 0 : (t > siz-1?siz-1 : t);
    }
  case GL_CLAMP_TO_BORDER: {
      return t < 0 ? -1 : (t > siz-1?-1 : t);
    }
  }
  return t < 0 ? (siz-1-((-t)%siz)) : (t%siz); // GL_REPEAT
}

INLINE GLuint doBlend(GLuint dest, GLuint source, GLint sourceFunc, GLint destFunc, GLuint constantColor, GLint blendEquation) {

  if ((blendEquation == GL_FUNC_ADD) && (sourceFunc == GL_ONE) && (destFunc == GL_ONE)) {
    // purelay additive speedup for dest = 0xxx00000 and source = 0xffxxxxxx; seems to have no or almost no effect, but whatever
    if ( ((source & 0xff000000) == 0xff000000)  && ((dest & 0x00ffffff) == 0x00) ) {
      return source;
    }
  }

  GLint Sr = source & 255;
  GLint Sg = (source>>8) & 255;
  GLint Sb = (source>>16) & 255;
  GLint Sa = (source>>24) & 255;

  GLint Dr = dest & 255;
  GLint Dg = (dest>>8) & 255;
  GLint Db = (dest>>16) & 255;
  GLint Da = (dest>>24) & 255;

  GLint Cr = constantColor & 255;
  GLint Cg = (constantColor>>8) & 255;
  GLint Cb = (constantColor>>16) & 255;
  GLint Ca = (constantColor>>24) & 255;

  GLint sr=0,sg=0,sb=0,sa=0;
  GLint dr=0,dg=0,db=0,da=0;

  GLboolean fullS = GL_FALSE;
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
      fullS = GL_TRUE;
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
      if (Sa == 255) fullS = GL_TRUE;
    } break;
    case GL_ONE_MINUS_SRC_ALPHA: {
      sr = 255-Sa;
      sg = 255-Sa;
      sb = 255-Sa;
      sa = 255-Sa;
      if (Sa == 0) fullS = GL_TRUE;
    } break;
    case GL_DST_ALPHA: {
      sr = Da;
      sg = Da;
      sb = Da;
      sa = Da;
      if (Da == 255) fullS = GL_TRUE;
    } break;
    case GL_ONE_MINUS_DST_ALPHA: {
      sr = 255-Da;
      sg = 255-Da;
      sb = 255-Da;
      sa = 255-Da;
      if (Da == 0) fullS = GL_TRUE;
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
      if (Ca == 255) fullS = GL_TRUE;
    } break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: {
      sr = 255-Ca;
      sg = 255-Ca;
      sb = 255-Ca;
      sa = 255-Ca;
      if (Ca == 0) fullS = GL_TRUE;
    } break;
  }

  GLboolean fullD = GL_FALSE;
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
      fullD = GL_TRUE;
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
      if (Sa == 255) fullD = GL_TRUE;
    } break;
    case GL_ONE_MINUS_SRC_ALPHA: {
      dr = 255-Sa;
      dg = 255-Sa;
      db = 255-Sa;
      da = 255-Sa;
      if (Sa == 0) fullD = GL_TRUE;
    } break;
    case GL_DST_ALPHA: {
      dr = Da;
      dg = Da;
      db = Da;
      da = Da;
      if (Da == 255) fullD = GL_TRUE;
    } break;
    case GL_ONE_MINUS_DST_ALPHA: {
      dr = 255-Da;
      dg = 255-Da;
      db = 255-Da;
      da = 255-Da;
      if (Da == 0) fullD = GL_TRUE;
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
      if (Ca == 255) fullD = GL_TRUE;
    } break;
    case GL_ONE_MINUS_CONSTANT_ALPHA: {
      dr = 255-Ca;
      dg = 255-Ca;
      db = 255-Ca;
      da = 255-Ca;
      if (Ca == 0) fullD = GL_TRUE;
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

INLINE GLdouble __clamp__(GLdouble v, GLdouble a, GLdouble b) {
  return v < a ? a : (v > b ? b : v);
}

INLINE GLdouble __max__(GLdouble a, GLdouble b) {
  return a > b ? a : b;
}

GLdouble glPolygonOffset_(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2,GLdouble v0z,GLdouble v1z,GLdouble v2z) {
  const GLdouble ex = v0->sx - v2->sx;
  const GLdouble ey = v0->sy - v2->sy;
  const GLdouble fx = v1->sx - v2->sx;
  const GLdouble fy = v1->sy - v2->sy;
  const GLdouble det = ex * fy - ey * fx;
  GLdouble m = 0.0;
  if (det != 0) {
    // factor
    const GLdouble ez = v0z - v2z;
    const GLdouble fz = v1z - v2z;
    const GLdouble invDet = 1.0 / det;
    const GLdouble a = ey*fz - ez*fy;
    const GLdouble b = ez*fx - ex*fz;
    const GLdouble dzdx = fabs(a * invDet);
    const GLdouble dzdy = fabs(b * invDet);
    m = __max__(dzdx, dzdy);
  }
  // units
  GLfloat zf = __max__(__max__(fabs(v0z), fabs(v1z)), fabs(v2z));
  GLint *zi = (GLint*)&zf;
  GLuint *zui = (GLuint*)&zf;
  *zui &= 0xff << 23;
  *zi -= 23 << 23;
  if (*zi < 0) *zi = 0;

  return zf * context->polygonOffsetUnits + m * context->polygonOffsetFactor;
}


#define __CLIP_FAR_PLANE__ if (v0->sz > 1.0 && v1->sz > 1.0 && v2->sz > 1.0) return;

GLvoid glDrawTriangleAAPrecise(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2) {
  glMultiSampleBase = 0; glDrawTrianglePrecise(context,v0,v1,v2);
  if (glFrameBufferMultiSample > 1) {
    const GLdouble AAX = 0.49; const GLdouble AAY = 0.49;
    v0->sx += AAX; v1->sx += AAX; v2->sx += AAX; v0->sy += AAY; v1->sy += AAY; v2->sy += AAY;
    glMultiSampleBase = 1; glDrawTrianglePrecise(context,v0,v1,v2);
    v0->sx -= AAX; v1->sx -= AAX; v2->sx -= AAX; v0->sy -= AAY; v1->sy -= AAY; v2->sy -= AAY;
  }
  glMultiSampleBase = 0;
}

void glApplyTextureMatrix(GLfloat *tx,GLfloat *ty,GLfloat *tz,GLfloat *tw,GLdouble *m) {
  const GLdouble *cm = m;
  const GLdouble vx = (GLdouble)*tx;
  const GLdouble vy = (GLdouble)*ty;
  const GLdouble vz = (GLdouble)*tz;
  const GLdouble vw = (GLdouble)*tw;
  GLdouble x = vx * cm[0*4+0] + vy * cm[1*4+0] + vz * cm[2*4+0] + vw * cm[3*4+0];
  GLdouble y = vx * cm[0*4+1] + vy * cm[1*4+1] + vz * cm[2*4+1] + vw * cm[3*4+1];
  GLdouble z = vx * cm[0*4+2] + vy * cm[1*4+2] + vz * cm[2*4+2] + vw * cm[3*4+2];
  GLdouble w = vx * cm[0*4+3] + vy * cm[1*4+3] + vz * cm[2*4+3] + vw * cm[3*4+3];
  if (w != 0) {
    x /= w;
    y /= w;
    z /= w;
    w /= w;
  }
  *tx = (GLfloat)x;
  *ty = (GLfloat)y;
  *tz = (GLfloat)z;
  *tw = (GLfloat)w;
}


INLINE GLvoid glApplyStencileOp(GLint op, GLubyte *buffer, GLubyte stencilWriteMask, GLubyte stencilRef) {
  if (stencilWriteMask == 255) {
    switch(op) {
    case GL_KEEP: return;
    case GL_REPLACE: {*buffer = stencilRef;} return;
    case GL_INCR: {(*buffer)++;} return;
    case GL_DECR: {(*buffer)--;} return;
    case GL_INVERT: {(*buffer)^= 255;} return;
    }
    return;
  }
  switch(op) {
  case GL_KEEP: return;
  case GL_REPLACE: {*buffer = (GLubyte)(((*buffer)&(255^stencilWriteMask))|(stencilRef&stencilWriteMask));} return;
  case GL_INCR: {*buffer = (GLubyte)(((*buffer)&(255^stencilWriteMask))|(((*buffer)+1)&stencilWriteMask));} return;
  case GL_DECR: {*buffer = (GLubyte)(((*buffer)&(255^stencilWriteMask))|(((*buffer)-1)&stencilWriteMask));} return;
  case GL_INVERT: {*buffer = (GLubyte)(((*buffer)&(255^stencilWriteMask))|(((*buffer)^255)&stencilWriteMask));} return;
  }
}

GLvoid glDrawTrianglePrecise(_GLContext *context,glVertex *v0,glVertex *v1,glVertex *v2) {
  GLboolean forceWrapRepeat = GL_FALSE;

  if (isBackFaceCulled(context,v0,v1,v2)) return;

  __CLIP_FAR_PLANE__
  __POLYMINMAX__
  GLint glClipRectX0 = 0;
  GLint glClipRectY0 = 0;
  GLint glClipRectX1 = glFrameBufferWidth;
  GLint glClipRectY1 = glFrameBufferHeight;
  combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,context->viewportX0,context->viewportY0,context->viewportX1,context->viewportY1);
  if (glIsEnabled2(context,GL_SCISSOR_TEST)) {
    combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,context->scissorX0,context->scissorY0,context->scissorX1,context->scissorY1);
  }
  __POLYCLIP__
  if (fullyClipped) 
    return;

  GLboolean blending = glIsEnabled2(context,GL_BLEND);
  GLboolean maskRed = context->maskRed;
  GLboolean maskGreen = context->maskGreen;
  GLboolean maskBlue = context->maskBlue;
  GLboolean maskAlpha = context->maskAlpha;
  GLboolean notMasked = maskRed && maskGreen && maskBlue && maskAlpha;

  GLdouble v0w = 1.0/(v0->sw);
  GLdouble v1w = 1.0/(v1->sw);
  GLdouble v2w = 1.0/(v2->sw);
  GLdouble v0z = ((v0->sz)*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;
  GLdouble v1z = ((v1->sz)*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;
  GLdouble v2z = ((v2->sz)*0.5+0.5) * (context->depthRangeZFar-context->depthRangeZNear)+context->depthRangeZNear;

  if (glIsEnabled2(context,GL_POLYGON_OFFSET_FILL)) {
    const GLdouble zadd = glPolygonOffset_(context,v0,v1,v2,v0z,v1z,v2z);
    v0z += zadd;
    v1z += zadd;
    v2z += zadd;
  }

  glVertex *vk0 = v0;
  glVertex *vk1 = v1;
  glVertex *vk2 = v2;
  if (vk0->sy>vk1->sy) {glVertex *t; t = vk0; vk0 = vk1; vk1 = t;}
  if (vk0->sy>vk2->sy) {glVertex *t; t = vk0; vk0 = vk2; vk2 = t;}
  if (vk1->sy>vk2->sy) {glVertex *t; t = vk1; vk1 = vk2; vk2 = t;}

  glTexture *t = &glTextures[context->boundTextures[context->activeTexture]];
  GLboolean textured = (glIsEnabled2(context,GL_TEXTURE_2D) && t->data != NULL && t->name != 0) ? GL_TRUE : GL_FALSE;
  GLboolean filtering = ((t->magFilter != GL_NEAREST) && (t->magFilter != GL_NEAREST_MIPMAP_NEAREST) && (t->magFilter != GL_NEAREST_MIPMAP_LINEAR)) ? GL_TRUE : GL_FALSE;
  GLfloat tx0,ty0,tz0,tw0;
  GLfloat tx1,ty1,tz1,tw1;
  GLfloat tx2,ty2,tz2,tw2;
  GLuint *tdata0 = NULL;
  GLuint borderColor = 0xff000000;
  GLint twidth0;
  GLint theight0;
  GLint texEnvMode;
  // texture
  if (textured) {
    twidth0 = t->width;
    theight0 = t->height;
    tdata0 = t->data;
    texEnvMode = t->texEnvMode;

    GLdouble scx = glTexelCenterX;
    GLdouble scy = glTexelCenterY;
    if (!filtering) {scx += 0.5; scy += 0.5;} // rounding


    tx0 = (GLfloat)v0->textureX;
    ty0 = (GLfloat)v0->textureY;
    tz0 = (GLfloat)v0->textureZ;
    tw0 = (GLfloat)v0->textureW;
  

    tx1 = (GLfloat)v1->textureX;
    ty1 = (GLfloat)v1->textureY;
    tz1 = (GLfloat)v1->textureZ;
    tw1 = (GLfloat)v1->textureW;

    tx2 = (GLfloat)v2->textureX;
    ty2 = (GLfloat)v2->textureY;
    tz2 = (GLfloat)v2->textureZ;
    tw2 = (GLfloat)v2->textureW;

    if (glTextureMatrixIsSet[glContext.activeTexture]) {
      glApplyTextureMatrix(&tx0,&ty0,&tz0,&tw0,context->textureMatrix[glContext.activeTexture]);
      glApplyTextureMatrix(&tx1,&ty1,&tz1,&tw1,context->textureMatrix[glContext.activeTexture]);
      glApplyTextureMatrix(&tx2,&ty2,&tz2,&tw2,context->textureMatrix[glContext.activeTexture]);
    }

    if (glIsEnabled2(context,GL_TEXTURE_GEN_S) || glIsEnabled2(context,GL_TEXTURE_GEN_T)) {
      fixSphereMapUV(context, &tx0,&ty0,&tx1,&ty1);
      fixSphereMapUV(context, &tx0,&ty0,&tx2,&ty2);
      forceWrapRepeat = GL_TRUE;
    }

    tx0 = (GLfloat)((tx0*twidth0+scx)*v0w);
    ty0 = (GLfloat)((ty0*theight0+scy)*v0w);

    tx1 = (GLfloat)((tx1*twidth0+scx)*v1w);
    ty1 = (GLfloat)((ty1*theight0+scy)*v1w);

    tx2 = (GLfloat)((tx2*twidth0+scx)*v2w);
    ty2 = (GLfloat)((ty2*theight0+scy)*v2w);

    borderColor = ((GLint)FLOOR(glClampf(t->borderColorRed*255.f,0.f,255.f)));
    borderColor |= ((GLint)FLOOR(glClampf(t->borderColorGreen*255.f,0.f,255.f)))<<8;
    borderColor |= ((GLint)FLOOR(glClampf(t->borderColorBlue*255.f,0.f,255.f)))<<16;
    borderColor |= ((GLint)FLOOR(glClampf(t->borderColorAlpha*255.f,0.f,255.f)))<<24;

    if ((filtering) && ((t->minFilter == GL_NEAREST) || (t->minFilter == GL_NEAREST_MIPMAP_NEAREST) || (t->minFilter == GL_NEAREST_MIPMAP_LINEAR))) {
      const GLfloat tw = (GLfloat)twidth0;
      const GLfloat th = (GLfloat)theight0;
      const GLfloat lengtht0 = (GLfloat)sqrt((v1->textureX-v0->textureX)*(v1->textureX-v0->textureX)*tw*tw+(v1->textureY-v0->textureY)*(v1->textureY-v0->textureY)*th*th);
      const GLfloat lengtht1 = (GLfloat)sqrt((v2->textureX-v0->textureX)*(v2->textureX-v0->textureX)*tw*tw+(v2->textureY-v0->textureY)*(v2->textureY-v0->textureY)*th*th);
      const GLfloat lengtht2 = (GLfloat)sqrt((v2->textureX-v1->textureX)*(v2->textureX-v1->textureX)*tw*tw+(v2->textureY-v1->textureY)*(v2->textureY-v1->textureY)*th*th);
      const GLfloat lengths0 = (GLfloat)sqrt((v1->sx-v0->sx)*(v1->sx-v0->sx)+(v1->sy-v0->sy)*(v1->sy-v0->sy));
      const GLfloat lengths1 = (GLfloat)sqrt((v2->sx-v0->sx)*(v2->sx-v0->sx)+(v2->sy-v0->sy)*(v2->sy-v0->sy));
      const GLfloat lengths2 = (GLfloat)sqrt((v2->sx-v1->sx)*(v2->sx-v1->sx)+(v2->sy-v1->sy)*(v2->sy-v1->sy));
      if ((lengths0<lengtht0) && (lengths1<lengtht1) && (lengths2<lengtht2)) {
        filtering = GL_FALSE;
      }
    }
  }
  GLint blendFuncS = context->blendFuncSFactor;
  GLint blendFuncD = context->blendFuncDFactor;
  GLint blendEquation = context->blendEquation;
  GLuint constantColor = ((GLint)FLOOR(glClampf(context->blendColorRed*255.f,0.f,255.f)));
  constantColor |= ((GLint)FLOOR(glClampf(context->blendColorGreen*255.f,0.f,255.f)))<<8;
  constantColor |= ((GLint)FLOOR(glClampf(context->blendColorBlue*255.f,0.f,255.f)))<<16;
  constantColor |= ((GLint)FLOOR(glClampf(context->blendColorAlpha*255.f,0.f,255.f)))<<24;
  GLboolean writeDepth = context->depthMask;  
  GLint depthFunction = context->depthFunc;
  GLboolean depthTest = glIsEnabled2(context,GL_DEPTH_TEST);
  if (depthTest && (depthFunction == GL_ALWAYS)) depthTest=GL_FALSE;
  if (glDepthBuffer == NULL) {
    depthTest=GL_FALSE;
    writeDepth=GL_FALSE;
  }
  GLint alphaFunction = context->alphaFunc;
  GLint alphaRef = (GLint)FLOOR(context->alphaFuncRef*255.f);
  GLboolean alphaTest = glIsEnabled2(context,GL_ALPHA_TEST);

  // rgba
  GLdouble r0 = v0->colorRed;
  GLdouble g0 = v0->colorGreen;
  GLdouble b0 = v0->colorBlue;
  GLdouble a0 = v0->colorAlpha;

  GLdouble r1 = v1->colorRed;
  GLdouble g1 = v1->colorGreen;
  GLdouble b1 = v1->colorBlue;
  GLdouble a1 = v1->colorAlpha;

  GLdouble r2 = v2->colorRed;
  GLdouble g2 = v2->colorGreen;
  GLdouble b2 = v2->colorBlue;
  GLdouble a2 = v2->colorAlpha;

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

  GLint r,g,b,a,rf,gf,bf,af;
  GLboolean interpolateColor = GL_TRUE;
  GLboolean modulate = GL_TRUE;
  if ( (GLint)FLOOR(r0) == (GLint)FLOOR(r1) && (GLint)FLOOR(r1) == (GLint)FLOOR(r2)
    && (GLint)FLOOR(g0) == (GLint)FLOOR(g1) && (GLint)FLOOR(g1) == (GLint)FLOOR(g2)
    && (GLint)FLOOR(b0) == (GLint)FLOOR(b1) && (GLint)FLOOR(b1) == (GLint)FLOOR(b2)
    && (GLint)FLOOR(a0) == (GLint)FLOOR(a1) && (GLint)FLOOR(a1) == (GLint)FLOOR(a2)) {
    interpolateColor = GL_FALSE;
    rf = (GLint)r0;
    gf = (GLint)g0;
    bf = (GLint)b0;
    af = (GLint)a0;
    if ((rf == 255) && (gf == 255) && (bf == 255) && (af == 255)) 
      modulate = GL_FALSE;
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

  GLdouble sr0 = v0->additionalSpecularColorRed;
  GLdouble sg0 = v0->additionalSpecularColorGreen;
  GLdouble sb0 = v0->additionalSpecularColorBlue;

  GLdouble sr1 = v1->additionalSpecularColorRed;
  GLdouble sg1 = v1->additionalSpecularColorGreen;
  GLdouble sb1 = v1->additionalSpecularColorBlue;

  GLdouble sr2 = v2->additionalSpecularColorRed;
  GLdouble sg2 = v2->additionalSpecularColorGreen;
  GLdouble sb2 = v2->additionalSpecularColorBlue;


  const GLboolean fogging = glIsEnabled2(context,GL_FOG);
  GLdouble f0=0,f1=0,f2=0;
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
    GLdouble *cm = glContext.matrixForMode[GL_MODELVIEW & 1]; // for eyespace z
    GLdouble fz0 = v0->vertexX * cm[0*4+2] + v0->vertexY * cm[1*4+2] + v0->vertexZ * cm[2*4+2] + v0->vertexW * cm[3*4+2];
    GLdouble fw0 = v0->vertexX * cm[0*4+3] + v0->vertexY * cm[1*4+3] + v0->vertexZ * cm[2*4+3] + v0->vertexW * cm[3*4+3];
    GLdouble fz1 = v1->vertexX * cm[0*4+2] + v1->vertexY * cm[1*4+2] + v1->vertexZ * cm[2*4+2] + v1->vertexW * cm[3*4+2];
    GLdouble fw1 = v1->vertexX * cm[0*4+3] + v1->vertexY * cm[1*4+3] + v1->vertexZ * cm[2*4+3] + v1->vertexW * cm[3*4+3];
    GLdouble fz2 = v2->vertexX * cm[0*4+2] + v2->vertexY * cm[1*4+2] + v2->vertexZ * cm[2*4+2] + v2->vertexW * cm[3*4+2];
    GLdouble fw2 = v2->vertexX * cm[0*4+3] + v2->vertexY * cm[1*4+3] + v2->vertexZ * cm[2*4+3] + v2->vertexW * cm[3*4+3];
    if (fw0 != 0.0) fz0/=fw0;
    if (fw1 != 0.0) fz1/=fw1;
    if (fw2 != 0.0) fz2/=fw2;
    fz0 = fabs(fz0);
    fz1 = fabs(fz1);
    fz2 = fabs(fz2);
    if (context->fogMode == GL_LINEAR) {
      GLdouble fogLength = context->fogEnd-context->fogStart;
      if (fogLength != 0.0) {
        f0 = (context->fogEnd-fz0)/fogLength;
        f1 = (context->fogEnd-fz1)/fogLength;
        f2 = (context->fogEnd-fz2)/fogLength;
      }
    }
    if (context->fogMode == GL_EXP) {
      f0 = exp(-context->fogDensity*fz0);
      f1 = exp(-context->fogDensity*fz1);
      f2 = exp(-context->fogDensity*fz2);
    }
    if (context->fogMode == GL_EXP2) {
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
    if (f0 < 1.0/256.0 && f1 < 1.0/256.0 && f2 < 1.0/256.0) {textured = GL_FALSE; interpolateColor = GL_FALSE;}
    f0 *= v0w; // fadeout colors by this
    f1 *= v1w;
    f2 *= v2w;
  }


  GLint srf,sgf,sbf;

  const GLboolean separateSpecular = (context->separateSpecular || fogging) ? GL_TRUE : GL_FALSE;
  GLboolean interpolateSpecular = GL_TRUE;
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
  
    if ( (GLint)FLOOR(sr0) == (GLint)FLOOR(sr1) && (GLint)FLOOR(sr1) == (GLint)FLOOR(sr2)
      && (GLint)FLOOR(sg0) == (GLint)FLOOR(sg1) && (GLint)FLOOR(sg1) == (GLint)FLOOR(sg2)
      && (GLint)FLOOR(sb0) == (GLint)FLOOR(sb1) && (GLint)FLOOR(sb1) == (GLint)FLOOR(sb2)) {
      interpolateSpecular = GL_FALSE;
      srf = (GLint)sr0;
      sgf = (GLint)sg0;
      sbf = (GLint)sb0;
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

  GLraster iw = 0;
  const GLboolean wValue = (textured || interpolateColor) ? GL_TRUE : GL_FALSE;
  const GLboolean normalAlphaBlending = (blendFuncS == GL_SRC_ALPHA && blendFuncD == GL_ONE_MINUS_SRC_ALPHA && blendEquation == GL_FUNC_ADD) ? GL_TRUE : GL_FALSE;
  const GLboolean preMultipliedAlpha = (blendFuncS == GL_ONE && blendFuncD == GL_ONE_MINUS_SRC_ALPHA && blendEquation == GL_FUNC_ADD) ? GL_TRUE : GL_FALSE;
  const GLboolean normalAlphaBlendingOrPreMultipliedAlpha = (normalAlphaBlending || preMultipliedAlpha) ? GL_TRUE : GL_FALSE;
  const GLboolean useExplicitAlpha = (context->useExplicitAlpha) ? GL_TRUE : GL_FALSE;
  const GLboolean alphaSolelyOpacity = (blending && (normalAlphaBlending || (blendFuncS == GL_SRC_ALPHA && blendFuncD == GL_ONE && blendEquation == GL_FUNC_ADD))) ? GL_TRUE : GL_FALSE;
  const GLboolean colorSolelyPossible = ((texEnvMode == GL_MODULATE) || (texEnvMode == GL_ADD) || (texEnvMode == GL_REPLACE)) ? GL_TRUE : GL_FALSE;
  const GLboolean colorSolelyOpacity = (colorSolelyPossible && blending && (blendFuncS == GL_ONE && blendFuncD == GL_ONE && blendEquation == GL_FUNC_ADD)) ? GL_TRUE : GL_FALSE;

  GLint explicitAlphaValue = (GLint)FLOOR(context->explicitAlpha * 255.f);
  if (explicitAlphaValue < 0) explicitAlphaValue=0;
  if (explicitAlphaValue > 255) explicitAlphaValue=255;
  GLubyte eAlpha = (GLubyte)explicitAlphaValue;

  const GLuint wrapS = forceWrapRepeat ? GL_REPEAT : t->wrapS;
  const GLuint wrapT = forceWrapRepeat ? GL_REPEAT : t->wrapT;
  GLint tpx,tpy,tpxa,tpya;

  GLboolean useStencilBuffer = glIsEnabled2(context,GL_STENCIL_TEST) && (glStencilBuffer != NULL);
  if (context->stencilFunc == GL_ALWAYS &&
    context->stencilOpFail == GL_KEEP &&
    context->stencilOpZFail == GL_KEEP &&
    context->stencilOpZPass == GL_KEEP) useStencilBuffer = GL_FALSE;
  const GLboolean stencilDepthTest = (context->stencilOpZFail != GL_KEEP)||(context->stencilOpZPass != GL_KEEP);
  GLboolean stencilPassed = GL_TRUE;
  const GLint stencilFunc = (GLint)context->stencilFunc;
  const GLubyte stencilValueMask = (GLubyte)(context->stencilFuncMask & 255);
  const GLubyte stencilWriteMask = (GLubyte)(context->stencilMask & 255);
  const GLubyte stencilRef = (GLubyte)(context->stencilFuncRef & 255) & stencilValueMask;
  GLboolean depthTestPassed = GL_TRUE;
  const GLint stencilOpFail = (GLint)context->stencilOpFail;
  const GLint stencilOpZFail = (GLint)context->stencilOpZFail;
  const GLint stencilOpZPass = (GLint)context->stencilOpZPass;
  const GLboolean stencilFullReplace = (stencilWriteMask==255)&&(context->stencilFunc == GL_ALWAYS)&&(context->stencilOpFail == GL_REPLACE)&&(context->stencilOpZFail == GL_REPLACE)&&(context->stencilOpZPass == GL_REPLACE);

  glDontPaint = GL_FALSE;
  __PAINTPOLYQUAD_BEGINY__
  __PAINTPOLYQUAD_INITBARYFORX__
  if (glDontPaint) return;
#ifdef __FASTTEXTURING__
  GLboolean fastTexturing = glFastTexturing;
  GLint fastApprox = 0;
  GLint fastX = 0;
  GLraster xbaryAdd0 = (GLraster)(baryAdd0*glFastTextureSpanWidth);
  GLraster xbaryAdd1 = (GLraster)(baryAdd1*glFastTextureSpanWidth);
  GLraster xbaryAdd2 = (GLraster)(baryAdd2*glFastTextureSpanWidth);
#endif // __FASTTEXTURING__
  __PAINTPOLYQUAD_BEGINX__
      if (bary0 >= 0 && bary1 >= 0 && bary2 >= 0) {
        GLraster zp = (GLraster)(__BARY0__(v0z)+__BARY1__(v1z)+__BARY2__(v2z));
        if (useStencilBuffer) {
          GLubyte *stencilHere = &sDest[x];
          if (stencilFullReplace) {
            *stencilHere = stencilRef;
          } else { // stencilFullReplace
            switch(stencilFunc) {
            case GL_NEVER: {stencilPassed = GL_FALSE;} break;
            case GL_LESS: {stencilPassed = stencilRef < ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_EQUAL: {stencilPassed = stencilRef == ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_LEQUAL: {stencilPassed = stencilRef <= ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_GREATER: {stencilPassed = stencilRef > ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_NOTEQUAL: {stencilPassed = stencilRef != ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_GEQUAL: {stencilPassed = stencilRef >= ((*stencilHere) & stencilValueMask) ? GL_TRUE : GL_FALSE;} break;
            case GL_ALWAYS: {stencilPassed = GL_TRUE;} break;
            }
            if (stencilPassed) {
              if (stencilDepthTest) {
                depthTestPassed = (!depthTest) || glCheckDepthFunction(*zDest,(GLfloat)zp,depthFunction);
                if (depthTestPassed)
                  glApplyStencileOp(stencilOpZPass,stencilHere,stencilWriteMask,stencilRef);
                else
                  glApplyStencileOp(stencilOpZFail,stencilHere,stencilWriteMask,stencilRef);
              }
            } else { // stencilPassed
              glApplyStencileOp(stencilOpFail,stencilHere,stencilWriteMask,stencilRef);
              pDest++;
              zDest++;
              bary0+=baryAdd0;
              bary1+=baryAdd1;
              bary2+=baryAdd2;
              continue; // !ATTENTION!
            }
          }
        }
        if ((!depthTest) || glCheckDepthFunction(*zDest,(GLfloat)zp,depthFunction)) {
          if (wValue) iw = (GLraster)(1.0/(__BARY0__(v0w)+__BARY1__(v1w)+__BARY2__(v2w)));
          GLuint rgba;
          GLboolean writePixel = GL_TRUE;
           GLboolean writePixel2 = GL_TRUE;
          if (textured) {
            if (filtering) {
#ifdef __FASTTEXTURING__
              if (fastTexturing) {
                if (fastApprox == 0 || fastX != x) {
                  fastApprox=glFastTextureSpanWidth;
                  GLdouble iw2=iw*0x10000;
                  tpx = (GLint)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw2);
                  tpy = (GLint)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw2);
                  if (wValue) iw2 = (1.0/(__BARY0PADD__(v0w)+__BARY1PADD__(v1w)+__BARY2PADD__(v2w)))*0x10000;
                  GLint k = glFastTextureSpanWidth;
                  tpxa = ((GLint)FLOOR((__BARY0PADD__(tx0)+__BARY1PADD__(tx1)+__BARY2PADD__(tx2))*iw2)-tpx)/k;
                  tpya = ((GLint)FLOOR((__BARY0PADD__(ty0)+__BARY1PADD__(ty1)+__BARY2PADD__(ty2))*iw2)-tpy)/k;
                  if (k+x >= dmaxx) fastTexturing = GL_FALSE; // this is maybe not the triangle edge
                }
                fastX=x+1;
                const GLint kx = tpx>>16;
                const GLint ky = tpy>>16;
                GLint tix0 = textureWrap(kx, twidth0, wrapS);
                GLint tiy0 = textureWrap(ky, theight0, wrapT);
                const GLint tix1 = textureWrap(kx+1, twidth0, wrapS);
                GLint tiy1 = textureWrap(ky+1, theight0, wrapT);
                const GLint txf = (tpx>>8)&255;
                const GLint tyf = (tpy>>8)&255;
                const GLint p1v = ((256-txf)*(256-tyf))>>8; 
                const GLint p2v = ((txf)*(256-tyf))>>8; 
                const GLint p3v = ((txf)*(tyf))>>8; 
                const GLint p4v = ((256-txf)*(tyf))>>8;
                tiy0 *= twidth0;
                tiy1 *= twidth0;
                const GLuint rgba00 = (tix0|tiy0) >= 0 ? tdata0[tix0+tiy0] : borderColor;
                const GLuint rgba10 = (tix1|tiy0) >= 0 ? tdata0[tix1+tiy0] : borderColor;
                const GLuint rgba11 = (tix1|tiy1) >= 0 ? tdata0[tix1+tiy1] : borderColor;
                const GLuint rgba01 = (tix0|tiy1) >= 0 ? tdata0[tix0+tiy1] : borderColor;
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
                const GLint tx = (GLint)((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*256.0*iw);
                const GLint ty = (GLint)((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*256.0*iw);
                const GLint kx = tx>>8;
                const GLint ky = ty>>8;
                const GLint tix0 = textureWrap(kx, twidth0, wrapS);
                const GLint tix1 = textureWrap(kx+1, twidth0, wrapS);
                GLint tiy0 = textureWrap(ky, theight0, wrapT);
                GLint tiy1 = textureWrap(ky+1, theight0, wrapT);
                GLint txf = tx & 255;
                GLint tyf = ty & 255;
                const GLint p1v = ((256-txf)*(256-tyf))>>8; 
                const GLint p2v = ((txf)*(256-tyf))>>8; 
                const GLint p3v = ((txf)*(tyf))>>8; 
                const GLint p4v = ((256-txf)*(tyf))>>8;
                tiy0 *= twidth0;
                tiy1 *= twidth0;
                const GLuint rgba00 = (tix0|tiy0) >= 0 ? tdata0[tix0+tiy0] : borderColor;
                const GLuint rgba10 = (tix1|tiy0) >= 0 ? tdata0[tix1+tiy0] : borderColor;
                const GLuint rgba11 = (tix1|tiy1) >= 0 ? tdata0[tix1+tiy1] : borderColor;
                const GLuint rgba01 = (tix0|tiy1) >= 0 ? tdata0[tix0+tiy1] : borderColor;
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
                if (fastApprox == 0 || fastX != x) {
                  fastApprox=glFastTextureSpanWidth;
                  GLdouble iw2=iw*0x10000;
                  tpx = (GLint)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw2);
                  tpy = (GLint)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw2);
                  if (wValue) iw2 = (1.0/(__BARY0PADD__(v0w)+__BARY1PADD__(v1w)+__BARY2PADD__(v2w)))*0x10000;
                  GLint k = glFastTextureSpanWidth;
                  tpxa = ((GLint)FLOOR((__BARY0PADD__(tx0)+__BARY1PADD__(tx1)+__BARY2PADD__(tx2))*iw2)-tpx)/k;
                  tpya = ((GLint)FLOOR((__BARY0PADD__(ty0)+__BARY1PADD__(ty1)+__BARY2PADD__(ty2))*iw2)-tpy)/k;
                  if (k+x >= dmaxx) fastTexturing = GL_FALSE; // this is maybe not the triangle edge
                }
                fastX=x+1;
                GLint tix0 = textureWrap(tpx>>16, twidth0, wrapS);
                GLint tiy0 = textureWrap(tpy>>16, theight0, wrapT);
                if ((GLint)(tix0|tiy0) >= 0) {
                  rgba = tdata0[tix0+tiy0*twidth0];
                } else {
                  rgba = borderColor;
                }
                tpx+=tpxa;
                tpy+=tpya;
                fastApprox--;
              } else {
#endif // __FASTTEXTURING__
                GLint tix0 = textureWrap((GLint)FLOOR((__BARY0__(tx0)+__BARY1__(tx1)+__BARY2__(tx2))*iw), twidth0, wrapS);
                GLint tiy0 = textureWrap((GLint)FLOOR((__BARY0__(ty0)+__BARY1__(ty1)+__BARY2__(ty2))*iw), theight0, wrapT);
                if ((GLint)(tix0|tiy0) >= 0) {
                  rgba = tdata0[tix0+tiy0*twidth0];
                } else {
                  rgba = borderColor;
                }
#ifdef __FASTTEXTURING__
              }
#endif // __FASTTEXTURING__
            }
            if (alphaSolelyOpacity) writePixel2 = ((rgba&0xff000000) != 0) ? GL_TRUE : GL_FALSE;
            if (colorSolelyOpacity) writePixel2 = (rgba != 0x00000000) ? GL_TRUE : GL_FALSE; // alpha channel has to be 0,too for optimized additive transparency
          }
          
          if (writePixel2) {
            if (interpolateColor) {
              r = (GLint)((__BARY0__(r0)+__BARY1__(r1)+__BARY2__(r2))*iw);
              g = (GLint)((__BARY0__(g0)+__BARY1__(g1)+__BARY2__(g2))*iw);
              b = (GLint)((__BARY0__(b0)+__BARY1__(b1)+__BARY2__(b2))*iw);
              a = (GLint)((__BARY0__(a0)+__BARY1__(a1)+__BARY2__(a2))*iw);
            } else {
              r = rf;
              g = gf;
              b = bf;
              a = af;
            }
  
            if (textured) {
              if (texEnvMode == GL_MODULATE) {
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
                    const GLint as = ((rgba >> 24)&255);
                    r = r*(255-as)+(rgba & 255)*as;
                    g = g*(255-as)+((rgba>>8) & 255)*as;
                    b = b*(255-as)+((rgba>>16) & 255)*as;
                    r DIVE255;
                    g DIVE255;
                    b DIVE255;
                  } break;
                  case GL_ADD: {
                    const GLint as = ((rgba >> 24)&255);
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
              case GL_NEVER: {writePixel = GL_FALSE;} break;
              case GL_LESS: {writePixel = (a < alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_EQUAL: {writePixel = (a == alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_LEQUAL: {writePixel = (a <= alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_GREATER: {writePixel = ( a > alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_NOTEQUAL: {writePixel = (a != alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_GEQUAL: {writePixel = (a >= alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_ALWAYS: {writePixel = GL_TRUE;} break;
              }
            }
  
            if (separateSpecular) {
              if (fogging) {
                const GLdouble f = (__BARY0__(f0)+__BARY1__(f1)+__BARY2__(f2))*iw;
                r = (GLint)(r*f);
                g = (GLint)(g*f);
                b = (GLint)(b*f);
              }
              if (interpolateSpecular) {
                r += (GLint)((__BARY0__(sr0)+__BARY1__(sr1)+__BARY2__(sr2))*iw);
                g += (GLint)((__BARY0__(sg0)+__BARY1__(sg1)+__BARY2__(sg2))*iw);
                b += (GLint)((__BARY0__(sb0)+__BARY1__(sb1)+__BARY2__(sb2))*iw);
              } else {
                r += srf;
                g += sgf;
                b += sbf;
              }
              if (r > 255) r = 255;
              if (g > 255) g = 255;
              if (b > 255) b = 255;
            }
          } else {
            if (alphaTest) {
              switch(alphaFunction) {
              case GL_NEVER: {writePixel = GL_FALSE;} break;
              case GL_LESS: {writePixel = (0 < alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_EQUAL: {writePixel = (0 == alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_LEQUAL: {writePixel = (0 <= alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_GREATER: {writePixel = (0 > alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_NOTEQUAL: {writePixel = (0 != alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_GEQUAL: {writePixel = (0 >= alphaRef) ? GL_TRUE : GL_FALSE;} break;
              case GL_ALWAYS: {writePixel = GL_TRUE;} break;
              }
            }
          }

          if (writePixel) {
            if (writeDepth) 
              *zDest=(GLfloat)zp;
            if (!blending)
              if (notMasked) {
                *pDest=r|(g<<8)|(b<<16)|(a<<24);
              } else {
                GLubyte *pDest2 = (GLubyte *)pDest;
                if (maskRed) pDest2[0] = (GLubyte)r;
                if (maskGreen) pDest2[1] = (GLubyte)g;
                if (maskBlue) pDest2[2] = (GLubyte)b;
                if (maskAlpha) pDest2[3] = (GLubyte)a;
              }
            else {
              if (writePixel2) {
                if (notMasked) {
                  if (normalAlphaBlendingOrPreMultipliedAlpha) {
                    GLuint a8 = (a<<16)/255;
                    GLubyte *c = (GLubyte*)pDest;
                    if (preMultipliedAlpha) {
                      a8 = 0x10000-a8;
                      r += (c[0]*a8)>>16;
                      g += (c[1]*a8)>>16;
                      b += (c[2]*a8)>>16;
                      a += (c[3]*a8)>>16;
                      if (r > 255) r = 255;
                      if (g > 255) g = 255;
                      if (b > 255) b = 255;
                      if (a > 255) a = 255;
                      *pDest=r|(g<<8)|(b<<16)|(a<<24);
                    } else {
                      c[0] = (GLubyte)(c[0] + (((r-c[0])*a8)>>16));
                      c[1] = (GLubyte)(c[1] + (((g-c[1])*a8)>>16));
                      c[2] = (GLubyte)(c[2] + (((b-c[2])*a8)>>16));
                      c[3] = (GLubyte)(c[3] + (((a-c[3])*a8)>>16));
                    }
                  } else {
                    *pDest=doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                  }
                } else {
                  GLuint k = doBlend(*pDest,r|(g<<8)|(b<<16)|(a<<24),blendFuncS,blendFuncD,constantColor,blendEquation);
                  GLubyte *pDest2 = (GLubyte *)pDest;
                  GLubyte *k2 = (GLubyte *)&k;
                  if (maskRed) pDest2[0] = k2[0];
                  if (maskGreen) pDest2[1] = k2[1];
                  if (maskBlue) pDest2[2] = k2[2];
                  if (maskAlpha) pDest2[3] = k2[3];
                }
              }
            }
            if (useExplicitAlpha) {
              ((GLubyte *)pDest)[3] = eAlpha;
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

GLvoid glDrawTriangleNone(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2) {
  __UNUSED(context);
  __UNUSED(v0);
  __UNUSED(v1);
  __UNUSED(v2);
}

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

GLboolean glDirectBlit = GL_FALSE;
GLboolean glHiColor = GL_FALSE;
GLboolean glOtherFrame = GL_FALSE;
GLfloat glPixelLastSX = 0;
GLfloat glPixelLastSY = 0;
GLboolean glPixelLastValid = GL_FALSE;

GLvoid glSetMonitorAspectRatio(GLfloat aspect) {
  glContext.zoomX = 1.0/(aspect*glFrameBufferHeight0/glFrameBufferWidth0);
  glContext.zoomY = 1.0;
}

GLdouble glGetMonitorAspectRatio() {
  return 1.0/glContext.zoomX/((GLdouble)glFrameBufferHeight0/glFrameBufferWidth0);
}

GLvoid glZoomX(GLfloat zoom) {
  glContext.zoomX = zoom;
}

GLvoid glZoomY(GLfloat zoom) {
  glContext.zoomY = zoom;
}

GLfloat glGetZoomX() {
  return (GLfloat)glContext.zoomX;
}

GLfloat glGetZoomY() {
  return (GLfloat)glContext.zoomY;
}

GLvoid glExplicitAlpha(GLboolean useExplicitAlpha, GLfloat alpha) {
  glContext.explicitAlpha = alpha;
  glContext.useExplicitAlpha = useExplicitAlpha;
}

GLboolean glPixel(GLboolean newXYZ, GLfloat xp, GLfloat yp, GLfloat zp, GLint x, GLint y, GLuint color) {
  if (newXYZ) {
    glVertex v;
    v.vertexX = xp;
    v.vertexY = yp;
    v.vertexZ = zp;
    v.vertexW = 1;
    glPixelLastValid = (glTransformVertex(&glContext, &v, GL_TRUE) == 0);
    glPixelLastSX = (GLfloat)v.sx;
    glPixelLastSY = (GLfloat)v.sy;
  }
  if (glPixelLastValid)
  {
    GLint glClipRectX0 = 0;
    GLint glClipRectY0 = 0;
    GLint glClipRectX1 = glFrameBufferWidth;
    GLint glClipRectY1 = glFrameBufferHeight;
    combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,glContext.viewportX0,glContext.viewportY0,glContext.viewportX1,glContext.viewportY1);
    if (glIsEnabled(GL_SCISSOR_TEST)) {
      combineIntoWindow(&glClipRectX0,&glClipRectY0,&glClipRectX1,&glClipRectY1,glContext.scissorX0,glContext.scissorY0,glContext.scissorX1,glContext.scissorY1);
    }
    GLint px = (GLint)(glPixelLastSX+x);
    GLint py = (GLint)(glPixelLastSY+y);
    if (px < glClipRectX0) return GL_FALSE; 
    if (py < glClipRectY0) return GL_FALSE;
    if (px >= glClipRectX1) return GL_FALSE;
    if (py >= glClipRectY1) return GL_FALSE;
    glFrameBuffer[px+py*glFrameBufferWidth] = color;
    return GL_TRUE;
  }
  return GL_FALSE;
}

GLvoid glTexturePointer(GLint width, GLint height, GLuint *textureData) {
  if (glContext.boundTextures[glContext.activeTexture] == 0) 
    return;
  glTexture *t = &glTextures[glContext.boundTextures[glContext.activeTexture]];
  if (t->data != NULL)  {
    __FREE(t->data);
    t->data = NULL;
  }
  t->data = textureData;
  t->width = width;
  t->height = height;
}

GLvoid glConfigureAntiAlias() {
  glFrameBufferMultiSample = 2;
  glSetTriangleDrawer(glDrawTriangleAAPrecise);
}

GLvoid glSetRenderTarget(GLuint *frameBufferOrNULL, GLfloat *depthBuffer, GLuint width, GLuint height) {
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

GLvoid glCleanSetup(GLuint width, GLuint height, GLuint *frameBuffer, GLfloat *depthBuffer, GLubyte *stencilBuffer) {
  constructGL();
  glDirectBlit = GL_FALSE;
  glHiColor = GL_FALSE;
  glOtherFrame = GL_FALSE;
  glFrameBufferDedicated = NULL;
  glFrameBufferWidth = width;
  glFrameBufferWidth0 = width;
  glFrameBufferHeight = height;
  glFrameBufferHeight0 = height;
  glFrameBuffer = frameBuffer;
  glFrameBuffer0 = frameBuffer;
  glDepthBuffer = depthBuffer;
  glDepthBuffer0 = depthBuffer;
  glStencilBuffer = stencilBuffer;
  glStencilBuffer0 = stencilBuffer;
  memset(glFrameBuffer,0,width*height*glFrameBufferBytesPerPixel*glFrameBufferMultiSample);
  if (glDepthBuffer != NULL) memset(glDepthBuffer,0,width*height*glFrameBufferMultiSample*sizeof(GLfloat));
  if (glStencilBuffer != NULL) memset(glStencilBuffer,0,width*height*glFrameBufferMultiSample*sizeof(GLubyte));
  glViewport(0,0,width,height);
  glSetTime(0);
}

GLboolean glDirect(GLuint *frameBuffer, GLfloat *depthBuffer, GLubyte *stencilBuffer, GLuint width, GLuint height) {
  glCleanSetup(width, height, frameBuffer, depthBuffer, stencilBuffer);
  glDirectBlit = GL_TRUE;
  return GL_TRUE;
}
// --------------------------------------

// --------------------------------------
#ifdef __GLDISABLEDOSFUNCTIONS__
// --------------------------------------
GLdouble glSeconds() {return 0;}
GLvoid glSetTime(GLdouble seconds) {;}
GLvoid glSpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {;}
GLvoid glSetupMouse() {;}
GLvoid glNextMouseDelta(GLdouble *mouseX, GLdouble *mouseY) {;}
GLushort glMouseButtons() {return 0;}
GLvoid glSetMousePos(GLint x, GLint y) {;}
GLushort glNextKey() {return 0;}
GLvoid glDone() {;}
GLvoid glRefresh() {;}
GLboolean glVGA() {return GL_FALSE;}
GLboolean glVesa(GLint xRes,GLint yRes, GLint bPP) {return GL_FALSE;}
// --------------------------------------
#else // __GLDISABLEDOSFUNCTIONS__
// --------------------------------------
GLvoid glSetupMouse();
GLushort glGetBiosGraphicsMode();
GLubyte glHiRedTab[256];
GLint glGraphicsModeToRestore=-1;
GLint glKey = 0;
GLint glLastMouseX = 0;
GLint glLastMouseY = 0;
GLint glLastMouseB = 0;
GLint glMouseX = 0;
GLint glMouseY = 0;
GLint glMouseB = 0;
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
GLboolean glVesaBGRA = GL_TRUE;
GLint glBits = 32;
GLushort glHiColorTableR[256];
GLushort glHiColorTableG[256];
GLushort glHiColorTableB[256];
typedef GLubyte uint8_t;
typedef GLushort uint16_t;
typedef GLuint uint32_t;
#pragma pack(push)
#pragma pack(1)
typedef struct glVbeInfoBlock {
  uint8_t signature[4];
  uint16_t version;
  uint8_t *oemString;
  uint32_t capabilities;
  uint16_t *videoModes;
  uint16_t totalMemory;
  uint16_t softwareRev;
  uint8_t *vendor;
  uint8_t *productName;
  uint8_t *productRev;
  uint8_t reserved1[222];
  uint8_t oemData[256];
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

void glBuildHiColorTables(glVbeModeInfo *mode) {
  const GLint redShift = 8 - mode->redMask;
  const GLint greenShift = 8 - mode->greenMask;
  const GLint blueShift = 8 - mode->blueMask;
  for (GLint i = 0; i < 256; i++) {
    glHiColorTableR[i] = (GLushort)((i>>redShift)<<mode->redPosition);
    glHiColorTableG[i] = (GLushort)((i>>greenShift)<<mode->greenPosition);
    glHiColorTableB[i] = (GLushort)((i>>blueShift)<<mode->bluePosition);
  }
}

// --------------------------------------
// ---- DPMI for VESA ---
// --------------------------------------

// --------------------------------------
#ifdef __WATCOMC__
// --------------------------------------

#define DJGPPVGAMEMON
#define DJGPPVGAMEMOFF
#define VGARAMBASE (0xa0000)
#define IN(reg) rmregs.e##reg = in->##reg
#define OUT(reg) out->##reg = (uint16_t)rmregs.e##reg

GLint glVESABUF_sel=0x00;
GLint glVESABUF_rseg=0x00;
GLint glVESABUF_len=1024;

typedef struct {
  uint16_t di;
  uint16_t si;
  uint16_t bp;
  uint16_t reserved;
  uint16_t bx;
  uint16_t dx;
  uint16_t cx;
  uint16_t ax;
  uint16_t flags;
  uint16_t es,ds,fs,gs,ip,cs,sp,ss;
  uint32_t cflag;
} glRMREGS;

typedef struct {
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t reserved;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  uint16_t flags;
  uint16_t es,ds,fs,gs,ip,cs,sp,ss;
} _glRMREGS;
typedef struct {
  uint16_t es,ds,fs,gs,ip,cs,sp,ss;
} glRMSREGS;

GLvoid *glVesaPointer(const GLvoid *d) {
  GLint segment = ((GLuint)d) >> 16;
  GLint offset = ((GLuint)d) & 0xffff;
  return (GLvoid*)(((GLuint)segment<<4)+offset);
}

GLboolean glDPMI_allocRealSeg(GLint size, GLint *sel, GLint *r_seg) {
  union REGS r;
  memset(&r,0,sizeof(r));
  r.w.ax = 0x100;
  r.w.bx = (uint16_t)((size+0x0f)>>4);
  int386(0x31,&r,&r);
  if (r.w.cflag)
    return GL_FALSE;
  *sel = r.w.dx;
  *r_seg = r.w.ax;
  return GL_TRUE;
}

GLvoid glDPMI_freeRealSeg(uint16_t sel) {
  union REGS r;
  memset(&r,0,sizeof(r));
  r.w.ax = 0x101;
  r.w.dx = (uint16_t)sel;
  int386(0x31,&r,&r);
}

GLvoid glExitVBEBuf() {
  glDPMI_freeRealSeg((uint16_t)glVESABUF_sel);
}

uint16_t glDPMI_int386(uint32_t intno, glRMREGS *in, glRMREGS *out) {
  _glRMREGS rmregs;
  union REGS r;
  struct SREGS sr;
  memset(&rmregs,0,sizeof(rmregs));
  memset(&r,0,sizeof(r));
  memset(&sr,0,sizeof(sr));

  IN(ax); IN(bx); IN(cx); IN(dx); IN(si); IN(di);

  segread(&sr);
  r.w.ax = 0x300;
  r.h.bl = (uint8_t)intno;
  r.h.bh = 0;
  r.w.cx = 0;
  sr.es = sr.ds;
  r.x.edi = (uint32_t)&rmregs;
  int386x(0x31,&r,&r,&sr);

  OUT(ax); OUT(bx); OUT(cx); OUT(dx); OUT(si); OUT(di);
  out->cflag = rmregs.flags & 0x01;
  return out->ax;
}

uint16_t glDPMI_int386x(uint32_t intno, glRMREGS *in, glRMREGS *out, glRMSREGS *sregs) {
  _glRMREGS rmregs;
  union REGS r;
  struct SREGS sr;
  memset(&rmregs,0,sizeof(rmregs));
  memset(&r,0,sizeof(r));
  memset(&sr,0,sizeof(sr));
  IN(ax); IN(bx); IN(cx); IN(dx); IN(si); IN(di);
  rmregs.es = sregs->es;
  rmregs.ds = sregs->ds;

  segread(&sr);
  r.w.ax = 0x300;
  r.h.bl = (uint8_t)intno;
  r.h.bh = 0;
  r.w.cx = 0;
  sr.es = sr.ds;
  r.x.edi = (GLuint)&rmregs;
  int386x(0x31,&r,&r,&sr);

  OUT(ax); OUT(bx); OUT(cx); OUT(dx); OUT(si); OUT(di);
  sregs->es = rmregs.es;
  sregs->cs = rmregs.cs;
  sregs->ss = rmregs.ss;
  sregs->ds = rmregs.ds;
  out->cflag = rmregs.flags & 0x01;
  return out->ax;
}

GLvoid glVBE_callESDI(glRMREGS *regs, GLvoid *buffer, GLint size) {
  glRMSREGS sregs;
  memset(&sregs,0,sizeof(sregs));
  if (!glVESABUF_sel) {
    if (!glDPMI_allocRealSeg(glVESABUF_len,&glVESABUF_sel,&glVESABUF_rseg)) {
      glSetError(GL_OUT_OF_MEMORY);
      regs->ax = 0x0000;
      return;
    }
    atexit(glExitVBEBuf);
  }
  sregs.es = (uint16_t)glVESABUF_rseg;
  regs->di = 0;
  _fmemcpy(MK_FP(glVESABUF_sel,0),buffer,size);
  glDPMI_int386x(0x10,regs,regs,&sregs);
  _fmemcpy(buffer,MK_FP(glVESABUF_sel,0),size);
}

uint32_t glMapPhysicalToLinear(GLuint physicalAddress, GLuint size) {
  union REGS r;
  memset(&r,0,sizeof(r));
  r.w.ax = 0x800;
  r.w.bx = (uint16_t)(physicalAddress>>16);
  r.w.cx = (uint16_t)(physicalAddress & 0xffff);
  r.w.si = (uint16_t)(size >> 16);
  r.w.di = (uint16_t)(size & 0xffff);
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return 0;
  return (((uint32_t)r.w.bx)<<16)+r.w.cx;
}

GLboolean glUnmapPhysical(GLuint linearAddress) {
  union REGS r;
  memset(&r,0,sizeof(r));
  r.w.ax = 0x801;
  r.w.bx = (uint16_t)(linearAddress>>16);
  r.w.cx = (uint16_t)(linearAddress & 0xffff);
  int386(0x31,&r,&r);
  if (r.x.cflag)
    return GL_FALSE;
  return GL_TRUE;
}

GLboolean glVesa(GLint xRes,GLint yRes, GLint bPP) {
  glVbeInfoBlock vbeInfo;
  glVbeModeInfo modeInfo;
  memset(&vbeInfo,0,sizeof(vbeInfo));
  memset(&modeInfo,0,sizeof(modeInfo));
  glRMREGS regs;
  memset(&regs,0,sizeof(regs));
  regs.ax = 0x4f00;
  glVBE_callESDI(&regs,&vbeInfo,sizeof(glVbeInfoBlock));
  if (regs.ax != 0x004f) return GL_FALSE;
  uint16_t *u = (uint16_t *)glVesaPointer(vbeInfo.videoModes);
  uint16_t *v = u;
  while(*v != 0xffff) {v++;}
  v++; // 0xffff
  uint16_t *w=(uint16_t*)__MALLOC((v-u)*sizeof(uint16_t));
  if (w == NULL) {glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
  memcpy(w,u,((GLuint)v)-((GLuint)u));
  v = w;
  while(*v != 0xffff) {
    GLint mode = *v;
    memset(&regs,0,sizeof(regs));
    regs.ax = 0x4f01;
    regs.cx = (uint16_t)mode;
    glVBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    const GLboolean linearFrameBufferAvailable = (modeInfo.attributes & 0x90) == 0x90 ? GL_TRUE : GL_FALSE;
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP && linearFrameBufferAvailable) {

      __FREE(w);

      GLuint *frameBuffer = (GLuint *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLuint));
      if (frameBuffer == NULL) {glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
      GLfloat *depthBuffer = (GLfloat *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLfloat));
      if (depthBuffer == NULL) {__FREE(frameBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
      GLubyte *stencilBuffer = (GLubyte *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLubyte));
      if (stencilBuffer == NULL) {__FREE(frameBuffer);__FREE(depthBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}

      glBits = bPP;
      if (glBits == 15 || glBits == 16) 
        glBuildHiColorTables(&modeInfo);

      glGraphicsModeToRestore = glGetBiosGraphicsMode();
      glVesaBGRA = (modeInfo.bluePosition == 0) ? GL_TRUE : GL_FALSE;

      memset(&regs,0,sizeof(regs));
      regs.ax = 0x4f02;
      regs.bx = (uint16_t)(mode|0x4000);
      glDPMI_int386(0x10,&regs,&regs); // needed here for later mouse setup

      glCleanSetup(xRes, yRes, frameBuffer, depthBuffer, stencilBuffer);
      glSetupMouse();

      glFrameBufferDedicated = (GLuint*)glMapPhysicalToLinear(modeInfo.linearFrameBuffer,4096*1024-1);
      memset(glFrameBufferDedicated,0,xRes*yRes*sizeof(GLuint));

      return GL_TRUE;
    }
    v++;
  }
  __FREE(w);
  return GL_FALSE;
}
// --------------------------------------
#endif // __WATCOMC__
// --------------------------------------
#ifdef __DJGPP__
// --------------------------------------

#define VGARAMBASE (0xa0000+__djgpp_conventional_base)
#define DJGPPVGAMEMON if (useNearPointers) __djgpp_nearptr_enable();
#define DJGPPVGAMEMOFF if (useNearPointers) __djgpp_nearptr_disable();

GLint glVESABUF_len=1024; // length of struct to work on with realmode es:di

GLboolean glVESABUF_sel_init = GL_FALSE; // we need VESABUF_sel just once

GLint glVesasel; // final linear framebuffer accessor (_farpokel)

_go32_dpmi_seginfo glVESABUF_sel; // struct to work on with realmode es:di

__dpmi_meminfo glLinearFrameBufferStruct; // physical to linear address mapping for linear framebuffer


GLboolean glUnmapPhysical(GLuint linearAddress) {
  __UNUSED(linearAddress);
  __dpmi_free_physical_address_mapping(&glLinearFrameBufferStruct);
  __dpmi_free_ldt_descriptor(glVesasel);
  return GL_TRUE;
}

GLvoid glExitVBEBuf() {
  _go32_dpmi_free_dos_memory(&glVESABUF_sel); // called at exit of app
}

GLvoid glVBE_callESDI(__dpmi_regs *regs, GLvoid *buffer, GLint size) {
  if (!glVESABUF_sel_init) {
    glVESABUF_sel.size = (glVESABUF_len+15)/16;
    if (_go32_dpmi_allocate_dos_memory(&glVESABUF_sel) == -1) {
      glSetError(GL_OUT_OF_MEMORY);
      regs->x.ax = 0x0000;
      return;
    } 
    glVESABUF_sel_init = GL_TRUE;
    atexit(glExitVBEBuf);
  }
  regs->x.es = glVESABUF_sel.rm_segment;
  regs->x.di = glVESABUF_sel.rm_offset;
  unsigned long addr; // here long seems to be 64bit
  __dpmi_get_segment_base_address(glVESABUF_sel.pm_selector, &addr);
  DJGPPVGAMEMON
  GLvoid *ptr = (GLvoid*)(addr+__djgpp_conventional_base);
  memcpy(ptr,buffer,size);
  __dpmi_simulate_real_mode_interrupt(0x10, regs);
  memcpy(buffer,ptr,size);
  //printf("%c%c%c%c\n",((GLbyte*)buffer)[0],((GLbyte*)buffer)[1],((GLbyte*)buffer)[2],((GLbyte*)buffer)[3]);
  DJGPPVGAMEMOFF
}

GLboolean glVesa(GLint xRes,GLint yRes, GLint bPP) {

  glVbeInfoBlock vbeInfo;
  glVbeModeInfo modeInfo;
  __dpmi_regs regs;
  memset(&regs,0,sizeof(__dpmi_regs));
  regs.x.ax = 0x4f00;
  glVBE_callESDI(&regs,&vbeInfo,sizeof(glVbeInfoBlock));
  if (regs.x.ax != 0x004f) return GL_FALSE;

  GLuint videoModes = (((GLuint)vbeInfo.videoModes)>>16<<4)+(((GLuint)vbeInfo.videoModes) & 0xffff);
  GLuint ofs = videoModes;
  do {
    uint16_t k = 0;
    dosmemget(ofs,2,&k);
    ofs += 2;
    if (k == 0xffff) break;
  } while(GL_TRUE);
  uint16_t *w=(uint16_t*)__MALLOC((ofs-videoModes)/2*sizeof(uint16_t));
  if (w == NULL) {glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
  uint16_t *v=w;
  dosmemget(videoModes,ofs-videoModes,w);

  while(*v != 0xffff) {
    GLint mode = *v;
    memset(&regs,0,sizeof(__dpmi_regs));
    regs.x.ax = 0x4f01;
    regs.x.cx = (uint16_t)mode;
    glVBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    const GLboolean linearFrameBufferAvailable = (modeInfo.attributes & 0x90) == 0x90 ? GL_TRUE : GL_FALSE;
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP && linearFrameBufferAvailable) {

      __FREE(w);

      GLuint *frameBuffer = (GLuint *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLuint));
      if (frameBuffer == NULL) {glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
      GLfloat *depthBuffer = (GLfloat *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLfloat));
      if (depthBuffer == NULL) {__FREE(frameBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
      GLubyte *stencilBuffer = (GLubyte *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLubyte));
      if (stencilBuffer == NULL) {__FREE(frameBuffer);__FREE(depthBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}

      glBits = bPP;
      if (glBits == 15 || glBits == 16) 
        glBuildHiColorTables(&modeInfo);

      glGraphicsModeToRestore = glGetBiosGraphicsMode();
      glVesaBGRA = (modeInfo.bluePosition == 0) ? GL_TRUE : GL_FALSE;

      memset(&regs,0,sizeof(__dpmi_regs));
      regs.x.ax = 0x4f02;
      regs.x.bx = (uint16_t)(mode|0x4000);
      __dpmi_simulate_real_mode_interrupt(0x10, &regs); // needed here for later mouse setup

      glCleanSetup(xRes, yRes, frameBuffer, depthBuffer, stencilBuffer);
      glSetupMouse();

      // linear framebuffer mapping to vesasel not glFrameBufferDedicated
      glLinearFrameBufferStruct.size = 4096*1024-1;
      glLinearFrameBufferStruct.address = modeInfo.linearFrameBuffer;
      __dpmi_physical_address_mapping(&glLinearFrameBufferStruct);
      glVesasel = __dpmi_allocate_ldt_descriptors(1);
      __dpmi_set_segment_base_address(glVesasel,glLinearFrameBufferStruct.address);
      __dpmi_set_segment_limit(glVesasel,glLinearFrameBufferStruct.size-1);
      for (GLint i = 0; i < xRes*yRes; i++) _farpokel(glVesasel,i<<2,0);
      glFrameBufferDedicated = (GLuint*)0x10101010; // not NULL so glDone calls unmap

      return GL_TRUE;
    }
    v++;
  }
  __FREE(w);
  return GL_FALSE;
}
// --------------------------------------
#endif // __DJGPP__
// --------------------------------------

GLvoid glSetBiosGraphicsMode(uint16_t mode) {
    union REGS regs;
    memset(&regs,0,sizeof(regs));
    regs.w.ax = mode;
    int386(0x10, &regs, &regs);
}

uint16_t glGetBiosGraphicsMode() {
    union REGS regs;
    memset(&regs,0,sizeof(regs));
    regs.w.ax = 0x0f00;
    int386 (0x10, &regs, &regs);
    return regs.h.al;
}

GLuint glToColor(GLint r,GLint g,GLint b) {
  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;
  return r|(g<<8)|(b<<16)|0xff000000;
}

GLvoid glSetPalette(GLint index, GLuint color) {
  outp(0x3c8,index);
  outp(0x3c9,(color & 255)>>2);
  outp(0x3c9,((color>>8) & 255)>>2);
  outp(0x3c9,((color>>16) & 255)>>2);
}

GLvoid glSetHiColorPalette() {
  GLint i;
  for (i = 0; i < 64; i++) glSetPalette(i,glToColor(0,i*4+3,0));
  for (i = 0; i < 16*8; i++) glSetPalette(i+128,glToColor((i/8)*16+15,0,(i & 7)*32+31));
}

GLvoid glSetModeX() {
  outp(0x3c4,4);
  GLint a = inp(0x3c5);
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

GLvoid glSet400Lines() {
  outp(0x3d4,9);
  GLint a = inp(0x3d5);
  a &= 16+32+64;//001110000b
  outp(0x3d5,a);
}

GLvoid glSetVGABufferStart(GLuint adr) {
  outp(0x3d4,0x0c);
  outp(0x3d5,(adr >> 8) & 255);
  outp(0x3d4,0x0d);
  outp(0x3d5,adr & 255);
}

GLboolean glVGA() {
  GLuint xRes = 320;
  GLuint yRes = 200;

  GLuint *frameBuffer = (GLuint *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLuint));
  if (frameBuffer == NULL) {glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
  GLfloat *depthBuffer = (GLfloat *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLfloat));
  if (depthBuffer == NULL) {__FREE(frameBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}
      GLubyte *stencilBuffer = (GLubyte *)__MALLOC(xRes*yRes*glFrameBufferMultiSample*sizeof(GLubyte));
      if (stencilBuffer == NULL) {__FREE(frameBuffer);__FREE(depthBuffer);glSetError(GL_OUT_OF_MEMORY); return GL_FALSE;}

  glGraphicsModeToRestore = glGetBiosGraphicsMode();

  DJGPPVGAMEMON  
  memset((uint8_t*)(VGARAMBASE),0,0x10000);
  DJGPPVGAMEMOFF
  glSetBiosGraphicsMode(0x13);
  glSetModeX();
  glSet400Lines();
  glSetHiColorPalette();
  glSetVGABufferStart(0);
  for (GLint i = 0; i < 256; i++) glHiRedTab[i] = (uint8_t)(128+(((i>>4)&15)<<3));

  glCleanSetup(xRes, yRes, frameBuffer, depthBuffer, stencilBuffer);
  glSetupMouse();
  glHiColor = GL_TRUE;

  return GL_TRUE;
}

GLvoid glRefreshHiColor() {
  DJGPPVGAMEMON
  uint8_t *vga = (uint8_t*)(VGARAMBASE+(glOtherFrame ? 0x8000 : 0x00));
  for (GLint p = 0; p < 4; p++) {
    outp(0x3c4,0x02);
    outp(0x3c5,1<<p);
    uint8_t *s = (uint8_t*)&glFrameBuffer[p];
    for (GLint y = 0; y < 200; y++) {
      int k0 = glVGACheckered ? (p & 1) : 0;
      int k1 = 1-k0;
      uint8_t *w1 = &vga[y*(320/4*2)+k0*320/4];
      uint8_t *w2 = &vga[y*(320/4*2)+k1*320/4];
      for (GLint x = 320/4; x > 0; x--) {
        *w2++ = (uint8_t)(s[1]>>2);
        *w1++ = (uint8_t)((s[2]>>5)|glHiRedTab[s[0]]);
        s += 16;
      }
    }
  }
  DJGPPVGAMEMOFF
  glSetVGABufferStart(glOtherFrame ? 0x8000 : 0x0000);
  glOtherFrame = !glOtherFrame;   
}

GLvoid glWaitVerticalRetrace() {
  while((inp(0x03da)&8)!=0) {;}
  while((inp(0x03da)&8)==0) {;} // wait till retrace start
}

GLvoid glFlattenMultiSample() {
  if (glFrameBufferMultiSample==2) {
    for (int i = 0; i < glFrameBufferWidth*glFrameBufferHeight; i++) {
      glFrameBuffer[i]=((glFrameBuffer[i] & 0xfefefefe)+(glFrameBuffer[i+glFrameBufferWidth*glFrameBufferHeight] & 0xfefefefe))>>1;
    }
  }
}

GLvoid glRefresh() {

  glFlattenMultiSample();
  
  if (glDirectBlit) 
    return;
  
  if (glWaitVSync) 
    glWaitVerticalRetrace();
  
  if (glHiColor) {
    glRefreshHiColor();
    return;
  }

  if (glBits == 32) {
    // 32bit
    GLuint *read = &glFrameBuffer0[0];
    const GLint k = glFrameBufferWidth0*glFrameBufferHeight0;
    if (glVesaBGRA) {
      for (GLint i = 0; i < k; i++) {
        const GLuint rgba = *read++;
#ifdef __DJGPP__
        _farpokel(glVesasel,i<<2,(rgba & 0xff00ff00)|((rgba&0x00ff00ff)>>16)|((rgba&0x00ff00ff)<<16));
#else // __DJGPP__
        glFrameBufferDedicated[i] = (rgba & 0xff00ff00)|((rgba&0x00ff00ff)>>16)|((rgba&0x00ff00ff)<<16);
#endif // __DJGPP__
      }
    } else {
      for (GLint i = 0; i < k; i++) {
        const GLuint rgba = *read++;
#ifdef __DJGPP__
        _farpokel(glVesasel,i<<2,rgba);
#else // __DJGPP__
        glFrameBufferDedicated[i] = rgba;
#endif // __DJGPP__
      }
    }
  } 

  if (glBits == 24) { // very slow
    //24bit
    GLuint *read = &glFrameBuffer0[0];
    const GLint k = glFrameBufferWidth0*glFrameBufferHeight0*3;
#ifndef __DJGPP__
    GLubyte *dedicated = (GLubyte*)glFrameBufferDedicated;
#endif // __DJGPP__
    if (glVesaBGRA) {
      for (GLint i = 0; i < k; i += 3) {
        const GLuint rgba = *read++;
#ifdef __DJGPP__
        _farpokeb(glVesasel,i,(rgba>>16) & 0xff);
        _farpokeb(glVesasel,i+1,(rgba>>8) & 0xff);
        _farpokeb(glVesasel,i+2,rgba & 0xff);
#else // __DJGPP__
        dedicated[0] = (GLubyte)((rgba>>16) & 0xff);
        dedicated[1] = (GLubyte)((rgba>>8) & 0xff);
        dedicated[2] = (GLubyte)(rgba & 0xff);
        dedicated += 3;
#endif // __DJGPP__
      }
    } else {
      for (GLint i = 0; i < k; i += 3) {
        const GLuint rgba = *read++;
#ifdef __DJGPP__
        _farpokeb(glVesasel,i,rgba & 0xff);
        _farpokeb(glVesasel,i+1,(rgba>>8) & 0xff);
        _farpokeb(glVesasel,i+2,(rgba>>16) & 0xff);
#else // __DJGPP__
        dedicated[0] = (GLubyte)(rgba & 0xff);
        dedicated[1] = (GLubyte)((rgba>>8) & 0xff);
        dedicated[2] = (GLubyte)((rgba>>16) & 0xff);
        dedicated += 3;
#endif // __DJGPP__
      }
    }
  }

  if (glBits == 15 || glBits == 16) {
    //16bit
    GLuint *read = &glFrameBuffer0[0];
    const GLint k = glFrameBufferWidth0*glFrameBufferHeight0;
#ifndef __DJGPP__
    GLushort *dedicated = (GLushort*)glFrameBufferDedicated;
#endif // __DJGPP__
    for (GLint i = 0; i < k; i++) {
      const GLuint rgba = *read++;
      const GLushort in16 = (GLushort)(glHiColorTableR[rgba&0xff]|glHiColorTableG[(rgba>>8)&0xff]|glHiColorTableB[(rgba>>16)&0xff]);
#ifdef __DJGPP__
      _farpokew(glVesasel,i*2,in16);
#else // __DJGPP__
      *dedicated++ = in16;
#endif // __DJGPP__
    }
  }
}

GLvoid glDone() {

  if (!glDirectBlit) { // glDirect?

    if (glFrameBuffer0 != NULL) {
      __FREE(glFrameBuffer0); glFrameBuffer0 = NULL; glFrameBuffer = NULL;
    }

    if (glDepthBuffer0 != NULL) {
      __FREE(glDepthBuffer0); glDepthBuffer0 = NULL; glDepthBuffer = NULL;
    }

    if (glStencilBuffer0 != NULL) {
      __FREE(glStencilBuffer0); glStencilBuffer0 = NULL; glStencilBuffer = NULL;
    }

    if (glFrameBufferDedicated != NULL) {
      glUnmapPhysical((GLuint)glFrameBufferDedicated);
      glFrameBufferDedicated = NULL;
    }

    if (glGraphicsModeToRestore>=0) {
      glSetBiosGraphicsMode((GLushort)glGraphicsModeToRestore);
      glGraphicsModeToRestore = -1;
    }

  }

}

GLushort glNextKey() {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x0600;
  regs.w.dx = 0xff;
  int386(0x21, &regs, &regs);
  glKey = regs.w.ax & 0xff;
  if (glKey == 0) {
    memset(&regs,0,sizeof(regs));
    regs.w.ax = 0x0600;
    regs.w.dx = 0xff;
    int386(0x21, &regs, &regs);
    glKey += (regs.w.ax & 0xff)<<8;
  }
  return (GLushort)glKey;
}

GLvoid glSetMousePos(GLint x, GLint y) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x04;
  regs.w.cx = (uint16_t)(x & 0xffff);
  regs.w.dx = (uint16_t)(y & 0xffff);
  int386(0x33, &regs, &regs);
}

GLvoid glNextMouseDelta(GLdouble *mouseX, GLdouble *mouseY) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
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


GLushort glMouseButtons() {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseB = glMouseB;
  glMouseB = regs.w.bx & 0xffff;
  return (GLushort)glMouseB;
}

GLvoid glSetupMouse() {
    union REGS regs;
    memset(&regs,0,sizeof(regs));
    regs.w.ax = 0x07;
    regs.w.cx = 0;
    regs.w.dx = (uint16_t)(glFrameBufferWidth0-1);
    int386(0x33, &regs, &regs);
    memset(&regs,0,sizeof(regs));
    regs.w.ax = 0x08;
    regs.w.cx = 0;
    regs.w.dx = (uint16_t)(glFrameBufferHeight0-1);
    int386(0x33, &regs, &regs);
    glMouseX = glFrameBufferWidth0/2;
    glMouseY = glFrameBufferHeight0/2;
    glSetMousePos(glMouseX, glMouseY);
}

GLvoid glSpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey) {
#ifdef __WATCOMC__
  uint8_t keyState = *((uint8_t *)0x417);
#endif // __WATCOMC__
#ifdef __DJGPP__
  uint8_t keyState;
  dosmemget(0x417,1,&keyState);
#endif // __DJGPP__
  *shiftKey = ((keyState & 0x03) != 0) ? GL_TRUE : GL_FALSE;
  *ctrlKey = ((keyState & 0x04) != 0) ? GL_TRUE : GL_FALSE;
  *altKey = ((keyState & 0x08) != 0) ? GL_TRUE : GL_FALSE;
}

#ifdef __DJGPP__
  typedef signed long long int int64_t;
  typedef unsigned long long int uint64_t;
  int64_t glStartClock = 0;
  GLdouble glSeconds() {
    return (GLdouble)(uclock() - glStartClock) / UCLOCKS_PER_SEC;
  }
  GLvoid glSetTime(GLdouble seconds) {
    glStartClock = uclock() - (int64_t)(seconds * UCLOCKS_PER_SEC);
  }
#endif // __DJGPP__

#ifdef __WATCOMC__
  GLdouble glStartClock = 0;
  // very bad resolution, things seem to stutter by that, better use speaker.hpp and it's reprogrammed timer
  GLdouble glSeconds() {
    return (GLdouble)(clock() - glStartClock) / CLOCKS_PER_SEC;
  }
  GLvoid glSetTime(GLdouble seconds) {
    glStartClock = clock() - seconds * CLOCKS_PER_SEC;
  }
#endif // __WATCOMC__

#endif // __GLDISABLEDOSFUNCTIONS__

// --------------------------------------
// --------------------------------------
// --------------------------------------

#ifdef __cplusplus
}
#endif // __cplusplus

/*
MIT License

Copyright (c) 2025 Stefan Mader

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

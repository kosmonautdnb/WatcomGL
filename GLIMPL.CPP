#include "gl.h"
#include <string.h>
#include <math.h>

int glFrameBufferWidth = 0;
int glFrameBufferHeight = 0;
unsigned int *glFrameBuffer = NULL;
unsigned int *glFrameBufferDedicated = NULL;
int *glStencilBuffer = NULL;
float *glDepthBuffer = NULL;

GLfloat identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};

class _GLContext {
public:
  _GLContext() {
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
    blendFuncSFactor = 0;
    blendFuncDFactor = 0;
    cullFaceMode = 0;
    depthFunc = 0;
    depthMask = 0;
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
    lineWidth = 0;
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
    beginMode = 0;
  }
  void enable(GLenum prop, bool enable) {enabledCaps[prop&255]=enable;}
  GLboolean isEnabled(GLenum prop) {return enabledCaps[prop&255];}
  void enableClientState(GLenum prop, bool enable) {;}
  GLboolean getBoolean(GLenum prop) {return true;}
  GLfloat getFloat(GLenum prop) {return 0.f;}
  GLint getInteger(GLenum prop) {return 0;}
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
  GLenum beginMode;
  GLboolean enabledCaps[256];
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
  float sx,sy,sz;
} glVertex;

glVertex glVertices[4];
int glCurrentVertexElement = 0;

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
        a += mb[i+k*4] * ma[j*4+k];
      }
      result[i+j*4] = a;
    }
  }
  memcpy(r,result,4*4*sizeof(float));
}

void glUpdateMatrix() {
  glMatMul(glContext.matrixForMode[0],glContext.matrixForMode[1],glContext.matrix);
}

void glDrawTriangle(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2);
void glDrawQuad(_GLContext *context, glVertex *v0,glVertex *v1,glVertex *v2,glVertex *v3);

int glTransformVertex(_GLContext *context, glVertex *v) {
  float x = v->vertexX * context->matrix[0*4+0] + v->vertexY * context->matrix[1*4+0] + v->vertexZ * context->matrix[2*4+0] + v->vertexW * context->matrix[3*4+0];
  float y = v->vertexX * context->matrix[0*4+1] + v->vertexY * context->matrix[1*4+1] + v->vertexZ * context->matrix[2*4+1] + v->vertexW * context->matrix[3*4+1];
  float z = v->vertexX * context->matrix[0*4+2] + v->vertexY * context->matrix[1*4+2] + v->vertexZ * context->matrix[2*4+2] + v->vertexW * context->matrix[3*4+2];
  float w = v->vertexX * context->matrix[0*4+3] + v->vertexY * context->matrix[1*4+3] + v->vertexZ * context->matrix[2*4+3] + v->vertexW * context->matrix[3*4+3];

  if (w <= 0.0001f) 
    return 1;
     
  v->sx = x/w;
  v->sy = y/w;
  v->sz = z/w;

  v->sx *= (context->viewportX1-context->viewportX0)*0.5;
  v->sy *= (context->viewportY1-context->viewportY0)*-0.5;
  v->sx += (context->viewportX0+context->viewportX1)*0.5;
  v->sy += (context->viewportY0+context->viewportY1)*0.5;
  return 0;
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
  glCurrentVertexElement++;
  if (glCurrentVertexElement == 3 && glContext.beginMode == GL_TRIANGLES) {
    int a = glTransformVertex(&glContext,&glVertices[0]);
    a |= glTransformVertex(&glContext,&glVertices[1]);
    a |= glTransformVertex(&glContext,&glVertices[2]);
    if (a == 0) glDrawTriangle(&glContext,&glVertices[0],&glVertices[1],&glVertices[2]);
    glCurrentVertexElement = 0;
  }
  if (glCurrentVertexElement == 4 && glContext.beginMode == GL_QUADS) {
    int a = glTransformVertex(&glContext,&glVertices[0]);
    a |= glTransformVertex(&glContext,&glVertices[1]);
    a |= glTransformVertex(&glContext,&glVertices[2]);
    a |= glTransformVertex(&glContext,&glVertices[3]);
    if (a == 0) glDrawQuad(&glContext,&glVertices[0],&glVertices[1],&glVertices[2],&glVertices[3]);
    glCurrentVertexElement = 0;
  }
}

void glActiveTexture (GLenum texture) {
  glContext.activeTexture = texture;
}

void glAlphaFunc(GLenum func, GLclampf ref) {
  glContext.alphaFunc = func;
  glContext.alphaFuncRef = ref;
}

void glBegin(GLenum mode) {
  glContext.beginMode = mode;
  glCurrentVertexElement = 0;
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
}

void glCullFace(GLenum mode) {
  glContext.cullFaceMode = mode;
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

void glGetBooleanv(GLenum pname, GLboolean *params) {
  *params = glContext.getBoolean(pname);
}

void glGetFloatv (GLenum pname, GLfloat *params) {
  *params = glContext.getFloat(pname);
}

void glGetIntegerv (GLenum pname, GLint *params) {
  *params = glContext.getInteger(pname);
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
}

void glMaterialfv (GLenum face, GLenum pname, const GLfloat *params) {
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

void gluPerspective(GLfloat fov, GLfloat aspect, GLfloat nearPlane, GLfloat farPlane) {
  glhPerspectivef2(glContext.matrixForMode[glContext.matrixModeNr],fov,aspect,nearPlane,farPlane);
}

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

void gluLookAt(GLfloat cx,GLfloat cy,GLfloat cz,GLfloat ox,GLfloat oy,GLfloat oz,GLfloat ux,GLfloat uy,GLfloat uz) {

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
float getBary(int x, int y, const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  float dx = v2->sx - v1->sx;
  float dy = v2->sy - v1->sy;
  float nx = dy;
  float ny = -dx;
  float nl = sqrt(nx*nx+ny*ny);
  if (nl > 0) {
    nx /= nl;
    ny /= nl;
  }
  float l = (v0->sx-v1->sx)*nx + (v0->sy-v1->sy)*ny;
  float p = (x-v1->sx)*nx + (y-v1->sy)*ny;
  return p/l;
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
  float bary0,bary1;\
  float baryAdd0,baryAdd1;\
  unsigned int*pDest;\
  float *zDest;\
  for (y = pminy;y<pmaxy;y++) {

#define __PAINTPOLYQUAD_INITBARYFORX__\
  bary0 = getBary(pminx,y,v0,v1,v2);\
  bary1 = getBary(pminx,y,v1,v2,v0);\
  baryAdd0 = (getBary(pmaxx,y,v0,v1,v2)-bary0)/(pmaxx-pminx);\
  baryAdd1 = (getBary(pmaxx,y,v1,v2,v0)-bary1)/(pmaxx-pminx);
  
#define __PAINTPOLYQUAD_BEGINX__\
  pDest = &glFrameBuffer[pminx+y*glFrameBufferWidth];\
  zDest = &glDepthBuffer[pminx+y*glFrameBufferWidth];\
  int dminx = pminx;\
  int dmaxx = pmaxx;\
  for (x = dminx;x<dmaxx;x++) {


#define __BARY0__(__v__) ((__v__)*bary0)
#define __BARY1__(__v__) ((__v__)*bary1)
#define __BARY2__(__v__) ((__v__)*(1.f-bary0-bary1))

bool glCCW(const glVertex *v0, const glVertex *v1, const glVertex *v2) {
  const float dx0 = v1->sx - v0->sx;
  const float dy0 = v1->sy - v0->sy;
  const float dx1 = v2->sx - v0->sx;
  const float dy1 = v2->sy - v0->sy;
  return (dx0*dy1 - dy0*dx1)>=0;
}

__inline bool glCheckDepthFunction(_GLContext *context, float depthBufferValue, float newValue) {
  return newValue <= depthBufferValue;
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

  float v0z = 1.f/v0->sz;
  float v1z = 1.f/v1->sz;
  float v2z = 1.f/v2->sz;

  float r0 = v0->colorRed;
  float g0 = v0->colorGreen;
  float b0 = v0->colorBlue;
  float a0 = v0->colorAlpha;

  float r1 = v1->colorRed;
  float g1 = v1->colorGreen;
  float b1 = v1->colorBlue;
  float a1 = v1->colorAlpha;

  float r2 = v2->colorRed;
  float g2 = v2->colorGreen;
  float b2 = v2->colorBlue;
  float a2 = v2->colorAlpha;

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
   
  r0 = r0*255.f*v0z;
  g0 = g0*255.f*v0z;
  b0 = b0*255.f*v0z;
  a0 = a0*255.f*v0z;

  r1 = r1*255.f*v1z;
  g1 = g1*255.f*v1z;
  b1 = b1*255.f*v1z;
  a1 = a1*255.f*v1z;

  r2 = r2*255.f*v2z;
  g2 = g2*255.f*v2z;
  b2 = b2*255.f*v2z;
  a2 = a2*255.f*v2z;


  __PAINTPOLYQUAD_BEGINY__
  __PAINTPOLYQUAD_INITBARYFORX__
  __PAINTPOLYQUAD_BEGINX__
      if (bary0>=0.f&&bary1>=0.f&&(bary0+bary1)<1.f) {
        float iz = 1.f/(__BARY0__(v0z)+__BARY1__(v1z)+__BARY2__(v2z));
        if (glCheckDepthFunction(context,*zDest,iz)) {
          int r = (__BARY0__(r0)+__BARY1__(r1)+__BARY2__(r2))*iz;
          int g = (__BARY0__(g0)+__BARY1__(g1)+__BARY2__(g2))*iz;
          int b = (__BARY0__(b0)+__BARY1__(b1)+__BARY2__(b2))*iz;
          *zDest=iz;
          *pDest=r|(g<<8)|(b<<16)|0xff000000;
        }
      }
      pDest++;
      zDest++;
      bary0+=baryAdd0;
      bary1+=baryAdd1;
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
}
// --------------------------------------
// --------------------------------------
// --------------------------------------

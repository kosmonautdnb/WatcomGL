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
#ifndef __GL_H__
#define __GL_H__

/*
#include "GL.H"

int main(int argc, const char **argv) {
  if (!glVesa(640,480,32))
    glVGA();

  while(1) {
    if (glNextKey() == GL_VK_ESCAPE) break;
    glClearColor(0.2,0.4,0.6,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glRefresh();
  }

  glDone();
  return 0;
}
*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// ------------------------
// ------------------------
#define GL_TRUE 1
#define GL_FALSE 0
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef short GLshort;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef double GLdouble;
typedef float GLclampf;
typedef void GLvoid;

// ------------------------
// ------------------------

// in C++ the context is already constructed at program start
// in C glVesa,glVGA,glDirect construct the context

GLboolean glVesa(GLint xRes,GLint yRes, GLint bPP); // e.g. glVesa(320,200,32) enables Vesa mode 320x200x32BitColors (only 32 bit, yet)
GLboolean glVGA(); // 320x200 with 6 bit green, 3 bit blue, 4 bit red, actually it's 320x400
GLboolean glDirect(GLuint *frameBuffer, GLfloat *depthBuffer, GLuint width, GLuint height); // setup direct rendering to a framebuffer+depthbuffer // look also at glSetRenderTarget() // you need to delete the buffers yourself

GLvoid glRefresh(); // copies the framebuffer to the actual screen and updates per frame stuff

GLvoid glDone(); // close the Vesa/VGA mode and clean up OpenGL again

// ------------------------
// ------------------------

extern GLint glFrameBufferWidth;
extern GLint glFrameBufferHeight;
extern GLuint *glFrameBuffer;
extern GLfloat *glDepthBuffer;
extern GLint glFrameBufferBytesPerPixel;
extern GLdouble glPixelCenterX;
extern GLdouble glPixelCenterY;
extern GLdouble glTexelCenterX;
extern GLdouble glTexelCenterY;
extern GLdouble glMonitorAspectRatio; // use glSetMonitorAspectRatio()
extern GLboolean useNearPointers; // option for DJGPP, default GL_TRUE
extern GLboolean glUseHalveVector; // seems to be OSMesa is using this, the docs require phong using the reflection vector, default GL_FALSE
extern GLboolean glFastTexturing; // use perspective approximations for more performance, default GL_FALSE
extern GLboolean glVGACheckered; // glVGA() with "dithering", default GL_FALSE
extern GLboolean glWaitVSync; // to achieve better "double buffer" enable this. Use this with care, since this is a VGA function and not Vesa, default GL_FALSE 
extern GLint glFastTextureSpanWidth;
extern GLenum glError;

// ------------------------
// ------------------------

GLvoid glSetRenderTarget(GLuint *frameBufferOrNULL, GLfloat *depthBuffer, GLuint width, GLuint height); // set a render target (NULL sets screen/default render target)

// ------------------------
// ------------------------

GLdouble glSeconds(); // seconds elapsed since the start of OpenGL (it's quite stuttery on WatcomC)
GLvoid glSetTime(GLdouble seconds); // sets the time (in seconds)
                        
GLushort glNextKey(); // gets the next pressed key from the keyboard buffer 0 or (GL_VK_xxxx or 'a' etc..)
GLvoid glNextMouseDelta(GLdouble *deltaX, GLdouble *deltaY); // gets the current mouse delta and prepares for a new mouse delta (mousePos+=delta)
GLubyte glMouseButtons(); // the current mouse button state (GL_MOUSE_BUTTON_xxxx)
GLvoid glSpecialKeys(GLboolean *shiftKey, GLboolean *ctrlKey, GLboolean *altKey);

GLuint glGetTextureWidth(GLuint textureId);
GLuint glGetTextureHeight(GLuint textureId);

GLvoid glExplicitAlpha(GLboolean useExplicitAlpha, GLfloat alpha); // writes this alpha to the framebuffer when rendering polygons (maybe helps as some sort of "stencil buffer") // alpha between 0 and 1
GLboolean glPixel(GLboolean newXYZ, GLfloat xp, GLfloat yp, GLfloat GLzp, GLint x, GLint y, GLuint color); // paint pixel at 3d position (x,y,z) plus 2D offset (x,y) (returns GL_FALSE if clipped) (newXYZ calculates new 3d/2d pos from xp,yp,zp, without it, it will take just the last one)
GLvoid glAdditionalPointSpriteXStretch(GLfloat widthStretch); // for better (quadratic) point sprites and lines on e.g. 16:9 screens

// ------------------------
// ------------------------

GLvoid glActiveTexture(GLenum texture); // supported
GLvoid glAlphaFunc(GLenum func, GLclampf ref); // supported
GLvoid glBegin(GLenum mode); // supported (GL_POINTS, GL_LINES, GL_TRIANGLES, GL_QUADS, GL_LINE_STRIP)
GLvoid glBindTexture(GLenum target, GLuint texture); // supported
GLvoid glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
GLvoid glBlendFunc(GLenum sfactor, GLenum dfactor); // supported
GLvoid glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
GLvoid glClear(GLbitfield mask); // supported
GLvoid glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); // supported
GLvoid glClearDepthf(GLclampf depth); // supported
GLvoid glClearDepth(GLclampf depth); // supported
GLvoid glClearStencil(GLint s);
GLvoid glClientActiveTexture(GLenum texture);
GLvoid glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha); // supported
GLvoid glColor4fv(const GLfloat *v); // supported
GLvoid glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha); // supported
GLvoid glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha); // supported
GLvoid glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid glColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *table);
GLvoid glColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
GLvoid glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
GLvoid glCullFace(GLenum mode); // supported
GLvoid glDepthFunc(GLenum func); // supported
GLvoid glDepthMask(GLboolean flag); // supported
GLvoid glDepthRangef(GLclampf zNear, GLclampf zFar); // supported (no clipping of values, gluProject/gluUnProject not right here)
GLvoid glDisable(GLenum cap); // supported
GLvoid glDisableClientState(GLenum array);
GLvoid glDrawArrays(GLenum mode, GLint first, GLsizei count);
GLvoid glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
GLvoid glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
GLvoid glEnable(GLenum cap); // supported
GLvoid glEnableClientState(GLenum array);
GLvoid glEnd(); // supported
GLvoid glEndList();
GLvoid glFinish(); // supported
GLvoid glFlush(); // supported
GLvoid glFogfv(GLenum pname, GLfloat *params); // :mad: supported
GLvoid glFogf(GLenum pname, GLfloat param); // :mad: supported
GLvoid glFogi(GLenum pname, GLint param); // :mad: supported
GLvoid glFrontFace(GLenum mode); // supported
GLvoid glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar); // supported
GLuint glGenLists(GLsizei range);
GLvoid glGenTextures(GLsizei n, GLuint *textures); // supported
GLenum glGetError();
GLvoid glGetBooleanv(GLenum pname, GLboolean *params); // supported
GLvoid glGetDoublev(GLenum pname, GLdouble *params); // supported
GLvoid glGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table);
GLvoid glGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params);
GLvoid glGetFloatv(GLenum pname, GLfloat *params); // supported
GLvoid glGetIntegerv(GLenum pname, GLint *params); // supported
GLvoid glGetLightfv(GLenum light, GLenum pname, GLfloat *params); // supported
GLvoid glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params); // supported
GLvoid glGetPointerv(GLenum pname, GLvoid * *params);
GLvoid glGetPolygonStipple(GLubyte *mask);
GLvoid glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
GLvoid glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
GLvoid glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
const GLubyte *glGetString(GLenum name);
GLvoid glHint(GLenum target, GLenum mode);
GLboolean glIsEnabled(GLenum cap); // supported
GLvoid glLightf(GLenum light, GLenum pname, GLfloat param); // supported
GLvoid glLightfv(GLenum light, GLenum pname, const GLfloat *params); // supported
GLvoid glLightModelfv(GLenum pname, const GLfloat *params);
GLvoid glLightModeli(GLenum pname, const GLenum param); // supported partially (GL_SEPARATE_SPECULAR_COLOR,GL_SINGLE_COLOR,GL_LIGHT_MODEL_TWO_SIDE)
GLvoid glLightModelf(GLenum pname, const GLfloat param); // supported partially (GL_LIGHT_MODEL_TWO_SIDE)
GLvoid glLineStipple(GLint factor, GLushort pattern);
GLvoid glLineWidth(GLfloat width); // supported
GLvoid glListBase(GLuint base);
GLvoid glLoadIdentity(); // supported
GLvoid glLoadMatrixf(const GLfloat *m); // supported
GLvoid glLoadMatrixd(const GLdouble *m); // supported
GLvoid glMaterialf(GLenum face, GLenum pname, GLfloat param);
GLvoid glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
GLvoid glMatrixMode(GLenum mode); // supported
GLvoid glMultMatrixf(const GLfloat *m); // supported
GLvoid glMultMatrixd(const GLdouble *m); // supported
GLvoid glMultiTexCoord2f(GLenum target, GLfloat s, GLfloat t);
GLvoid glMultiTexCoord2fv(GLenum target, const GLfloat *v);
GLvoid glNewList(GLuint list, GLenum mode);
GLvoid glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz); // supported
GLvoid glNormal3fv(const GLfloat *v); // supported
GLvoid glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat zNear, GLfloat zFar); // supported
GLvoid glPixelStorei(GLenum pname, GLint param);
GLvoid glPointSize(GLfloat size); // supported
GLvoid glPolygonMode(GLenum face, GLenum mode); // supported
GLvoid glPolygonOffset(GLfloat factor, GLfloat units); // supported, only units (maybe inverse + scaled (don't rely on this "implementation"))
GLvoid glPolygonStipple(const GLubyte *mask);
GLvoid glPopMatrix(); // supported
GLvoid glPushMatrix(); // supported
GLvoid glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
GLvoid glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels); // supported
GLvoid glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z); // supported
GLvoid glScalef(GLfloat x, GLfloat y, GLfloat z); // supported
GLvoid glScalefv(const GLfloat *v); // supported
GLvoid glScaled(GLdouble x, GLdouble y, GLdouble z); // supported
GLvoid glScaledv(const GLdouble *v); // supported
GLvoid glScissor(GLint x, GLint y, GLsizei width, GLsizei height); // supported
GLvoid glShadeModel(GLenum mode);
GLvoid glStencilFunc(GLenum func, GLint ref, GLuint mask);
GLvoid glStencilMask(GLuint mask);
GLvoid glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
GLvoid glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
GLvoid glTexEnvi(GLenum target, GLenum pname, GLint param);
GLvoid glTexGeni(GLenum coord, GLenum pname, GLint param); // supported partially and only per vertex (GL_S,GL_T:...:GL_SPHERE_MAP,GL_SPHERE_MAP_2)
GLvoid glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels); // supported
GLvoid glTexParameteri(GLenum target, GLenum pname, GLint param); // supported
GLvoid glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels); // supported
GLvoid glTranslatef(GLfloat x, GLfloat y, GLfloat z); // supported
GLvoid glTranslatefv(const GLfloat *v); // supported
GLvoid glTranslated(GLdouble x, GLdouble y, GLdouble z); // supported
GLvoid glTranslatedv(const GLdouble *v); // supported
GLvoid glVertex2f(GLfloat x, GLfloat y); // supported
GLvoid glVertex2fv(const GLfloat *v); // supported
GLvoid glVertex3f(GLfloat x, GLfloat y, GLfloat z); // supported
GLvoid glVertex3fv(const GLfloat *v); // supported
GLvoid glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w); // supported
GLvoid glVertex4fv(const GLfloat *v); // supported
GLvoid glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
GLvoid glViewport(GLint x, GLint y, GLsizei width, GLsizei height); // supported
GLvoid glColor3d(GLdouble red, GLdouble green, GLdouble blue); // :mad: // supported
GLvoid glColor3dv(const GLdouble *v); // :mad: // supported
GLvoid glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha); // :mad: // supported
GLvoid glColor4dv(const GLdouble *v); // :mad: // supported
GLvoid glColor3f(GLfloat red, GLfloat green, GLfloat blue); // :mad: // supported
GLvoid glColor3fv(const GLfloat *v); // :mad: // supported
GLvoid glColor3ub(GLubyte red, GLubyte green, GLubyte blue); // :mad: // supported
GLvoid glColor4ubv(const GLubyte *color); // :mad: // supported
GLvoid glColor3ubv(const GLubyte *color); // :mad: // supported
GLvoid glColorMaterial(GLenum face, GLenum pname); // :mad: // supported
GLvoid glMateriali(GLenum face, GLenum pname, GLint param); // :mad: // supported
GLvoid glFrustum(float left, float right, float bottom, float top, float znear, float zfar); // :mad: // supported
GLvoid glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz); // supported
GLvoid glNormal3dv(const GLdouble *v); // supported
GLvoid glVertex2d(GLdouble x, GLdouble y); // :mad: // supported
GLvoid glVertex2dv(const GLdouble *v); // supported
GLvoid glVertex3d(GLdouble x, GLdouble y, GLdouble z); // :mad: // supported
GLvoid glVertex3dv (const GLdouble *v); // supported
GLvoid glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w); // :mad: // supported
GLvoid glVertex4dv (const GLdouble *v); // supported
GLvoid glTexCoord1d(GLdouble x); // :mad: // supported
GLvoid glTexCoord1f(GLfloat x); // :mad: // supported
GLvoid glTexCoord1dv(const GLdouble *v); // :mad: // supported
GLvoid glTexCoord1fv(const GLfloat *v); // :mad: // supported
GLvoid glTexCoord2d(GLdouble x, GLdouble y); // :mad: // supported
GLvoid glTexCoord2f(GLfloat x, GLfloat y); // :mad: // supported
GLvoid glTexCoord2dv(const GLdouble *v); // :mad: // supported
GLvoid glTexCoord2fv(const GLfloat *v); // :mad: // supported
GLvoid glTexCoord3d(GLdouble x, GLdouble y, GLdouble z); // :mad: // supported
GLvoid glTexCoord3f(GLfloat x, GLfloat y, GLfloat z); // :mad: // supported
GLvoid glTexCoord3dv(const GLdouble *v); // :mad: // supported
GLvoid glTexCoord3fv(const GLfloat *v); // :mad: // supported
GLvoid glTexCoord4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w); // :mad: // supported
GLvoid glTexCoord4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w); // :mad: // supported
GLvoid glTexCoord4dv(const GLdouble *v); // :mad: // supported
GLvoid glTexCoord4fv(const GLfloat *v); // :mad: // supported
GLvoid glPushAttrib(GLbitfield mask); // :mad: // supported (mask not supported)
GLvoid glPopAttrib(); // :mad: // supported
GLvoid glOrtho(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat znear, GLfloat zfar); // :mad: // supported
GLvoid glDeleteTextures(GLsizei n, GLuint *textures); // :mad: // supported
GLvoid glTexParameterfv(GLenum target, GLenum pname, GLfloat *param); // :mad: // supported
GLvoid glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha); // :mad: // supported
GLvoid glBlendEquation(GLenum mode); // :mad: // supported

// ------------------------
// ------------------------

GLvoid gluLookAt(GLfloat cx,GLfloat cy,GLfloat cz,GLfloat ox,GLfloat oy,GLfloat oz,GLfloat ux,GLfloat uy,GLfloat uz); // :mad: // supported
GLvoid gluPerspective(GLfloat fov, GLfloat aspect, GLfloat nearPlane, GLfloat farPlane); // :mad: // supported
GLvoid gluOrtho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top); // :mad: // supported
GLint gluProjectf(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ); // supported
GLint gluProject(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ); // supported
GLint gluUnProjectf(GLfloat winX, GLfloat winY, GLfloat winZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *objX, GLfloat *objY, GLfloat *objZ); // supported
GLint gluUnProject(GLdouble winX, GLdouble winY, GLdouble winZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *objX, GLdouble *objY, GLdouble *objZ); // supported
GLint gluInvertMatrixf(const GLfloat *m, GLfloat *dest); // supported
GLint gluInvertMatrix(const GLdouble *m, GLdouble *dest); // supported

// ------------------------
// ------------------------

// gluProject/gluUnproject seem not to work with ortho matrices, use gluProjectx for direct point transformation without strange enhancements/optimizations
GLint gluProjectfx(GLfloat objX, GLfloat objY, GLfloat objZ, GLfloat *model, GLfloat *projection, GLint *view, GLfloat *winX, GLfloat *winY, GLfloat *winZ); // supported
GLint gluProjectx(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ); // supported

// ------------------------
// ------------------------
// glEnable 0x00-0xFF
//GL_DEPTH_TEST 0x0B71 (0x71)
//GL_CULL_FACE 0x0B44 (0x44)
//GL_TEXTURE_2D 0x0DE1 (0xE1)
//GL_LIGHTING 0x0B50 (0x50)
//GL_LIGHT0 0x4000 (0x00)
//GL_LIGHT1 0x4001 (0x01)
//GL_ALPHA_TEST 0x0BC0 (0xC0)
//GL_BLEND 0x0BE2 (0xE2)
//GL_SCISSOR_TEST 0x0C11 (0x11)
//GL_COLOR_MATERIAL 0x0B57 (0x57)
//GL_FOG 0x0B64 (0x64)
//GL_TEXTURE_GEN_S 0x0c60 (0x60)
//GL_TEXTURE_GEN_T 0x0c61 (0x61)
//GL_POLYGON_OFFSET_FILL 0x8037 (0x37)
#define GL_MOUSE_BUTTON_LEFT 1
#define GL_MOUSE_BUTTON_RIGHT 2
#define GL_VK_NONE 0
#define GL_VK_ESCAPE 27
#define GL_VK_ENTER 13
#define GL_VK_UP (72*256)
#define GL_VK_PAGEUP (73*256)
#define GL_VK_LEFT (75*256)
#define GL_VK_RIGHT (77*256)
#define GL_VK_LEFT_CTRL (115*256)
#define GL_VK_RIGHT_CTRL (116*256)
#define GL_VK_DOWN (80*256)
#define GL_VK_PAGEDOWN (81*256)
#define GL_VK_POS1 (71*256)
#define GL_VK_HOME GL_VK_POS1
#define GL_VK_END (79*256)
#define GL_VK_F1 (59*256)
#define GL_VK_F2 (60*256)
#define GL_VK_F3 (61*256)
#define GL_VK_F4 (62*256)
#define GL_VK_F5 (63*256)
#define GL_VK_F6 (64*256)
#define GL_VK_F7 (65*256)
#define GL_VK_F8 (66*256)
#define GL_VK_F9 (67*256)
#define GL_VK_F10 (68*256)
#define GL_VK_F1_CTRL (94*256)
#define GL_VK_F2_CTRL (95*256)
#define GL_VK_F3_CTRL (96*256)
#define GL_VK_F4_CTRL (97*256)
#define GL_VK_F5_CTRL (98*256)
#define GL_VK_F6_CTRL (99*256)
#define GL_VK_F7_CTRL (100*256)
#define GL_VK_F8_CTRL (101*256)
#define GL_VK_F9_CTRL (102*256)
#define GL_VK_F10_CTRL (103*256)
#define GL_VK_F1_SHIFT (84*256)
#define GL_VK_F2_SHIFT (85*256)
#define GL_VK_F3_SHIFT (86*256)
#define GL_VK_F4_SHIFT (87*256)
#define GL_VK_F5_SHIFT (88*256)
#define GL_VK_F6_SHIFT (89*256)
#define GL_VK_F7_SHIFT (90*256)
#define GL_VK_F8_SHIFT (91*256)
#define GL_VK_F9_SHIFT (92*256)
#define GL_VK_F10_SHIFT (93*256)
#define GL_VK_Q_ALT (16*256)
#define GL_VK_W_ALT (17*256)
#define GL_VK_E_ALT (18*256)
#define GL_VK_R_ALT (19*256)
#define GL_VK_T_ALT (20*256)
#define GL_VK_Z_ALT (21*256)
#define GL_VK_U_ALT (22*256)
#define GL_VK_I_ALT (23*256)
#define GL_VK_O_ALT (24*256)
#define GL_VK_P_ALT (25*256)
#define GL_VK_A_ALT (30*256)
#define GL_VK_S_ALT (31*256)
#define GL_VK_D_ALT (32*256)
#define GL_VK_F_ALT (33*256)
#define GL_VK_G_ALT (34*256)
#define GL_VK_H_ALT (35*256)
#define GL_VK_J_ALT (36*256)
#define GL_VK_K_ALT (37*256)
#define GL_VK_L_ALT (38*256)
#define GL_VK_Y_ALT (44*256)
#define GL_VK_X_ALT (45*256)
#define GL_VK_C_ALT (46*256)
#define GL_VK_V_ALT (47*256)
#define GL_VK_B_ALT (48*256)
#define GL_VK_N_ALT (49*256)
#define GL_VK_M_ALT (50*256)
#define GL_VK_TAB 9
#define GL_VK_TAB_SHIFT (15*256)
#define GL_VK_ENTF (83*256)
#define GL_VK_INSERT (82*256)
#define GL_VK_DELETE GL_VK_ENTF
#define GL_VK_BACKSPACE 8
#define GL_VK_C_CTRL 3
#define GL_VK_P_CTRL 16
#define GL_VK_V_CTRL 22
#define GL_VK_S_CTRL 19
#define GL_VK_L_CTRL 12
#define GL_VK_F_CTRL 6
#define GL_VK_G_CTRL 7
#define GL_VK_U_CTRL 21
#define GL_VK_X_CTRL 24
#define GL_VK_PLUS_CTRL 29
#define GL_FOG 0x0B64 //glEnable (not overriding)
#define GL_FOG_DENSITY 0x0B62
#define GL_FOG_START  0x0B63
#define GL_FOG_END 0x0B64
#define GL_FOG_MODE 0x0B65
#define GL_FOG_COLOR 0x0B66
#define GL_EXP 0x0800
#define GL_EXP2 0x0801
#define GL_RGBA8 0
#define GL_ALL_ATTRIB_BITS 0
#define GL_TEXTURE_BASE_LEVEL  0x2804
#define GL_TEXTURE_WRAP_R  0x2805
#define GL_TEXTURE_MIN_LOD  0x2806
#define GL_TEXTURE_MAX_LOD  0x2807
#define GL_TEXTURE_LOD_BIAS 0x2808 
#define GL_TEXTURE_MAX_LEVEL 0x2809
#define GL_TEXTURE_BORDER_COLOR 0x280a
#define GL_TEXTURE_CUBE_MAP       0x0DE2 
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0x851a
#define GL_QUADS 0x0007
#define GL_GREEN 0x1904
#define GL_BLUE 0x1905
#define GL_RED 0x190b
#define GL_RG 0x190c
#define GL_BGR 0x190d
#define GL_BGRA 0x190e
#define GL_RED_INTEGER 0x190f
#define GL_RG_INTEGER 0x1910
#define GL_RGB_INTEGER 0x1911
#define GL_BGR_INTEGER 0x1912
#define GL_RGBA_INTEGER 0x1913
#define GL_BGRA_INTEGER 0x1914
#define GL_STENCIL_INDEX 0x1915
#define GL_DEPTH_COMPONENT 0x1916
#define GL_DEPTH_STENCIL 0x1917
#define GL_MIRRORED_REPEAT 0x2902
#define GL_CLAMP_TO_BORDER 0x2903
#define GL_SRC_COLOR 0x330
#define GL_ONE_MINUS_SRC_COLOR 0x331
#define GL_DST_COLOR 0x332
#define GL_ONE_MINUS_DST_COLOR 0x333
#define GL_DST_ALPHA 0x336
#define GL_ONE_MINUS_DST_ALPHA 0x337
#define GL_CONSTANT_ALPHA 0x338
#define GL_ONE_MINUS_CONSTANT_ALPHA 0x339
#define GL_CONSTANT_COLOR 0x33a
#define GL_ONE_MINUS_CONSTANT_COLOR 0x33b
#define GL_FUNC_ADD 0x105
#define GL_FUNC_SUBTRACT 0x106
#define GL_FUNC_REVERSE_SUBTRACT 0x107
#define GL_MIN 0x108
#define GL_MAX 0x109
#define GL_TRANSPOSE_MODELVIEW_MATRIX 0x0BA8
#define GL_TRANSPOSE_PROJECTION_MATRIX 0x0BA9
#define GL_COLOR_MATERIAL_FACE  0x0B55
#define GL_COLOR_MATERIAL_PARAMETER  0x0B56
#define GL_ZOOM_X 0xD16
#define GL_ZOOM_Y 0xD17
#define GL_LIGHT_MODEL_COLOR_CONTROL 0x81f8
#define GL_SINGLE_COLOR 0x81f9
#define GL_SEPARATE_SPECULAR_COLOR 0x81fa
#define GL_CONSTANT_ATTENUATION 0x1207
#define GL_LINEAR_ATTENUATION 0x1208
#define GL_QUADRATIC_ATTENUATION 0x1209
#define GL_S 0x2000
#define GL_T 0x2001
#define GL_R 0x2002
#define GL_Q 0x2003
#define GL_SPHERE_MAP 0x2402
#define GL_SPHERE_MAP_ATAN2 0x2403
#define GL_SPHERE_MAP_DUAL_PARABOLOID 0x2404
#define GL_TEXTURE_GEN_MODE 0x2500
#define GL_TEXTURE_GEN_S 0x0c60
#define GL_TEXTURE_GEN_T 0x0c61
#define GL_TEXTURE_GEN_R 0x0c62
#define GL_TEXTURE_GEN_Q 0x0c63
#define GL_LIGHT_MODEL_TWO_SIDE 0x0b52
#define GL_POINT 0x1b00
#define GL_LINE 0x1b01
#define GL_FILL 0x1b02
#define GL_SPOT_DIRECTION 0x1204
#define GL_SPOT_EXPONENT 0x1205
#define GL_SPOT_CUTOFF 0x1206
#define GL_OSC_VERSION_1_0 1
#define GL_EXT_paletted_texture 1
#define GL_OES_single_precision 1
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_FALSE 0
#define GL_TRUE 1
#define GL_POINTS 0x0000
#define GL_LINES 0x0001
#define GL_LINE_LOOP 0x0002
#define GL_LINE_STRIP 0x0003
#define GL_TRIANGLES 0x0004
#define GL_TRIANGLE_STRIP 0x0005
#define GL_TRIANGLE_FAN 0x0006
#define GL_ZERO 0
#define GL_ONE 1
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_SRC_ALPHA_SATURATE 0x0308
#define GL_SRC_ALPHA 0x0302
#define GL_FRONT 0x0404
#define GL_BACK 0x0405
#define GL_FRONT_AND_BACK 0x0408
#define GL_LIGHTING 0x0B50
#define GL_TEXTURE_2D 0x0DE1
#define GL_CULL_FACE 0x0B44
#define GL_ALPHA_TEST 0x0BC0
#define GL_BLEND 0x0BE2
#define GL_STENCIL_TEST 0x0B90
#define GL_DEPTH_TEST 0x0B71
#define GL_LIGHT0 0x4000
#define GL_LIGHT1 0x4001
#define GL_POINT_SMOOTH 0x0B10
#define GL_LINE_STIPPLE 0x0B24
#define GL_LINE_SMOOTH 0x0B20
#define GL_SCISSOR_TEST 0x0C11
#define GL_COLOR_MATERIAL 0x0B57
#define GL_NORMALIZE 0x0BA1
#define GL_RESCALE_NORMAL 0x803A
#define GL_POLYGON_OFFSET_FILL 0x8037
#define GL_POLYGON_STIPPLE 0x0B42
#define GL_VERTEX_ARRAY 0x8074
#define GL_NORMAL_ARRAY 0x8075
#define GL_COLOR_ARRAY 0x8076
#define GL_TEXTURE_COORD_ARRAY 0x8078
#define GL_NO_ERROR 0
#define GL_INVALID_ENUM 0x0500
#define GL_INVALID_VALUE 0x0501
#define GL_INVALID_OPERATION 0x0502
#define GL_STACK_OVERFLOW 0x0503
#define GL_STACK_UNDERFLOW 0x0504
#define GL_OUT_OF_MEMORY 0x0505
#define GL_CW 0x0900
#define GL_CCW 0x0901
#define GL_DEPTH_WRITEMASK 0x0B72
#define GL_COLOR_WRITEMASK 0x0C23
#define GL_CURRENT_COLOR 0x0B00
#define GL_CURRENT_NORMAL 0x0B02
#define GL_CURRENT_TEXTURE_COORDS 0x0B03
#define GL_CURRENT_RASTER_COLOR 0x0B04
#define GL_CURRENT_RASTER_TEXTURE_COORDS 0x0B06
#define GL_POINT_SIZE 0x0B11
#define GL_SMOOTH_POINT_SIZE_RANGE 0x0B12
#define GL_SMOOTH_POINT_SIZE_GRANULARITY 0x0B13
#define GL_LINE_WIDTH 0x0B21
#define GL_SMOOTH_LINE_WIDTH_RANGE 0x0B22
#define GL_SMOOTH_LINE_WIDTH_GRANULARITY 0x0B23
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_DEPTH_RANGE 0x0B70
#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_ALPHA_TEST_REF 0x0BC2
#define GL_COLOR_CLEAR_VALUE 0x0C22
#define GL_POLYGON_OFFSET_UNITS 0x2A00
#define GL_POLYGON_OFFSET_FACTOR 0x8038
#define GL_ALIASED_POINT_SIZE_RANGE 0x846D
#define GL_ALIASED_LINE_WIDTH_RANGE 0x846E
#define GL_MATRIX_MODE 0x0BA0
#define GL_VIEWPORT 0x0BA2
#define GL_MODELVIEW_STACK_DEPTH 0x0BA3
#define GL_PROJECTION_STACK_DEPTH 0x0BA4
#define GL_MODELVIEW_MATRIX 0x0BA6
#define GL_PROJECTION_MATRIX 0x0BA7
#define GL_LINE_STIPPLE_PATTERN 0x0B25
#define GL_LINE_STIPPLE_REPEAT 0x0B26
#define GL_MAX_LIST_NESTING 0x0B31
#define GL_LIST_BASE 0x0B32
#define GL_CULL_FACE_MODE 0x0B45
#define GL_FRONT_FACE 0x0B46
#define GL_DEPTH_FUNC 0x0B74
#define GL_STENCIL_CLEAR_VALUE 0x0B91
#define GL_STENCIL_FUNC 0x0B92
#define GL_STENCIL_VALUE_MASK 0x0B93
#define GL_STENCIL_FAIL 0x0B94
#define GL_STENCIL_PASS_DEPTH_FAIL 0x0B95
#define GL_STENCIL_PASS_DEPTH_PASS 0x0B96
#define GL_STENCIL_REF 0x0B97
#define GL_STENCIL_WRITEMASK 0x0B98
#define GL_ALPHA_TEST_FUNC 0x0BC1
#define GL_BLEND_DST 0x0BE0
#define GL_BLEND_SRC 0x0BE1
#define GL_SCISSOR_BOX 0x0C10
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT 0x0C51
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_POLYGON_SMOOTH_HINT 0x0C53
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_MAX_LIGHTS 0x0D31
#define GL_MAX_TEXTURE_SIZE 0x0D33
#define GL_MAX_MODELVIEW_STACK_DEPTH 0x0D36
#define GL_MAX_PROJECTION_STACK_DEPTH 0x0D38
#define GL_MAX_VIEWPORT_DIMS 0x0D3A
#define GL_SUBPIXEL_BITS 0x0D50
#define GL_RED_BITS 0x0D52
#define GL_GREEN_BITS 0x0D53
#define GL_BLUE_BITS 0x0D54
#define GL_ALPHA_BITS 0x0D55
#define GL_DEPTH_BITS 0x0D56
#define GL_STENCIL_BITS 0x0D57
#define GL_VERTEX_ARRAY_SIZE 0x807A 
#define GL_VERTEX_ARRAY_TYPE 0x807B 
#define GL_VERTEX_ARRAY_STRIDE 0x807C 
#define GL_NORMAL_ARRAY_TYPE 0x807E 
#define GL_NORMAL_ARRAY_STRIDE 0x807F 
#define GL_COLOR_ARRAY_SIZE 0x8081 
#define GL_COLOR_ARRAY_TYPE 0x8082 
#define GL_COLOR_ARRAY_STRIDE 0x8083 
#define GL_TEXTURE_COORD_ARRAY_SIZE 0x8088 
#define GL_TEXTURE_COORD_ARRAY_TYPEM 0x8089 
#define GL_TEXTURE_COORD_ARRAY_STRIDE 0x808A 
#define GL_SHADE_MODEL 0x0B54
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_MAX_ELEMENTS_VERTICES 0x80E8
#define GL_MAX_ELEMENTS_INDICES 0x80E9
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_CLIENT_ACTIVE_TEXTURE 0x84E1
#define GL_MAX_TEXTURE_UNITS 0x84E2
#define GL_VERTEX_ARRAY_POINTER 0x808E
#define GL_NORMAL_ARRAY_POINTER 0x808F
#define GL_COLOR_ARRAY_POINTER 0x8090
#define GL_TEXTURE_COORD_ARRAY_POINTER  0x8092
#define GL_DONT_CARE 0x1100
#define GL_FASTEST 0x1101
#define GL_NICEST 0x1102
#define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
#define GL_POINT_SMOOTH_HINT 0x0C51
#define GL_LINE_SMOOTH_HINT 0x0C52
#define GL_LIGHT_MODEL_AMBIENT 0x0B53
#define GL_AMBIENT 0x1200
#define GL_DIFFUSE 0x1201
#define GL_SPECULAR 0x1202
#define GL_POSITION 0x1203
#define GL_COMPILE 0x1300
#define GL_BYTE 0x1400
#define GL_UNSIGNED_BYTE 0x1401
#define GL_INT 0x1404
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_EMISSION 0x1600
#define GL_SHININESS 0x1601
#define GL_AMBIENT_AND_DIFFUSE 0x1602
#define GL_MODELVIEW 0x1700
#define GL_PROJECTION 0x1701
#define GL_ALPHA 0x1906
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_LUMINANCE 0x1909
#define GL_LUMINANCE_ALPHA 0x190A
#define GL_COLOR_INDEX 0x1900
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_COLOR 0x1800
#define GL_FLAT 0x1D00
#define GL_SMOOTH 0x1D01
#define GL_NEVER 0x0200 
#define GL_LESS 0x0201
#define GL_EQUAL 0x0202
#define GL_LEQUAL 0x0203
#define GL_GREATER 0x0204
#define GL_NOTEQUAL 0x0205
#define GL_GEQUAL 0x0206
#define GL_ALWAYS 0x0207
#define GL_KEEP 0x1E00
#define GL_REPLACE 0x1E01
#define GL_INCR 0x1E02
#define GL_DECR 0x1E03
#define GL_INVERT 0x150A
#define GL_VENDOR 0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_EXTENSIONS 0x1F03
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
#define GL_ADD 0x0104
#define GL_TEXTURE_ENV_MODE 0x2200
#define GL_TEXTURE_ENV_COLOR 0x2201
#define GL_TEXTURE_ENV 0x2300
#define GL_NEAREST 0x2600
#define GL_LINEAR 0x2601
#define GL_NEAREST_MIPMAP_NEAREST 0x2700
#define GL_LINEAR_MIPMAP_NEAREST 0x2701
#define GL_NEAREST_MIPMAP_LINEAR 0x2702
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2
#define GL_TEXTURE3 0x84C3
#define GL_TEXTURE4 0x84C4
#define GL_TEXTURE5 0x84C5
#define GL_TEXTURE6 0x84C6
#define GL_TEXTURE7 0x84C7
#define GL_TEXTURE8 0x84C8
#define GL_TEXTURE9 0x84C9
#define GL_TEXTURE10 0x84CA
#define GL_TEXTURE11 0x84CB
#define GL_TEXTURE12 0x84CC
#define GL_TEXTURE13 0x84CD
#define GL_TEXTURE14 0x84CE
#define GL_TEXTURE15 0x84CF
#define GL_TEXTURE16 0x84D0
#define GL_TEXTURE17 0x84D1
#define GL_TEXTURE18 0x84D2
#define GL_TEXTURE19 0x84D3
#define GL_TEXTURE20 0x84D4
#define GL_TEXTURE21 0x84D5
#define GL_TEXTURE22 0x84D6
#define GL_TEXTURE23 0x84D7
#define GL_TEXTURE24 0x84D8
#define GL_TEXTURE25 0x84D9
#define GL_TEXTURE26 0x84DA
#define GL_TEXTURE27 0x84DB
#define GL_TEXTURE28 0x84DC
#define GL_TEXTURE29 0x84DD
#define GL_TEXTURE30 0x84DE
#define GL_TEXTURE31 0x84DF
#define GL_REPEAT 0x2901
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_COLOR_INDEX8_EXT 0x80E5
#define GL_COLOR_TABLE_FORMAT_EXT 0x80D8
#define GL_COLOR_TABLE_WIDTH_EXT 0x80D9
#define GL_COLOR_TABLE_RED_SIZE_EXT 0x80DA
#define GL_COLOR_TABLE_GREEN_SIZE_EXT 0x80DB
#define GL_COLOR_TABLE_BLUE_SIZE_EXT 0x80DC
#define GL_COLOR_TABLE_ALPHA_SIZE_EXT 0x80DD
#define GL_COLOR_TABLE_LUMINANCE_SIZE_EXT 0x80DE
#define GL_COLOR_TABLE_INTENSITY_SIZE_EXT 0x80DF

// ------------------------
// ------------------------

// better use glTexImage2D()
GLvoid glTexturePointer(GLint width, GLint height, GLuint *textureData);  // glBindTexture() must be called before
GLuint *glGetTexturePointer(GLuint textureId);

// ------------------------
// ------------------------

// Maybe you don't need that..
// actually just for GL_POINTS and GL_LINES (to have propper (and quadratic) width/height on a non 4:3 screen)
// It's poorly implemented. The best way is to specify the real monitors aspect ratio e.g. 16.0/9.0 at gluPerspective..
GLvoid glSetMonitorAspectRatio(GLfloat aspect); // e.g. 16.0/9.0 (default 1.0, maybe use only if you draw pointssprites, non standard stuff) modifies zoomX and zoomY
GLdouble glGetMonitorAspectRatio(); // hopefully not needed, maybe you can make all with glFrustum/glOrtho and so on without the extra aspect
GLvoid glZoomX(GLfloat zoom); // also non standard, hopefully you don't need it
GLvoid glZoomY(GLfloat zoom); // also non standard, hopefully you don't need it
GLfloat glGetZoomX(); // also non standard, hopefully you don't need it
GLfloat glGetZoomY(); // also non standard, hopefully you don't need it

// ------------------------
// ------------------------

#define GLMAXTEXTURES 1024
#define GLMAXTEXTUREUNITS 8
#define GLMAXLIGHTS 2

// ------------------------
// ------------------------

#ifdef WATCOMGLAPI

// ------------------------
// ------------------------

typedef struct _GLContext {
  GLboolean enabledCaps[256];
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
  GLfloat alphaFuncRef;
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
  GLdouble matrixForMode[2][4*4];
  GLdouble inverseMatrixForMode[2][4*4]; // use glGetInverseModelView
  GLdouble matrix[4*4];
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
  GLdouble normalX;
  GLdouble normalY;
  GLdouble normalZ;
  GLdouble vertexX;
  GLdouble vertexY;
  GLdouble vertexZ;
  GLdouble vertexW;
  GLdouble textureX;
  GLdouble textureY;
  GLdouble textureZ;
  GLdouble textureW;
  GLenum beginMode;
  GLdouble lightRed[8][GLMAXLIGHTS];
  GLdouble lightGreen[8][GLMAXLIGHTS];
  GLdouble lightBlue[8][GLMAXLIGHTS];
  GLdouble lightAlpha[8][GLMAXLIGHTS];
  GLfloat constantAttenuation[GLMAXLIGHTS];
  GLfloat linearAttenuation[GLMAXLIGHTS];
  GLfloat quadraticAttenuation[GLMAXLIGHTS];
  GLfloat spotExponent[GLMAXLIGHTS];
  GLfloat spotCutOff[GLMAXLIGHTS];
  GLenum colorMaterial[2];
  GLenum colorMaterialFace;
  GLfloat materialRed[2][5];
  GLfloat materialGreen[2][5];
  GLfloat materialBlue[2][5];
  GLfloat materialAlpha[2][5];
  GLuint boundTextures[GLMAXTEXTUREUNITS];
  GLfloat blendColorRed;
  GLfloat blendColorGreen;
  GLfloat blendColorBlue;
  GLfloat blendColorAlpha;
  GLenum blendEquation;
  GLenum frontFace;
  GLboolean maskRed;
  GLboolean maskGreen;
  GLboolean maskBlue;
  GLboolean maskAlpha;
  GLint forceNoCull;
  GLdouble zoomX;
  GLdouble zoomY;
  GLuint pushAttribBitsHere;
  GLfloat explicitAlpha;
  GLboolean useExplicitAlpha;
  GLboolean separateSpecular;
  GLenum texGenS;
  GLenum texGenT;
  GLboolean needNewInverseModelView;
  GLfloat fogStart;
  GLfloat fogEnd;
  GLfloat fogColor[4];
  GLfloat fogDensity;
  GLenum fogMode;
  GLboolean twoSidedLighting;
  GLboolean wireframe[2];
} _GLContext;

typedef struct glVertex {
  GLfloat colorRed;
  GLfloat colorGreen;
  GLfloat colorBlue;
  GLfloat colorAlpha;
  GLfloat additionalSpecularColorRed; // extra pass for specular (GL_SEPARATE_SPECULAR_COLOR)
  GLfloat additionalSpecularColorGreen;
  GLfloat additionalSpecularColorBlue;
  GLdouble normalX;
  GLdouble normalY;
  GLdouble normalZ;
  GLdouble vertexX;
  GLdouble vertexY;
  GLdouble vertexZ;
  GLdouble vertexW;
  GLdouble textureX;
  GLdouble textureY;
  GLdouble textureZ;
  GLdouble textureW;
  GLdouble sx,sy,sz,sw; // screen x,y,z and w (y is top to bottom) z for zbuffer and w for perspective correct interpolation // just nearplane clipping was performed internally
} glVertex;

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

extern _GLContext glContext;
extern glTexture glTextures[GLMAXTEXTURES];

// ------------------------
// ------------------------

 // you can implement your own triangle renderer here
typedef GLvoid (*TriangleDrawer)(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2);
GLvoid glDrawTrianglePrecise(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2);
GLvoid glDrawTriangleNone(_GLContext *context, glVertex *v0, glVertex *v1, glVertex *v2);
GLvoid glSetTriangleDrawer(TriangleDrawer drawer);

// ------------------------
// ------------------------

#endif // WATCOMGLAPI

// ------------------------
// ------------------------
#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __GL_H__

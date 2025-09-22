#include "gl.h"
#include <stdlib.h>
#include <string.h>
#include <dos.h>
#include <go32.h>
#include <dpmi.h>
#include <sys/nearptr.h>
#include <sys/farptr.h>
#include <stdint.h>

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

double glPixelCenterX = -0.5;
double glPixelCenterY = -0.5;
double glTexelCenterX = -0.5;
double glTexelCenterY = -0.5;
unsigned int *glFrameBufferDedicated = NULL; // linear framebuffer
int glFrameBufferWidth = 0;
int glFrameBufferHeight = 0;
int glFrameBufferBytesPerPixel = 4;
unsigned int *glFrameBuffer = NULL;
float *glDepthBuffer = NULL;
bool useNearPointers = true;
int glFrameBufferWidth0 = 0;
int glFrameBufferHeight0 = 0;
unsigned int *glFrameBuffer0 = NULL;
float *glDepthBuffer0 = NULL;
double identityMatrix[4*4] = {1,0,0,0 ,0,1,0,0 ,0,0,1,0 ,0,0,0,1};
bool currentBackFacing = false;

OSMesaContext ctx;

extern unsigned short glGraphicsModeToRestore;
unsigned short getBiosGraphicsMode();
void glSetupMouse();

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
  uint16_t *w=new uint16_t[(ofs-videoModes)/2];
  uint16_t *v=w;
  dosmemget(videoModes,ofs-videoModes,w);

  while(*v != 0xffff) {
    int mode = *v;
    memset(&regs,0,sizeof(__dpmi_regs));
    regs.x.ax = 0x4f01;
    regs.x.cx = mode;
    VBE_callESDI(&regs,&modeInfo,sizeof(glVbeModeInfo));
    if (modeInfo.width == xRes && modeInfo.height == yRes && modeInfo.bpp == bPP) {
      memset(&regs,0,sizeof(__dpmi_regs));
      regs.x.ax = 0x4f02;
      regs.x.bx = mode|0x4000;
      __dpmi_simulate_real_mode_interrupt(0x10, &regs); // needed here for later mouse setup
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
      glFrameBuffer = new unsigned int[glFrameBufferWidth*glFrameBufferHeight];
      glDepthBuffer = new float[glFrameBufferWidth*glFrameBufferHeight];
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
      delete[] w;
      ctx = OSMesaCreateContext(OSMESA_RGBA,NULL);
      OSMesaMakeCurrent(ctx,glFrameBuffer, GL_UNSIGNED_BYTE,xRes,yRes);
      return true;
    }
    v++;
  }
  delete[] w;
  return false;
}

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

bool glVGA() {
  return true;
}

bool glDirect(unsigned int *frameBuffer, float *depthBuffer, unsigned int width, unsigned int height) {
  return true;
}

void glRefresh() {
  unsigned int rgba;
  int i = 0;
  for (int y = 0; y < glFrameBufferHeight0; y++) {
    int i2 = (glFrameBufferHeight0-y-1)*glFrameBufferWidth0;
    for (int x = 0; x < glFrameBufferWidth0; x++) {
      rgba = glFrameBuffer0[i2++];
      _farpokel(vesasel,i<<2,(rgba & 0xff00ff00)|((rgba>>16)&0xff)|((rgba<<16)&0x00ff0000));
      i++;
    }
  }
}

void glDone() {
  OSMesaDestroyContext(ctx);
  if (glFrameBuffer!=NULL) delete[] glFrameBuffer;
  setBiosGraphicsMode(glGraphicsModeToRestore);
}

void glSetRenderTarget(unsigned int *frameBufferOrNULL, float *depthBuffer, unsigned int width, unsigned int height) {
}

unsigned int glGetTextureWidth(unsigned int textureId) {
  return 32;
}
unsigned int glGetTextureHeight(unsigned int textureId) {
  return 32;
}

void glExplicitAlpha(bool useExplicitAlpha, float alpha) {
}

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
  regs.w.ax = 0x04;
  regs.w.cx = x;
  regs.w.dx = y;
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
  if (true) {
    glSetMousePos(glFrameBufferWidth0/2, glFrameBufferHeight0/2);
    // update lastMouseX this way
    glMouseX = glFrameBufferWidth0/2;
    glMouseY = glFrameBufferHeight0/2;
  }
}


char glMouseButtons() {
  union REGS regs;
  regs.w.ax = 0x03;
  int386(0x33, &regs, &regs);
  glLastMouseB = glMouseB;
  glMouseB = regs.w.bx & 0xffff;
  return glMouseB & 0xff;
}

void glSetupMouse() {
    union REGS regs;
    regs.w.ax = 0x07;
    regs.w.cx = 0;
    regs.w.dx = glFrameBufferWidth0-1;
    int386(0x33, &regs, &regs);
    regs.w.ax = 0x08;
    regs.w.cx = 0;
    regs.w.dx = glFrameBufferHeight0-1;
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

void glTexturePointer(int width, int height, unsigned int *textureData) {
}

GLuint *glGetTexturePointer(GLuint textureId) {
  return NULL;
}

void glSetMonitorAspectRatio(float aspect) {
}

double glGetMonitorAspectRatio() {
  return 0.0;
}

static void MulMatVec(double *dest, double *m, double *v) {
  double x = v[0] * m[0*4+0] + v[1] * m[1*4+0] + v[2] * m[2*4+0] + v[3] * m[3*4+0];
  double y = v[0] * m[0*4+1] + v[1] * m[1*4+1] + v[2] * m[2*4+1] + v[3] * m[3*4+1];
  double z = v[0] * m[0*4+2] + v[1] * m[1*4+2] + v[2] * m[2*4+2] + v[3] * m[3*4+2];
  double w = v[0] * m[0*4+3] + v[1] * m[1*4+3] + v[2] * m[2*4+3] + v[3] * m[3*4+3];
  dest[0] = x;
  dest[1] = y;
  dest[2] = z;
  dest[3] = w;
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
  MulMatVec(in, modelview, in);
  MulMatVec(in, projection, in);
  if (in[3]==0) return 0;
  in[0]/=in[3];
  in[1]/=in[3];
  in[2]/=in[3];
  in[3] = 1;
  in[0] *= viewport[2]*0.5; //*glContext.zoomX;
  in[1] *= viewport[3]*-0.5; //*glContext.zoomY;
  in[0] += viewport[0]+viewport[2]*0.5;
  in[1] += viewport[1]+viewport[3]*0.5;
  windowCoordinate[0] = in[0];
  windowCoordinate[1] = in[1];
  windowCoordinate[2] = in[2];
  return 1;
}

GLint gluProjectx(GLdouble objX, GLdouble objY, GLdouble objZ, GLdouble *model, GLdouble *projection, GLint *view, GLdouble *winX, GLdouble *winY, GLdouble *winZ) {
  double d[3];
  const GLint r = glProjectx(objX, objY, objZ, model, projection, view, d);
  *winX = d[0]+glPixelCenterX; // actually you need the pixelCenter?
  *winY = d[1]-glPixelCenterY; // actually you need the pixelCenter?
  *winZ = d[2];
  return r;
}

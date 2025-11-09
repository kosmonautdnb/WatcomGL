// --
// WatSDL
// this is not the original SDL (SDL2) the original SDL can be found at https://github.com/libsdl-org/SDL
// --
// Github: https://github.com/kosmonautdnb/WatSDL
// by Stefan Mader in 2025
// --
// it's just a first attempt to get the game OpenTyrian running on FreeDOS with WatcomC++ 11
// you don't need other SDL header files just include this
// and maybe this will evolve further from a Tyrian implementation wrapper to a more usable SDL
// --
#include "sdl.h"
#include "gl.h"
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include <i86.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void SDL_SetBiosCursor(int x, int y) {
    union REGS r;
    memset(&r,0,sizeof(r));
    r.x.eax = 0x200;
    r.h.bh = 0;
    r.h.dl = (unsigned char)(x&255);
    r.h.dh = (unsigned char)(y&255);
    int386(0x10, &r, &r);
}

#define SDL_DPMIERROR {glDone(); printf("SDLError: No appropiate DPMI service.\n Maybe use PModeW at linking.\n");exit(0);}

#if defined(SDL_USETIMERIRQ) && (!defined(__DJGPP__))
static volatile double timerSeconds = 0;
static volatile int timerIrqPos = 0;
typedef void (__interrupt __far* TimerIrqHandler)();
static TimerIrqHandler oldTimerIrqHandler = NULL;
#define HARDWARE_TIMER_FREQUENCY 1193180 // 1193180 / 65536  = 18.2 (found in the internet that it's more like 1193181.66666666 instead of 1193180)
#define TIMER_MULTIPLICATOR 1024 // 1193180 * 1024 / 65536
static void (__interrupt __far timerIrqHandler) (void)
{
  timerSeconds += (double)1.0/((double)HARDWARE_TIMER_FREQUENCY * TIMER_MULTIPLICATOR / 65536.0);
  timerIrqPos++;
  if (timerIrqPos>=TIMER_MULTIPLICATOR) {
    timerIrqPos = 0;
    oldTimerIrqHandler();
  } else {
    outp(0x20,0x20);
  }
}

static void uninstallTimerIrq() {
  if (oldTimerIrqHandler != NULL) {
    union REGS r;
    memset(&r,0,sizeof(r));
    outp(0x43,0x36);
    outp(0x40,0x00);
    outp(0x40,0x00);
    r.x.eax = 0x205;
    r.h.bl = 0x08;
    r.x.ecx = FP_SEG(oldTimerIrqHandler);
    r.x.edx = FP_OFF(oldTimerIrqHandler);
    int386 (0x31, &r, &r);
    if (r.w.cflag) SDL_DPMIERROR;
    oldTimerIrqHandler = NULL;
  }
}

static void installTimerIrq() {
  if (oldTimerIrqHandler == NULL) {
    timerSeconds=0;
    timerIrqPos=0;
    union REGS r;
    memset(&r,0,sizeof(r));
    r.x.eax = 0x204;
    r.h.bl = 0x08;
    int386 (0x31, &r, &r);
    oldTimerIrqHandler = (TimerIrqHandler)MK_FP(r.x.ecx,r.x.edx);
    int here = 65536/TIMER_MULTIPLICATOR;
    r.x.eax = 0x205;
    r.h.bl = 0x08;
    r.x.ecx = FP_SEG(timerIrqHandler);
    r.x.edx = FP_OFF(timerIrqHandler);
    int386 (0x31, &r, &r);
    if (r.w.cflag) SDL_DPMIERROR;
    outp(0x43,0x36);
    outp(0x40,here & 255);
    outp(0x40,(here>>8) & 255);
    atexit(uninstallTimerIrq);
  }
}

void SDL_SetSeconds(double s) {timerSeconds = s;}
double SDL_GetSeconds() {return timerSeconds;}

#else // SDL_USETIMERIRQ
static void installTimerIrq() {;}
static void uninstallTimerIrq() {;}
void SDL_SetSeconds(double s) {glSetTime(s);}
double SDL_GetSeconds() {return glSeconds();}
#endif // SDL_USETIMERIRQ

typedef void (__interrupt __far* KeyboardHandler)();
static KeyboardHandler oldKeyboardHandler = NULL;

Uint8 volatile keyPressed[0x100] = {0};

Uint8 isKeyPressed(int scanCode) {
  return keyPressed[scanCode & 0x7f];
}

static void (__interrupt __far keyboardHandler) (void) {
  unsigned char key = (unsigned char)inp(0x60);
  keyPressed[key & 0x7f] = key & 0x80 ? false : true;
  oldKeyboardHandler();
}

void uninstallKeyboardHandler() {
  if (oldKeyboardHandler != NULL) {
    union REGS r;
    memset(&r,0,sizeof(r));
    r.w.ax = 0x205;
    r.h.bl = 0x09;
    r.x.ecx = FP_SEG(oldKeyboardHandler);
    r.x.edx = FP_OFF(oldKeyboardHandler);
    int386 (0x31, &r, &r);
    if (r.w.cflag) SDL_DPMIERROR;
    oldKeyboardHandler = NULL;
  }
}

void installKeyboardHandler() {
  if (oldKeyboardHandler != NULL)
    uninstallKeyboardHandler();
  union REGS r;
  memset(&r,0,sizeof(r));
  r.w.ax = 0x204;
  r.h.bl = 0x09;
  int386 (0x31, &r, &r);
  oldKeyboardHandler = (KeyboardHandler)MK_FP(r.x.ecx,r.x.edx);
  memset(&r,0,sizeof(r));
  r.w.ax = 0x205;
  r.h.bl = 0x09;
  r.x.ecx = FP_SEG(keyboardHandler);
  r.x.edx = FP_OFF(keyboardHandler);
  int386 (0x31, &r, &r);
  if (r.w.cflag) SDL_DPMIERROR;
  atexit(uninstallKeyboardHandler);
}

// ------------ DOS Long FileName Handling ---------------

#define SDL_DosErrorCode errno

typedef struct {
  Uint16 selector; // for protected mode
  Uint16 segment; // for 16 bit dos mode (offset is 0)
  void *ptr; // flat mode memory to write to
} SDL_DOSPOINTER;

typedef struct {
  Uint16 di;
  Uint16 si;
  Uint16 bp;
  Uint16 reserved;
  Uint16 bx;
  Uint16 dx;
  Uint16 cx;
  Uint16 ax;
  Uint16 es,ds,fs,gs,ip,cs,sp,ss;
  bool cflag;
} SDL_REGS16;

SDL_DOSPOINTER SDL_DosNullPointer() {
  SDL_DOSPOINTER k;
  memset(&k,0,sizeof(k));
  k.selector = 0;
  k.segment = 0;
  k.ptr = NULL;
  return k;
}

SDL_DOSPOINTER SDL_MakeDosPointer(Uint16 selector, Uint16 segment) {
  SDL_DOSPOINTER k;
  memset(&k,0,sizeof(k));
  k.selector = selector;
  k.segment = segment;
  k.ptr = (void*)((unsigned int)segment * 16);
  return k;
}

SDL_DOSPOINTER SDL_DosMalloc(Uint32 size) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x100;
  regs.w.bx = (size+15)/16;
  regs.w.cflag = false;
  int386(0x31, &regs, &regs);
  if (regs.w.cflag) {
    SDL_DosErrorCode = regs.w.ax;
    return SDL_DosNullPointer();
  }
  return SDL_MakeDosPointer(regs.w.dx,regs.w.ax);
}

bool SDL_DosFree(const SDL_DOSPOINTER &mem) {
  union REGS regs;
  memset(&regs,0,sizeof(regs));
  regs.w.ax = 0x101;
  regs.w.dx = mem.selector;
  regs.w.cflag = false;
  int386(0x31, &regs, &regs);
  if (regs.w.cflag) {
    SDL_DosErrorCode = regs.w.ax;
    return false;
  }
  return true;
}

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
} SDL_RMREGS16;

Uint16 SDL_DosInt386x(Uint32 intno, SDL_REGS16 *in, SDL_REGS16 *out) {
  SDL_RMREGS16 rmregs;
  union REGS r;
  struct SREGS sr;
  memset(&rmregs,0,sizeof(rmregs));
  memset(&r,0,sizeof(r));
  memset(&sr,0,sizeof(sr));
  segread(&sr);
  sr.es = sr.ds; // flat model?
  rmregs.eax = in->ax; 
  rmregs.ebx = in->bx; 
  rmregs.ecx = in->cx; 
  rmregs.edx = in->dx; 
  rmregs.esi = in->si; 
  rmregs.edi = in->di; 
  rmregs.ds = in->ds;
  rmregs.es = in->es;
  rmregs.flags = in->cflag ? 0x01 : 0x00;
  r.w.ax = 0x300;
  r.h.bl = intno;
  r.h.bh = 0;
  r.w.cx = 0;
  r.x.edi = (unsigned)&rmregs;
  int386x(0x31,&r,&r,&sr);
  out->ax = rmregs.eax; 
  out->bx = rmregs.ebx; 
  out->cx = rmregs.ecx; 
  out->dx = rmregs.edx; 
  out->si = rmregs.esi; 
  out->di = rmregs.edi; 
  out->es = rmregs.es;
  out->cs = rmregs.cs;
  out->ss = rmregs.ss;
  out->ds = rmregs.ds;
  out->cflag = (rmregs.flags & 0x01) ? true : false;
  return out->ax;
}

bool SDL_Path(char *name, const char *fileName, bool longPath) {
  SDL_DOSPOINTER dosInterfaceMemory1 = SDL_DosMalloc(strlen(fileName)+1);  if (dosInterfaceMemory1.ptr == NULL) return false; memcpy((char*)dosInterfaceMemory1.ptr,fileName,strlen(fileName)+1);
  SDL_DOSPOINTER dosInterfaceMemory2 = SDL_DosMalloc(SDL_MAX_LFN_PATH);  if (dosInterfaceMemory2.ptr == NULL) {SDL_DosFree(dosInterfaceMemory1);  return false; }
  SDL_REGS16 regs;
  memset(&regs,0,sizeof(regs));
  regs.ax = 0x7160;
  regs.cx = (longPath ? 2 : 1)+0x100;
  regs.ds = dosInterfaceMemory1.segment;
  regs.si = 0;
  regs.es = dosInterfaceMemory2.segment;
  regs.di = 0;
  SDL_DosInt386x(0x21, &regs, &regs);
  if (regs.cflag) {
    SDL_DosFree(dosInterfaceMemory1);
    SDL_DosFree(dosInterfaceMemory2);
    SDL_DosErrorCode = regs.ax;
    return false;
  }
  memcpy(name,(char*)dosInterfaceMemory2.ptr,strlen((char*)dosInterfaceMemory2.ptr)+1);
  SDL_DosFree(dosInterfaceMemory1);
  SDL_DosFree(dosInterfaceMemory2);
  return true;
}

bool SDL_LongPath(char *destName, const char *fileName) {
  memcpy(destName,fileName,strlen(fileName)+1);
  return SDL_Path(destName, fileName, true);
}

bool SDL_ShortPath(char *destName, const char *fileName) {
  memcpy(destName,fileName,strlen(fileName)+1);
  return SDL_Path(destName, fileName,false);
}
// -------------------------------------------------------

#define ERROR(__text__) {glDone();printf(__text__);exit(0);}
#define RDELETE(__a__) {if ((__a__)!=NULL) {free(__a__);__a__=NULL;}}
#define RALLOC(__type__,__var__) __type__ *__var__ = (__type__*)malloc(sizeof(__type__));memset(__var__,0,sizeof(__type__));
#define _RALLOC(__type__,__size__) (__type__*)malloc(sizeof(__type__)*(__size__));

unsigned int sdl_initialized = 0;

SDL_Result SDL_GL_SetAttribute(SDL_GLattr attr, int value) {
  return SDL_ERROR;
}

SDL_Result SDL_GL_GetAttribute(SDL_GLattr attr, int *value) {
  return SDL_ERROR;
}

const char *SDL_GetDisplayName(int displayIndex) {
  return "WatcomGL Display";
}

SDL_JoystickID SDL_JoystickInstanceID(SDL_Joystick *joystick) {
  return 0;
}

void *SDL_GL_GetProcAddress(const char *proc) {
  return NULL;
}

SDL_GLContext SDL_GL_CreateContext(SDL_Window *window) {
  RALLOC(void*,r);
  return (SDL_GLContext)r;
}

void SDL_GL_DeleteContext(SDL_GLContext context) {
  RDELETE(context);
}

void SDL_GL_SwapWindow(SDL_Window *window) {
  glRefresh();
}

void SDL_SetWindowGrab(SDL_Window *window, bool grabbed) {
}

SDL_Result SDL_GL_SetSwapInterval(int interval) {
  return SDL_OK;
}

SDL_Surface *SDL_ConvertSurface(SDL_Surface *src, const SDL_PixelFormat *fmt, Uint32 flags) {
  return NULL;
}

SDL_Finger *SDL_GetTouchFinger(SDL_TouchID touchID, int index) {
  return NULL;
}

SDL_Result SDL_RenderSetIntegerScale(SDL_Renderer *renderer, bool enable) {
  return SDL_OK;
}

bool SDL_IsGameController(int joystick_index) {
  return false;
}

SDL_Haptic *SDL_HapticOpen(int device_index) {
  return NULL;
}

void SDL_HapticClose(SDL_Haptic *haptic) {
}

SDL_Result SDL_HapticRumbleInit(SDL_Haptic *haptic) {
  return SDL_ERROR;
}

bool SDL_GameControllerGetAttached(SDL_GameController *gamecontroller) {
  return false;
}

SDL_GameControllerType SDL_GameControllerTypeForIndex(int joystick_index) {
  return SDL_CONTROLLER_TYPE_UNKNOWN;
}

SDL_Result SDL_GameControllerRumble(SDL_GameController *gamecontroller, Uint16 low_frequency_rumble, Uint16 high_frequency_rumble, Uint32 duration_ms) {
  return SDL_ERROR;
}

int SDL_GetNumTouchFingers(SDL_TouchID touchID) {
  return 0;
}

SDL_Result SDL_LockSurface(SDL_Surface *surface) {
  return SDL_OK;
}

void SDL_UnlockSurface(SDL_Surface *surface) {
}

void SDL_GL_GetDrawableSize(SDL_Window *window, int *w, int *h) {
  if (w != NULL) *w = window->w; 
  if (h != NULL) *h = window->h;
}

bool SDL_IsTextInputActive() {
  return false;
}

void SDL_StartTextInput() {
}

void SDL_StopTextInput() {
}

void SDL_RenderGetScale(SDL_Renderer *renderer, float *scaleX, float *scaleY) {
  if (scaleX != NULL) *scaleX = renderer->scale.x; 
  if (scaleY != NULL) *scaleY = renderer->scale.y;
}

void SDL_SetWindowTitle(SDL_Window *window, const char *title) {
  window->title = title;
}

void SDL_SetWindowMinimumSize(SDL_Window *window, int min_w, int min_h) {
  window->min_w = min_w; window->min_h = min_h;
}

Uint32 SDL_GetWindowPixelFormat(SDL_Window *window) {
  return window->pixelFormat;
}

SDL_Rect DRectToRect(SDL_DRect *d) {
  SDL_Rect r; 
  r.x = (int)d->x; 
  r.y = (int)d->y; 
  r.w = (int)d->w; 
  r.h = (int)d->h; 
  return r;
}

SDL_Result SDL_RenderGetViewport(SDL_Renderer *renderer, SDL_Rect *rect) {
  if (rect != NULL) *rect = DRectToRect(&renderer->viewport); 
  return SDL_OK;
}

SDL_Result SDL_SetPaletteColors(SDL_Palette *palette, const SDL_Color *colors, int firstcolor, int ncolors) {
  for(int i = 0; i < ncolors; i++) palette->colors[i+firstcolor]=colors[i];
  return SDL_OK;
}

SDL_Result SDL_OpenAudio(SDL_AudioSpec *desired, SDL_AudioSpec *obtained) {
  return SDL_ERROR;
}

void SDL_CloseAudio() {
}

void SDL_PauseAudio(int pause_on) {
}

void SDL_PumpEvents() {
}

Uint32 SDL_GetWindowID(SDL_Window *window) {
  return window->windowID;
}

SDL_MessageBoxButtonData _SDL_MessageBoxButtonData(int flags, int buttonid, const char *text) {
  SDL_MessageBoxButtonData r; r.flags = flags; r.buttonid = buttonid; r.text = text; return r;
}

SDL_Result SDL_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid) {
  return SDL_ERROR;
}

SDL_Result SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window) {
  return SDL_ERROR;
}

void SDL_Log(const char *fmt, ...) {;}
void SDL_GetVersion(SDL_version *a) {a->minor = SDL_MINOR_VERSION;a->major = SDL_MAJOR_VERSION;a->patch = SDL_PATCHLEVEL;}

Uint16 SDL_SwapLE16(Uint16 a) {
  return a;
}

Uint32 SDL_SwapLE32(Uint32 a) {
  return a;
}

Uint16 SDL_Swap16(Uint16 a) {
  return (Uint16)((a>>8)|(a<<8));
}

int snprintf(char *buffer, const int size, const char *format, ...) {
  return sprintf(buffer,format); // :mad: todo
}

char *SDL_GetPrefPath(const char *a, const char *b) {
  return ".";
}

char *SDL_GetBasePath() {
  return ".";
}

SDL_Result SDL_Init(Uint32 flags) {
  sdl_initialized |= flags;
  installKeyboardHandler();
  installTimerIrq();
  SDL_SetSeconds(0);
  //glWaitVSync = GL_TRUE;
  return SDL_OK;
}

bool SDL_WasInit(Uint32 flags) {
  return (sdl_initialized & flags)==flags ? true : false;
}

SDL_Result SDL_InitSubSystem(Uint32 flags) {
  sdl_initialized |= flags;
  return flags == SDL_INIT_AUDIO ? SDL_ERROR : SDL_OK;
}

void SDL_QuitSubSystem(Uint32 flags) {
}

void SDL_free(void *a) {
  free(a);
}

int SDL_GetNumVideoDisplays() {
  return 1;
}

int sdlDrawColorR = 0;
int sdlDrawColorG = 0;
int sdlDrawColorB = 0;
int sdlDrawColorA = 0;

SDL_Result SDL_SetRenderDrawColor(SDL_Renderer *renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
  sdlDrawColorR = r;
  sdlDrawColorG = g;
  sdlDrawColorB = b;
  sdlDrawColorA = a;
  return SDL_OK;
}

void SDL_FillRect(SDL_Surface *dst, const SDL_Rect *rect, Uint32 color) {
  if (dst == NULL) return;
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = dx0 + dst->w;
  int dy1 = dy0 + dst->h;
  if (rect != NULL) {
    dx0 = rect->x;
    dy0 = rect->y;
    dx1 = dx0+rect->w;
    dy1 = dy0+rect->h;
  }
  if (dx0<0) dx0 = 0;
  if (dy0<0) dy0 = 0;
  if (dx1>dst->w) dx1 = dst->w;
  if (dy1>dst->h) dy1 = dst->h;
  if (dst->format->BitsPerPixel==8) {
    for (int y = dy0; y < dy1; y++) {
      for (int x = dx0; x < dx1; x++) {
        dst->pixels[x+y*dst->pitch]=0;
      }
    }
  }
}

void SDL_Delay(double ms) {
  double b = SDL_GetTicks();
  while(fabs((double)SDL_GetTicks()-b)<ms) {;}
}

void SDL_Quit() {
  glDone();
  uninstallKeyboardHandler();
  uninstallTimerIrq();
  exit(0);
}

int SDL_JoystickEventState(int state) {
  return 0;
}

int SDL_JoystickGetAxis(SDL_Joystick *joystick, int axis) {
  return 0;
}

int SDL_JoystickGetButton(SDL_Joystick *joystick, int button) {
  return 0;
}

int SDL_JoystickGetHat(SDL_Joystick *joystick, int hat) {
  return 0;
}

void SDL_JoystickUpdate() {
}

int SDL_NumJoysticks() {
  return 0;
}

SDL_Joystick *SDL_JoystickOpen(int device_index) {
  return NULL;
}

void SDL_JoystickClose(SDL_Joystick *joystick) {
}

const char *SDL_JoystickName(SDL_Joystick *joystick) {
  return NULL;
}

const char *SDL_JoystickNameForIndex(int device_index) {
  return NULL;
}

int SDL_JoystickNumAxes(SDL_Joystick *joystick) {
  return 0;
}

int SDL_JoystickNumButtons(SDL_Joystick *joystick) {
  return 0;
}

int SDL_JoystickNumHats(SDL_Joystick *joystick) {
  return 0;
}

SDL_GameController *SDL_GameControllerOpen(int joystick_index) {
  return 0;
}

int SDL_GameControllerGetButton(SDL_GameController *gamecontroller,SDL_GameControllerButton button) {
  return 0;
}

int SDL_GameControllerGetAxis(SDL_GameController *gamecontroller,SDL_GameControllerAxis axis) {
  return 0;
}

void SDL_GameControllerClose(SDL_GameController *gamecontroller) {
}

Uint32 SDL_GetTicks() {
  return (Uint32)(SDL_GetSeconds()*1000.0);
}

SDL_Event events[256];
int currentEvent=0;

Uint8 scanCode_last[SDL_NUM_SCANCODES] = {0};

Uint8 *SDL_GetKeyboardState(int *numKeys) {
  *numKeys = sizeof(scanCode_last);
  return scanCode_last;
}

void SDL_ResetKeyboard() {
  memset(scanCode_last,0,sizeof(scanCode_last));
  memset((void*)keyPressed,0,sizeof(keyPressed));
}

#define SCANCODE_ESCAPE 1
#define SCANCODE_1 2
#define SCANCODE_2 3
#define SCANCODE_3 4
#define SCANCODE_4 5
#define SCANCODE_5 6
#define SCANCODE_6 7
#define SCANCODE_7 8
#define SCANCODE_8 9
#define SCANCODE_9 10
#define SCANCODE_0 11
#define SCANCODE_QUESTION 12
#define SCANCODE_UPPERDOT 13
#define SCANCODE_BACKSPACE 14
#define SCANCODE_TAB 15
#define SCANCODE_Q 16
#define SCANCODE_W 17
#define SCANCODE_E 18
#define SCANCODE_R 19
#define SCANCODE_T 20
#define SCANCODE_Z 21
#define SCANCODE_U 22
#define SCANCODE_I 23
#define SCANCODE_O 24
#define SCANCODE_P 25
#define SCANCODE_UE 26
#define SCANCODE_PLUS 27
#define SCANCODE_ENTER 28
#define SCANCODE_NUMENTER 28
#define SCANCODE_PAUSE 29
#define SCANCODE_CTRL 29
#define SCANCODE_A 30
#define SCANCODE_S 31
#define SCANCODE_D 32
#define SCANCODE_F 33
#define SCANCODE_G 34
#define SCANCODE_H 35
#define SCANCODE_J 36
#define SCANCODE_K 37
#define SCANCODE_L 38
#define SCANCODE_OE 39
#define SCANCODE_AE 40
#define SCANCODE_DACH 41
#define SCANCODE_LSHIFT 42
#define SCANCODE_HASHTAG 43
#define SCANCODE_Y 44
#define SCANCODE_X 45
#define SCANCODE_C 46
#define SCANCODE_V 47
#define SCANCODE_B 48
#define SCANCODE_N 49
#define SCANCODE_M 50
#define SCANCODE_COMMA 51
#define SCANCODE_DOT 52
#define SCANCODE_NUMDIVIDE 53
#define SCANCODE_MINUS 53
#define SCANCODE_RSHIFT 54
#define SCANCODE_NUMMUL 55
#define SCANCODE_PRINT 55
#define SCANCODE_ALT 56
#define SCANCODE_SPACE 57
#define SCANCODE_SHIFTLOCK 58
#define SCANCODE_F1 59
#define SCANCODE_F2 60
#define SCANCODE_F3 61
#define SCANCODE_F4 62
#define SCANCODE_F5 63
#define SCANCODE_F6 64
#define SCANCODE_F7 65
#define SCANCODE_F8 66
#define SCANCODE_F9 67
#define SCANCODE_F10 68
#define SCANCODE_NUMNUM 69
#define SCANCODE_ROLL 70
#define SCANCODE_START 71
#define SCANCODE_NUM7 71
#define SCANCODE_NUM8 72
#define SCANCODE_UP 72
#define SCANCODE_NUM9 73
#define SCANCODE_PAGEUP 73
#define SCANCODE_NUMMINUS 74
#define SCANCODE_NUM4 75
#define SCANCODE_LEFT 75
#define SCANCODE_NUM5 76
#define SCANCODE_NUM6 77
#define SCANCODE_RIGHT 77
#define SCANCODE_NUMPLUS 78
#define SCANCODE_NUM1 79
#define SCANCODE_END 79
#define SCANCODE_NUM2 80
#define SCANCODE_DOWN 80
#define SCANCODE_NUM3 81
#define SCANCODE_PAGEDOWN 81
#define SCANCODE_INSERT 82
#define SCANCODE_NUM0 82
#define SCANCODE_NUMCOMMA 83
#define SCANCODE_DELETE 83
#define SCANCODE_SMALLER 86
#define SCANCODE_F11 87
#define SCANCODE_F12 88
#define SCANCODE_WINDOWS 91

//  CONVSC2(SDLK_PLUS,SDL_SCANCODE_KP_PLUS,SCANCODE_PLUS) attention here
//  CONVSC2(SDLK_KP_PLUS,SDL_SCANCODE_KP_PLUS,SCANCODE_NUM_PLUS) attention here

#define SCANCODESHIFT 0x100
#define SCANCODEALT 0x200

#define CONVSCM\
  CONVSC2(SDLK_UNKNOWN,SDL_SCANCODE_UNKNOWN,0,"UNKNOWN")\
  CONVSC2(SDLK_RETURN,SDL_SCANCODE_RETURN,SCANCODE_ENTER,"RETURN")\
  CONVSC2(SDLK_ESCAPE,SDL_SCANCODE_ESCAPE,SCANCODE_ESCAPE,"ESCAPE")\
  CONVSC2(SDLK_BACKSPACE,SDL_SCANCODE_BACKSPACE,SCANCODE_BACKSPACE,"BACKSPACE")\
  CONVSC2(SDLK_TAB,SDL_SCANCODE_TAB,SCANCODE_TAB,"TAB")\
  CONVSC2(SDLK_SPACE,SDL_SCANCODE_SPACE,SCANCODE_SPACE,"SPACE")\
  CONVSC2(SDLK_PLUS,SDL_SCANCODE_KP_PLUS,SCANCODE_PLUS,"PLUS")\
  CONVSC2(SDLK_COMMA,SDL_SCANCODE_COMMA,SCANCODE_COMMA,"COMMA")\
  CONVSC2(SDLK_MINUS,SDL_SCANCODE_MINUS,SCANCODE_MINUS,"MINUS")\
  CONVSC2(SDLK_0,SDL_SCANCODE_0,SCANCODE_0,"0")\
  CONVSC2(SDLK_1,SDL_SCANCODE_1,SCANCODE_1,"1")\
  CONVSC2(SDLK_2,SDL_SCANCODE_2,SCANCODE_2,"2")\
  CONVSC2(SDLK_3,SDL_SCANCODE_3,SCANCODE_3,"3")\
  CONVSC2(SDLK_4,SDL_SCANCODE_4,SCANCODE_4,"4")\
  CONVSC2(SDLK_5,SDL_SCANCODE_5,SCANCODE_5,"5")\
  CONVSC2(SDLK_6,SDL_SCANCODE_6,SCANCODE_6,"6")\
  CONVSC2(SDLK_7,SDL_SCANCODE_7,SCANCODE_7,"7")\
  CONVSC2(SDLK_8,SDL_SCANCODE_8,SCANCODE_8,"8")\
  CONVSC2(SDLK_9,SDL_SCANCODE_9,SCANCODE_9,"9")\
  CONVSC2(SDLK_SEMICOLON,SDL_SCANCODE_SEMICOLON,SCANCODE_COMMA|SCANCODESHIFT,"SEMICOLON")\
  CONVSC2(SDLK_EQUALS,SDL_SCANCODE_EQUALS,SCANCODE_0|SCANCODESHIFT,"EQUALS")\
  CONVSC2(SDLK_LEFTBRACKET,SDL_SCANCODE_LEFTBRACKET,SCANCODE_8|SCANCODEALT,"LEFTBRACKET")\
  CONVSC2(SDLK_BACKSLASH,SDL_SCANCODE_BACKSLASH,SCANCODE_7|SCANCODESHIFT,"BACKSLASH")\
  CONVSC2(SDLK_RIGHTBRACKET,SDL_SCANCODE_RIGHTBRACKET,SCANCODE_9|SCANCODEALT,"RIGHTBRACKED")\
  CONVSC2(SDLK_a,SDL_SCANCODE_A,SCANCODE_A,"a")\
  CONVSC2(SDLK_b,SDL_SCANCODE_B,SCANCODE_B,"b")\
  CONVSC2(SDLK_c,SDL_SCANCODE_C,SCANCODE_C,"c")\
  CONVSC2(SDLK_d,SDL_SCANCODE_D,SCANCODE_D,"d")\
  CONVSC2(SDLK_e,SDL_SCANCODE_E,SCANCODE_E,"e")\
  CONVSC2(SDLK_f,SDL_SCANCODE_F,SCANCODE_F,"f")\
  CONVSC2(SDLK_g,SDL_SCANCODE_G,SCANCODE_G,"g")\
  CONVSC2(SDLK_h,SDL_SCANCODE_H,SCANCODE_H,"h")\
  CONVSC2(SDLK_i,SDL_SCANCODE_I,SCANCODE_I,"i")\
  CONVSC2(SDLK_j,SDL_SCANCODE_J,SCANCODE_J,"j")\
  CONVSC2(SDLK_k,SDL_SCANCODE_K,SCANCODE_K,"k")\
  CONVSC2(SDLK_l,SDL_SCANCODE_L,SCANCODE_L,"l")\
  CONVSC2(SDLK_m,SDL_SCANCODE_M,SCANCODE_M,"m")\
  CONVSC2(SDLK_n,SDL_SCANCODE_N,SCANCODE_N,"n")\
  CONVSC2(SDLK_o,SDL_SCANCODE_O,SCANCODE_O,"o")\
  CONVSC2(SDLK_p,SDL_SCANCODE_P,SCANCODE_P,"p")\
  CONVSC2(SDLK_q,SDL_SCANCODE_Q,SCANCODE_Q,"q")\
  CONVSC2(SDLK_r,SDL_SCANCODE_R,SCANCODE_R,"r")\
  CONVSC2(SDLK_s,SDL_SCANCODE_S,SCANCODE_S,"s")\
  CONVSC2(SDLK_t,SDL_SCANCODE_T,SCANCODE_T,"t")\
  CONVSC2(SDLK_u,SDL_SCANCODE_U,SCANCODE_U,"u")\
  CONVSC2(SDLK_v,SDL_SCANCODE_V,SCANCODE_V,"v")\
  CONVSC2(SDLK_w,SDL_SCANCODE_W,SCANCODE_W,"w")\
  CONVSC2(SDLK_x,SDL_SCANCODE_X,SCANCODE_X,"x")\
  CONVSC2(SDLK_y,SDL_SCANCODE_Y,SCANCODE_Y,"y")\
  CONVSC2(SDLK_z,SDL_SCANCODE_Z,SCANCODE_Z,"z")\
  CONVSC2(SDLK_CAPSLOCK,SDL_SCANCODE_CAPSLOCK,SCANCODE_SHIFTLOCK,"CAPSLOCK")\
  CONVSC2(SDLK_F1,SDL_SCANCODE_F1,SCANCODE_F1,"F1")\
  CONVSC2(SDLK_F2,SDL_SCANCODE_F2,SCANCODE_F2,"F2")\
  CONVSC2(SDLK_F3,SDL_SCANCODE_F3,SCANCODE_F3,"F3")\
  CONVSC2(SDLK_F4,SDL_SCANCODE_F4,SCANCODE_F4,"F4")\
  CONVSC2(SDLK_F5,SDL_SCANCODE_F5,SCANCODE_F5,"F5")\
  CONVSC2(SDLK_F6,SDL_SCANCODE_F6,SCANCODE_F6,"F6")\
  CONVSC2(SDLK_F7,SDL_SCANCODE_F7,SCANCODE_F7,"F7")\
  CONVSC2(SDLK_F8,SDL_SCANCODE_F8,SCANCODE_F8,"F8")\
  CONVSC2(SDLK_F9,SDL_SCANCODE_F9,SCANCODE_F9,"F9")\
  CONVSC2(SDLK_F10,SDL_SCANCODE_F10,SCANCODE_F10,"F10")\
  CONVSC2(SDLK_F11,SDL_SCANCODE_F11,SCANCODE_F11,"F11")\
  CONVSC2(SDLK_F12,SDL_SCANCODE_F12,SCANCODE_F12,"F12")\
  CONVSC2(SDLK_PRINTSCREEN,SDL_SCANCODE_PRINTSCREEN,SCANCODE_PRINT,"PRINTSCREEN")\
  CONVSC2(SDLK_INSERT,SDL_SCANCODE_INSERT,SCANCODE_INSERT,"INSERT")\
  CONVSC2(SDLK_HOME,SDL_SCANCODE_HOME,SCANCODE_START,"HOME")\
  CONVSC2(SDLK_PAGEUP,SDL_SCANCODE_PAGEUP,SCANCODE_PAGEUP,"PAGEUP")\
  CONVSC2(SDLK_DELETE,SDL_SCANCODE_DELETE,SCANCODE_DELETE,"DELETE")\
  CONVSC2(SDLK_END,SDL_SCANCODE_END,SCANCODE_END,"END")\
  CONVSC2(SDLK_PAGEDOWN,SDL_SCANCODE_PAGEDOWN,SCANCODE_PAGEDOWN,"PAGEDOWN")\
  CONVSC2(SDLK_RIGHT,SDL_SCANCODE_RIGHT,SCANCODE_RIGHT,"RIGHT")\
  CONVSC2(SDLK_LEFT,SDL_SCANCODE_LEFT,SCANCODE_LEFT,"LEFT")\
  CONVSC2(SDLK_DOWN,SDL_SCANCODE_DOWN,SCANCODE_DOWN,"DOWN")\
  CONVSC2(SDLK_UP,SDL_SCANCODE_UP,SCANCODE_UP,"UP")\
  CONVSC2(SDLK_NUMLOCKCLEAR,SDL_SCANCODE_NUMLOCKCLEAR,SCANCODE_NUMNUM,"NUMLOCKCLEAR")\
  CONVSC2(SDLK_KP_MINUS,SDL_SCANCODE_KP_MINUS,SCANCODE_NUMMINUS,"KEYPAD MINUS")\
  CONVSC2(SDLK_LCTRL,SDL_SCANCODE_LCTRL,SCANCODE_CTRL,"LEFT CTRL")\
  CONVSC2(SDLK_LSHIFT,SDL_SCANCODE_LSHIFT,SCANCODE_LSHIFT,"LEFT SHIFT")\
  CONVSC2(SDLK_LALT,SDL_SCANCODE_LALT,SCANCODE_ALT,"ALT")\
  CONVSC2(SDLK_RSHIFT,SDL_SCANCODE_RSHIFT,SCANCODE_RSHIFT,"RIGHT SHIFT")\
  CONVSC2(SDLK_KP_5,SDL_SCANCODE_KP_5,SCANCODE_NUM5,"KEYPAD 5")
/*
  CONVSC2(SDLK_EXCLAIM,SDL_SCANCODE_EXCLAIM,-1)\
  CONVSC2(SDLK_QUOTEDBL,SDL_SCANCODE_QUOTEDBL,-1)\
  CONVSC2(SDLK_HASH,SDL_SCANCODE_HASH,-1)\
  CONVSC2(SDLK_PERCENT,SDL_SCANCODE_PERCENT,-1)\
  CONVSC2(SDLK_DOLLAR,SDL_SCANCODE_DOLLAR,-1)\
  CONVSC2(SDLK_AMPERSAND,SDL_SCANCODE_AMPERSAND,-1)\
  CONVSC2(SDLK_QUOTE,SDL_SCANCODE_QUOTE,-1)\
  CONVSC2(SDLK_LEFTPAREN,SDL_SCANCODE_LEFTPAREN,-1)\
  CONVSC2(SDLK_RIGHTPAREN,SDL_SCANCODE_RIGHTPAREN,-1)\
  CONVSC2(SDLK_COLON,SDL_SCANCODE_COLON,-1)\
  CONVSC2(SDLK_LESS,SDL_SCANCODE_LESS,-1)\
  CONVSC2(SDLK_GREATER,SDL_SCANCODE_GREATER,-1)\
  CONVSC2(SDLK_QUESTION,SDL_SCANCODE_QUESTION,-1)\
  CONVSC2(SDLK_AT,SDL_SCANCODE_AT,-1)\
  CONVSC2(SDLK_CARET,SDL_SCANCODE_CARET,-1)\
  CONVSC2(SDLK_UNDERSCORE,SDL_SCANCODE_UNDERSCORE,-1)\
  CONVSC2(SDLK_SCROLLLOCK,SDL_SCANCODE_SCROLLOCK,-1)\
  CONVSC2(SDLK_ASTERISK,SDL_SCANCODE_ASTERISK,-1)\
  CONVSC2(SDLK_BACKQUOTE,SDL_SCANCODE_BACKQUOTE,-1)\
  CONVSC2(SDLK_KP_PERIOD,SDL_SCANCODE_KP_PERIOD,-1)\
  CONVSC2(SDLK_KP_DIVIDE,SDL_SCANCODE_KP_DIVIDE,-1)\
  CONVSC2(SDLK_KP_ENTER,SDL_SCANCODE_KP_ENTER,-1)\
  CONVSC2(SDLK_PAUSE,SDL_SCANCODE_PAUSE,-1)\
  CONVSC2(SDLK_KP_1,SDL_SCANCODE_KP_1,-1)\
  CONVSC2(SDLK_KP_2,SDL_SCANCODE_KP_2,-1)\
  CONVSC2(SDLK_KP_3,SDL_SCANCODE_KP_3,-1)\
  CONVSC2(SDLK_KP_4,SDL_SCANCODE_KP_4,-1)\
  CONVSC2(SDLK_KP_6,SDL_SCANCODE_KP_6,-1)\
  CONVSC2(SDLK_KP_7,SDL_SCANCODE_KP_7,-1)\
  CONVSC2(SDLK_KP_8,SDL_SCANCODE_KP_8,-1)\
  CONVSC2(SDLK_KP_9,SDL_SCANCODE_KP_9,-1)\
  CONVSC2(SDLK_KP_0,SDL_SCANCODE_KP_0,-1)\
  CONVSC2(SDLK_KP_MULTIPLY,SDL_SCANCODE_KP_MULTIPLY,-1)\
  CONVSC2(SDLK_RCTRL,SDL_SCANCODE_RCTRL,-1)\
  CONVSC2(SDLK_RALT,SDL_SCANCODE_RALT,-1)\
  CONVSC2(SDLK_PERIOD,SDL_SCANCODE_PERIOD,-1)\
  CONVSC2(SDLK_SLASH,SDL_SCANCODE_SLASH,-1)
*/
const char *SDL_GetKeyName(SDL_Keycode key) {
  switch((int)key) {
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) case ((int)(__keycode__)): {return __name__;}
    CONVSCM
  }
  return "UNKNOWN";
}

SDL_Scancode SDL_GetScancodeFromName(const char *name) {
#undef CONVSC2
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) if (strcmp(name,__name__)==0) return __scancode__;
    CONVSCM
  return SDL_SCANCODE_UNKNOWN;
}

const char *SDL_GetScancodeName(SDL_Scancode scancode) {
  switch((int)scancode) {
#undef CONVSC2
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) case ((int)(__scancode__)): {return __name__;}
    CONVSCM
  }
  return "UNKNOWN";
}

SDL_Keycode SDL_GetKeyFromName(const char *name) {
#undef CONVSC2
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) if (strcmp(name,__name__)==0) return __keycode__;
    CONVSCM
  return SDLK_UNKNOWN;
}

SDL_Keycode SDL_GetKeyFromScancode(SDL_Scancode scancode) {
  switch((int)scancode) {
#undef CONVSC2
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) case ((int)(__scancode__)): {return __keycode__;}
    CONVSCM
  }
  return SDLK_UNKNOWN;
}

Uint8 isDosKeyPressed(int dosCode) {
  Uint8 r=0;
  if (keyPressed[dosCode&0xff]) r = 1;
  if ((dosCode&SCANCODESHIFT)&&((!(keyPressed[SCANCODE_LSHIFT]))&&(!(keyPressed[SCANCODE_RSHIFT])))) r = 0;
  if ((dosCode&SCANCODEALT)&&(!(keyPressed[SCANCODE_ALT]))) r = 0;
  return r;
}

Uint8 pressedScanCode(SDL_Scancode s) {
  switch((int)s) {
#undef CONVSC2
#define CONVSC2(__keycode__,__scancode__,__doscode__,__name__) case ((int)(__scancode__)): {return isDosKeyPressed(__doscode__);}
    CONVSCM
  }
  return 0;
}

Uint32 SDL_TimeStamp() {
  return SDL_GetTicks();
}

bool addKeyboardEvents() {
  int key = glNextKey();
  if (key == GL_VK_END) {
    SDL_Quit();
  }
  if (key != 0) {
    if (key < 128 && key >= 32) {
      SDL_Event *v = &events[currentEvent];
      v->text.timestamp = SDL_TimeStamp();
      v->type = SDL_TEXTINPUT;
      v->text.text[0] = (unsigned char)(key);
      v->text.text[1] = 0;
      currentEvent++;
    }
  }
  for (int i = 0; i < sizeof(scanCode_last); i++) {
    Uint8 pressed = pressedScanCode((SDL_Scancode)i);
    if (pressed != scanCode_last[i]) {
      scanCode_last[i] = pressed;
      SDL_Event *v = &events[currentEvent];
      v->key.timestamp = SDL_TimeStamp();
      v->type = pressed ? SDL_KEYDOWN : SDL_KEYUP;
      v->key.state = pressed ? 1 : 0;
      v->key.keysym.scancode = (SDL_Scancode)i;
      v->key.keysym.sym = SDL_GetKeyFromScancode((SDL_Scancode)i);
      v->key.keysym.mod = 0;
      if (keyPressed[SDL_SCANCODE_LCTRL]) v->key.keysym.mod|=(int)KMOD_LCTRL;
      if (keyPressed[SDL_SCANCODE_RCTRL]) v->key.keysym.mod|=(int)KMOD_RCTRL;
      if (keyPressed[SDL_SCANCODE_LALT]) v->key.keysym.mod|=(int)KMOD_LALT;
      if (keyPressed[SDL_SCANCODE_RALT]) v->key.keysym.mod|=(int)KMOD_RALT;
      if (keyPressed[SDL_SCANCODE_LSHIFT]) v->key.keysym.mod|=(int)KMOD_LSHIFT;
      if (keyPressed[SDL_SCANCODE_RSHIFT]) v->key.keysym.mod|=(int)KMOD_RSHIFT;
      currentEvent++;
    }
  }
  return currentEvent>0;
}

Uint32 sdlMouseButtonState = 0;
bool buttonPressed_last[3] = {0};
double sdlMouseX = 0;
double sdlMouseY = 0;
double sdlMouseRelX = 0;
double sdlMouseRelY = 0;

void SDL_WarpMouseInWindow(SDL_Window *window, int x, int y) {
  sdlMouseX = window->x + x;
  sdlMouseY = window->y + y;
}

Uint32 SDL_GetMouseState(int *x, int *y) {
  if (x != NULL) *x=(int)sdlMouseX;
  if (y != NULL) *y=(int)sdlMouseY;
  return sdlMouseButtonState;
}

Uint32 SDL_GetRelativeMouseState(int *x, int *y) {
  if (x != NULL) *x=(int)sdlMouseRelX;
  if (y != NULL) *y=(int)sdlMouseRelY;
  return sdlMouseButtonState;
}

bool addMouseEvents() {
  double dx,dy;
  glNextMouseDelta(&dx,&dy);
  sdlMouseRelX = (int)dx;
  sdlMouseRelY = (int)dy;
  sdlMouseX += dx;
  sdlMouseY += dy;
  if (sdlMouseX<0)sdlMouseX=0;
  if (sdlMouseY<0)sdlMouseY=0;
  if (sdlMouseX>glFrameBufferWidth-1) sdlMouseX=glFrameBufferWidth-1;
  if (sdlMouseY>glFrameBufferHeight-1) sdlMouseY=glFrameBufferHeight-1;
  sdlMouseButtonState = 0;
  for (int i = 0; i < 0x03; i++) {
    bool buttonPressed = glMouseButtons() & (1<<i);
    if (buttonPressed != buttonPressed_last[i]) {
      buttonPressed_last[i] = buttonPressed;
      SDL_Event *v = &events[currentEvent];
      v->button.timestamp = SDL_TimeStamp();
      v->type = buttonPressed ? SDL_MOUSEBUTTONDOWN:SDL_MOUSEBUTTONUP;
      int j = SDL_BUTTON_LEFT;
      switch(i) {
      case 0: j = SDL_BUTTON_LEFT; break;
      case 1: j = SDL_BUTTON_RIGHT; break;
      case 2: j = SDL_BUTTON_MIDDLE; break;
      }
      sdlMouseButtonState|=(Uint32)j;
      v->button.button = (Uint8)j;
      v->button.x = sdlMouseX;
      v->button.y = sdlMouseY;
      currentEvent++;
    }
  }
  if (dx!=0||dy!=0) {
    SDL_Event *v = &events[currentEvent];
    v->motion.timestamp = SDL_TimeStamp();
    v->type = SDL_MOUSEMOTION;
    v->motion.x = sdlMouseX;
    v->motion.y = sdlMouseY;
    v->motion.xrel = dx;
    v->motion.yrel = dy;
    currentEvent++;
  }
  return currentEvent>0;
}

void addFocusEvent(bool hasNowFocus) {
  SDL_Event *v = &events[currentEvent];
  v->window.timestamp = SDL_TimeStamp();
  v->type = SDL_WINDOWEVENT;
  v->window.event = hasNowFocus?SDL_WINDOWEVENT_FOCUS_GAINED:SDL_WINDOWEVENT_FOCUS_LOST;
  currentEvent++;
}

void addResizeEvent() {
  SDL_Event *v = &events[currentEvent];
  v->window.timestamp = SDL_TimeStamp();
  v->type = SDL_WINDOWEVENT;
  v->window.event = SDL_WINDOWEVENT_RESIZED;
  currentEvent++;
}

int SDL_PushEvent(SDL_Event *event) {
  return 0;
}

int SDL_PollEvent(SDL_Event *event) {
  if (currentEvent <= 0) {
    currentEvent = 0;
    addKeyboardEvents();
    addMouseEvents();
    return 0;  
  }
  currentEvent--;
  if (event != NULL) *event = events[currentEvent];
  return 1;
}

int SDL_WaitEvent(SDL_Event *event) {
  while(!SDL_PollEvent(event));
  if (event==NULL) currentEvent++;
  return 1;
}

Uint8 SDL_EventState(Uint32 type, int state) {
  return 0;
}

const char *SDL_GetError() {
  return "";
}

int SDL_ShowCursor(int toggle) {
  return 0;
}

size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen) {
  int i;
  for (i = 0; i < maxlen-1; i++) {
    if (src[i]==0x00) break;
    dst[i]=src[i];
  }
  dst[i]=0x00;
  return i;
}

int SDL_SetRelativeMouseMode(bool enabled) {
  return 0;
}

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes) {
  return 0;
}

void SDL_PauseAudioDevice(unsigned int id, int b) {
}

void SDL_CloseAudioDevice(unsigned int id) {
}

void SDL_LockAudioDevice(unsigned int id) {
}

void SDL_UnlockAudioDevice(unsigned int id) {
}

int SDL_BuildAudioCVT(SDL_AudioCVT *cvt, SDL_AudioFormat src_format, Uint8 src_channels, int src_rate, SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate) {
  return 0;
}

SDL_Result SDL_ConvertAudio(SDL_AudioCVT *cvt) {
  return SDL_OK;
}

Uint32 SDL_MapRGB(const SDL_PixelFormat * format, Uint8 r, Uint8 g, Uint8 b) {
  switch(format->BitsPerPixel) {
  case 32:return (int)r|((int)g<<8)|((int)b<<16)|0xff000000;
  }
  return rand()*0x01020304;
}

SDL_Keymod SDL_GetModState() {
  int r = (int)KMOD_NONE;
  if (keyPressed[SDL_SCANCODE_LCTRL]) r|=(int)KMOD_LCTRL;
  if (keyPressed[SDL_SCANCODE_RCTRL]) r|=(int)KMOD_RCTRL;
  if (keyPressed[SDL_SCANCODE_LALT]) r|=(int)KMOD_LALT;
  if (keyPressed[SDL_SCANCODE_RALT]) r|=(int)KMOD_RALT;
  if (keyPressed[SDL_SCANCODE_LSHIFT]) r|=(int)KMOD_LSHIFT;
  if (keyPressed[SDL_SCANCODE_RSHIFT]) r|=(int)KMOD_RSHIFT;
  return (SDL_Keymod)r;
}

Uint32 sdlGetFormat(int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  if ((depth == 8) && (Rmask==0) && (Gmask==0) && (Bmask==0) && (Amask==0)) return SDL_PIXELFORMAT_INDEX8;
  if (depth == 32) return SDL_PIXELFORMAT_RGBA8888;
  return SDL_PIXELFORMAT_RGBA8888;
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  RALLOC(SDL_Surface,r);
  Uint32 format = sdlGetFormat(depth,Rmask,Gmask,Bmask,Amask);
  r->format = SDL_AllocFormat(format);
  r->pixels = (Uint8*)malloc(width*height*(depth/8));
  r->pitch = width*(depth/8);
  r->w = width;
  r->h = height;
  return r;
}

SDL_Surface *SDL_CreateRGBSurfaceFrom(void *pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  return NULL;
}

SDL_Texture *SDL_CreateTextureFromSurface(SDL_Renderer *renderer, SDL_Surface *surface) {
  SDL_Surface zwo;
  zwo.format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
  zwo.pixels = (Uint8*)malloc(surface->w*surface->h*4);
  zwo.pitch = surface->w*4;
  zwo.w = surface->w;
  zwo.h = surface->h;
  SDL_UpperBlit(surface, NULL, &zwo, NULL);
  SDL_FreeFormat(zwo.format);
  RALLOC(SDL_Texture,r);
  r->pitch = zwo.pitch;
  r->w = zwo.w;
  r->h = zwo.h;
  r->pixels = (unsigned int *)zwo.pixels; 
  r->format = SDL_PIXELFORMAT_RGBA8888;
  return r;
}

void SDL_FreeSurface(SDL_Surface *a) {
  if (a!=NULL) {
    RDELETE(a->pixels);
    SDL_FreeFormat(a->format);    
  }
  RDELETE(a);
}


SDL_DisplayMode sdlDisplayMode;
SDL_Rect sdlDisplayBounds;

SDL_Result SDL_GetDisplayBounds(int displayIndex, SDL_Rect *rect) {
  if (rect != NULL) {
    *rect = sdlDisplayBounds;
  }
  return SDL_OK;
}

SDL_Window *SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
  RALLOC(SDL_Window,r);
  r->x = x;
  r->y = y;
  r->w = w;
  r->h = h;
  r->flags = flags;
  r->pixelFormat = SDL_PIXELFORMAT_RGBA8888;
  glDone();
  if (!glVesa(w,h,32)) {
    printf("Vesa: SDL window:%dx%d failed!\n",w,h);
    SDL_Quit();
  }
  sdlDisplayMode.w = w;
  sdlDisplayMode.h = h;
  sdlDisplayBounds.x = 0;
  sdlDisplayBounds.y = 0;
  sdlDisplayBounds.w = w;
  sdlDisplayBounds.h = h;
  addFocusEvent(true);
  return r;
}

void SDL_SetWindowIcon(SDL_Window *window, SDL_Surface *icon) {
}

void SDL_DestroyWindow(SDL_Window *window) {
  RDELETE(window);
}

void SDL_GetWindowSize(SDL_Window *window, int *w, int *h) {
  if (w != NULL) *w = window->w;
  if (h != NULL) *h = window->h;
}

Uint32 SDL_GetWindowFlags(SDL_Window *window) {
  return window->flags;
}

void SDL_SetWindowPosition(SDL_Window *window, int x, int y) {
  window->x = x;
  window->y = y;
}

void SDL_SetWindowSize(SDL_Window *window, int w, int h) {
  glDone();
  if (!glVesa(w,h,32)) {
    printf("Vesa: SDL window:%dx%d failed!\n",w,h);
    SDL_Quit();
  }
  sdlDisplayMode.w = w;
  sdlDisplayMode.h = h;
  window->w = w;
  window->h = h;
}

SDL_Result SDL_GetCurrentDisplayMode(int displayIndex, SDL_DisplayMode *mode) {
  sdlDisplayMode.format=SDL_PIXELFORMAT_RGB888;
  sdlDisplayMode.refresh_rate = 50;
  *mode = sdlDisplayMode;
  return SDL_OK;
}

SDL_Result SDL_SetWindowFullscreen(SDL_Window *window, Uint32 flags) {
  return SDL_OK;
}

int SDL_GetWindowDisplayIndex(SDL_Window *window) {
  return 0;
}

SDL_RendererInfo sdlRendererDefaultInfo() {
  SDL_RendererInfo r;
  r.name = "WatcomGL";
  r.flags = 0;
  r.num_texture_formats = 1;
  r.texture_formats[0] = SDL_PIXELFORMAT_RGB888;
  r.max_texture_width = 16384; // can be more
  r.max_texture_height = 16384; // can be more
  return r;
}

SDL_Renderer *SDL_CreateRenderer(SDL_Window *window, int index, Uint32 flags) {
  RALLOC(SDL_Renderer,r);
  r->window = window;
  r->info = sdlRendererDefaultInfo();
  r->renderTarget = NULL;
  r->scale.x = 1;
  r->scale.y = 1;
  r->viewport.x = window->x;
  r->viewport.y = window->y;
  r->viewport.w = window->w;
  r->viewport.h = window->h;
  return r;
}

SDL_Result SDL_GetRendererInfo(SDL_Renderer *renderer, SDL_RendererInfo *info) {
  *info = renderer->info; return SDL_OK;
}
                                            
SDL_Result SDL_SetRenderTarget(SDL_Renderer *renderer, SDL_Texture *texture) {
  renderer->renderTarget = texture;
  return SDL_OK;
}

SDL_Result SDL_GetRendererOutputSize(SDL_Renderer *renderer, int *w, int *h) {
  if (renderer != NULL & w != NULL && renderer->window != NULL) *w = renderer->window->w;
  if (renderer != NULL & h != NULL && renderer->window != NULL) *h = renderer->window->h;
  return SDL_OK;
}

SDL_Result SDL_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) {
  renderer->logical_w = w;
  renderer->logical_h = h;
  return SDL_OK;
}

void SDL_DestroyRenderer(SDL_Renderer *renderer) {
  RDELETE(renderer);
}

SDL_Texture *SDL_CreateTexture(SDL_Renderer *renderer, Uint32 format, int access, int w, int h) {
  RALLOC(SDL_Texture,r);
  r->pitch = w * SDL_BYTESPERPIXEL(format);
  r->w = w;
  r->h = h;
  r->pixels = (unsigned int*)_RALLOC(unsigned char,w*h*SDL_BYTESPERPIXEL(format)); 
  r->format = format;
  return r;
}

void SDL_DestroyTexture(SDL_Texture *a) {
  if (a != NULL) RDELETE(a->pixels);
  RDELETE(a);
}

SDL_Result SDL_QueryTexture(SDL_Texture *texture, Uint32 *format, int *access, int *w, int *h) {
  if (texture!=NULL) *w = texture->w;
  if (texture!=NULL) *h = texture->h;
  return SDL_OK;
}

SDL_Result SDL_LockTexture(SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch) {
  if (pixels != NULL) *pixels = (void*)texture->pixels;
  if (pitch != NULL) *pitch = texture->pitch;
  return SDL_OK;
}

void SDL_UnlockTexture(SDL_Texture *t) {
}

SDL_Result SDL_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode) {
  texture->blendMode = blendMode;
  return SDL_OK;
}

SDL_Result SDL_SetTextureAlphaMod(SDL_Texture *texture, Uint8 alpha) {
  texture->modMode = alpha;
  return SDL_OK;
}

int sdlRShift = 0;
int sdlGShift = 0;
int sdlBShift = 0;
int sdlAShift = 0;
int sdlRBits = 0;
int sdlGBits = 0;
int sdlBBits = 0;
int sdlABits = 0;
bool SDL_PixelFormatEnumToMasks(Uint32 format, int *bpp, Uint32 *Rmask, Uint32 *Gmask, Uint32 *Bmask, Uint32 *Amask) {
  if (bpp != NULL) *bpp = SDL_BYTESPERPIXEL(format)*8; // sadly this would be 3 for XRGB8888
  if (Rmask != NULL) *Rmask = 0;
  if (Gmask != NULL) *Gmask = 0;
  if (Bmask != NULL) *Bmask = 0;
  if (Amask != NULL) *Amask = 0;
  int bits[4];
  switch(SDL_PIXELLAYOUT(format)) {
  case SDL_PACKEDLAYOUT_332: bits[0]=3; bits[1]=3; bits[2]=2; bits[3]=0; break;
  case SDL_PACKEDLAYOUT_4444: bits[0]=4; bits[1]=4; bits[2]=4; bits[3]=4; break;
  case SDL_PACKEDLAYOUT_1555: bits[0]=5; bits[1]=5; bits[2]=5; bits[3]=1; break;
  case SDL_PACKEDLAYOUT_5551: bits[0]=5; bits[1]=5; bits[2]=5; bits[3]=1; break;
  case SDL_PACKEDLAYOUT_565: bits[0]=5; bits[1]=6; bits[2]=5; bits[3]=0; break;
  case SDL_PACKEDLAYOUT_8888: bits[0]=8; bits[1]=8; bits[2]=8; bits[3]=8; break;
  case SDL_PACKEDLAYOUT_2101010: bits[0]=10; bits[1]=10; bits[2]=10; bits[3]=2; break;
  case SDL_PACKEDLAYOUT_1010102: bits[0]=10; bits[1]=10; bits[2]=10; bits[3]=2; break;
  default:
  case 0: bits[0]=0; bits[1]=0; bits[2]=0; bits[3]=0; break;
  }
  int ord[4];
  switch(SDL_PIXELORDER(format)) {
  case SDL_ARRAYORDER_RGB: ord[0]=0; ord[1]=1; ord[2]=2; ord[3]=-1; break;
  case SDL_ARRAYORDER_RGBA: ord[0]=0; ord[1]=1; ord[2]=2; ord[3]=3; break;
  case SDL_ARRAYORDER_ARGB: ord[0]=2; ord[1]=1; ord[2]=0; ord[3]=3; break;
  case SDL_ARRAYORDER_BGR: ord[0]=2; ord[1]=1; ord[2]=0; ord[3]=-1; break;
  case SDL_ARRAYORDER_BGRA: ord[0]=2; ord[1]=1; ord[2]=0; ord[3]=3; break;
  case SDL_ARRAYORDER_ABGR: ord[0]=0; ord[1]=1; ord[2]=2; ord[3]=3; break;
  default:
  case 0: ord[0]=-1; ord[1]=-1; ord[2]=-1; ord[3]=-1; break;
  }
  int startBit = 0;
  for (int i = 0; i < 4; i++) {
    int bs = bits[i];
    int b = ((1 << bs)-1)<<startBit;
    switch(ord[i]) {
    case 0: if (*Rmask!=NULL) *Rmask=b; sdlRShift=startBit; sdlRBits=bs; break;
    case 1: if (*Gmask!=NULL) *Gmask=b; sdlGShift=startBit; sdlGBits=bs; break;
    case 2: if (*Bmask!=NULL) *Bmask=b; sdlBShift=startBit; sdlBBits=bs; break;
    case 3: if (*Amask!=NULL) *Amask=b; sdlAShift=startBit; sdlABits=bs; break;
    }  
    startBit+=bs;
  }
  return true;
}

SDL_Palette *SDL_AllocPalette(int numColors) {
  RALLOC(SDL_Palette,r);
  r->ncolors = numColors;
  r->colors = _RALLOC(SDL_Color,numColors);
  return r;
}

void SDL_FreePalette(SDL_Palette *palette) {
  if (palette != NULL && palette->colors != NULL) RDELETE(palette->colors);
}

SDL_PixelFormat *SDL_AllocFormat(Uint32 pixel_format) {
  RALLOC(SDL_PixelFormat,r);
  r->BitsPerPixel = SDL_BYTESPERPIXEL(pixel_format)*8; // sadly this would be 24 for XRGB8888 format
  r->BytesPerPixel = r->BitsPerPixel/8;
  SDL_PixelFormatEnumToMasks(pixel_format, NULL, &r->Rmask, &r->Gmask, &r->Bmask, &r->Amask);
  r->Rloss=1<<(8-sdlRBits);
  r->Gloss=1<<(8-sdlGBits);
  r->Bloss=1<<(8-sdlBBits);
  r->Aloss=1<<(8-sdlABits);
  r->Rshift=sdlRShift;
  r->Gshift=sdlGShift;
  r->Bshift=sdlBShift;
  r->Ashift=sdlAShift;
  r->palette=NULL;
  switch(SDL_PIXELTYPE(pixel_format)) {
  case SDL_PIXELTYPE_INDEX8:r->palette=SDL_AllocPalette(256);break;
  }
  return r;
}

void SDL_FreeFormat(SDL_PixelFormat *format) {
  if (format != NULL) SDL_FreePalette(format->palette);
  RDELETE(format);
}

void SDL_ShowWindow(SDL_Window *window) {
}

SDL_Result SDL_RenderClear(SDL_Renderer *renderer) {
  memset(glFrameBuffer,0,glFrameBufferWidth*glFrameBufferHeight*sizeof(unsigned int));
  return SDL_OK;
}

void SDL_RenderPresent(SDL_Renderer *renderer) {
  glRefresh();
}

SDL_Result SDL_UpperBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  if (src == NULL) return SDL_ERROR;
  if (dst == NULL) return SDL_ERROR;
  int minx = 0;
  int miny = 0;
  int maxx = dst->w;
  int maxy = dst->h;
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = sx0+src->w;
  int sy1 = sy0+src->h;
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = dx0+dst->w;
  int dy1 = dy0+dst->h;
  if (srcrect!=NULL) {
    sx0 = srcrect->x;
    sy0 = srcrect->y;
    sx1 = sx0+srcrect->w;
    sy1 = sy0+srcrect->h;
  }
  if (dstrect!=NULL) {
    dx0 = dstrect->x;
    dy0 = dstrect->y;
    dx1 = dx0+dstrect->w;
    dy1 = dy0+dstrect->h;
  }
  if (dx0>=maxx) return SDL_ERROR;
  if (dx1<=minx) return SDL_ERROR;
  if (dy0>=maxy) return SDL_ERROR;
  if (dy1<=miny) return SDL_ERROR;
  double sfx0 = sx0;
  double sfx1 = sx1;
  double sfy0 = sy0;
  double sfy1 = sy1;
  double ax = (double)(sx1-sx0)/(dx1-dx0);
  double ay = (double)(sy1-sy0)/(dy1-dy0);
  if (dx0<minx) {
    sfx0 += (minx-dx0)*ax;
    dx0 = minx;
  }
  if (dy0<miny) {
    sfy0 += (miny-dy0)*ay;
    dy0 = miny;
  }
  if (dx1>maxx) {
    sfx1 -= (dx1-maxx)*ax;
    dx1 = maxx;
  }
  if (dy1>maxy) {
    sfy1 -= (dy1-maxy)*ay;
    dy1 = maxy;
  }

  int f=src->format->BytesPerPixel==1?1:0;
  f |= dst->format->BytesPerPixel==1?2:0;
  Uint32 *spal=NULL;
  if (src->format->palette!=NULL) spal=(Uint32 *)src->format->palette->colors;

  for (int y = dy0; y < dy1; y++) {
    const int sy = (int)((y - dy0)*ay + sfy0);
    if (sy >= 0 && sy < src->h) {
      unsigned char *s = (unsigned char*)((Uint32)src->pixels+sy*src->pitch);
      unsigned char *d = (unsigned char*)((Uint32)dst->pixels+y*dst->pitch);
      for (int x = dx0; x < dx1; x++) {
        const int sx =(int)((x - dx0)*ax + sx0);
        if (sx >= 0 && sx < src->w) {
          switch(f) {
            case 1*0+2*0:*((unsigned int*)&d[x*4])=*((unsigned int*)&s[sx*4]);break;
            case 1*1+2*0:*((unsigned int*)&d[x*4])=spal[*((unsigned char*)&s[sx])];break;
            case 1*0+2*1:*((unsigned char*)&d[x])=*((unsigned int*)&s[sx*4]);break;
            case 1*1+2*1:*((unsigned char*)&d[x])=*((unsigned char*)&s[sx]);break;
          }
        }
      }
    }
  }

  return SDL_OK;
}

SDL_Result SDL_LowerBlit(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect) {
  return SDL_UpperBlit(src, srcrect, dst, dstrect);
}

SDL_Result SDL_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) {
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = texture->w;
  int dy1 = texture->h;
  int sx0 = dx0;
  int sy0 = dy0;
  int sx1 = dx1;
  int sy1 = dy1;
  Uint8 *s = (Uint8*)pixels;
  Uint8 *d = (Uint8*)texture->pixels;
  s += sx0*4+sy0*pitch;
  d += dx0*4+dy0*texture->pitch;
  for (int y = dy0; y < dy1; y++) {
    Uint32 *s2 = (Uint32*)s;
    Uint32 *d2 = (Uint32*)d;
    for (int x = dx0; x < dx1; x++) {
      *d2=*s2;
      s2++;
      d2++;
    }
    s += pitch;
    d += texture->pitch;
  }
  return SDL_OK;
}

SDL_Result SDL_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect) {
  if (texture == NULL) return SDL_ERROR;
  unsigned int *dest = glFrameBuffer;
  int destWidth = glFrameBufferWidth;
  int destHeight = glFrameBufferHeight;
  int destPitch = destWidth*4;
  if (renderer->renderTarget!=NULL&&renderer->renderTarget!=texture) { // :mad: (renderer->renderTarget!=texture seems to be needed?)
    dest = renderer->renderTarget->pixels;
    destWidth = renderer->renderTarget->w;
    destHeight = renderer->renderTarget->h;
    destPitch = renderer->renderTarget->pitch;
  }
  if (dest == NULL) return SDL_ERROR;
  int sx0 = 0;
  int sy0 = 0;
  int sx1 = sx0+texture->w;
  int sy1 = sy0+texture->h;
  int dx0 = 0;
  int dy0 = 0;
  int dx1 = destWidth;
  int dy1 = destHeight;
  if (srcrect != NULL) {
    sx0 = srcrect->x;
    sy0 = srcrect->y;
    sx1 = sx0+srcrect->w;
    sy1 = sy0+srcrect->h;
  }
  if (dstrect != NULL) {
    dx0 = dstrect->x;
    dy0 = dstrect->y;
    dx1 = dx0+dstrect->w;
    dy1 = dy0+dstrect->h;
  }
  bool sameSize=((sx1-sx0)==(dx1-dx0))&&((sy1-sy0)==(dy1-dy0));
  int minx = 0;
  int miny = 0;
  int maxx = destWidth;
  int maxy = destHeight;
  if (dx0>=maxx) return SDL_ERROR;
  if (dx1<=minx) return SDL_ERROR;
  if (dy0>=maxy) return SDL_ERROR;
  if (dy1<=miny) return SDL_ERROR;
  if (sameSize) {
    if (dx0<minx) {
      sx0 += minx-dx0;
      dx0 = minx;
    }
    if (dy0<miny) {
      sy0 += miny-dy0;
      dy0 = miny;
    }
    if (dx1>maxx) {
      sx1 -= dx1-maxx;
      dx1 = maxx;
    }
    if (dy1>maxy) {
      sy1 -= dy1-maxy;
      dy1 = maxy;
    }
    for (int y = dy0; y < dy1; y++) {
      const int sy = y - dy0 + sy0;
      if (sy >= 0 && sy < texture->h) {
        Uint32 *d = (Uint32*)((int)dest + y*destPitch);
        for (int x = dx0; x < dx1; x++) {
          const int sx = x - dx0 + sx0;
          if (sx >= 0 && sx < texture->w) {
            d[x] = texture->pixels[sx+sy*texture->w]; // todo: texturepitch
          }
        }
      }
    }
  } else { // sameSize
    double sfx0 = sx0;
    double sfx1 = sx1;
    double sfy0 = sy0;
    double sfy1 = sy1;
    double ax = (double)(sx1-sx0)/(dx1-dx0);
    double ay = (double)(sy1-sy0)/(dy1-dy0);
    if (dx0<minx) {
      sfx0 += (minx-dx0)*ax;
      dx0 = minx;
    }
    if (dy0<miny) {
      sfy0 += (miny-dy0)*ay;
      dy0 = miny;
    }
    if (dx1>maxx) {
      sfx1 -= (dx1-maxx)*ax;
      dx1 = maxx;
    }
    if (dy1>maxy) {
      sfy1 -= (dy1-maxy)*ay;
      dy1 = maxy;
    }
    for (int y = dy0; y < dy1; y++) {
      const int sy = (int)((y - dy0)*ay + sfy0);
      if (sy >= 0 && sy < texture->h) {
        Uint32 *d = (Uint32*)((int)dest + y*destPitch);
        for (int x = dx0; x < dx1; x++) {
          const int sx = (int)((x - dx0)*ax + sx0);
          if (sx >= 0 && sx < texture->w) {
            d[x] = texture->pixels[sx+sy*texture->w]; // todo: texturepitch
          }
        }
      }
    }
  } // sameSize
  return SDL_OK;
}

const char *SDL_GetPixelFormatName(Uint32 format) {
  return "";
}

bool SDL_HasClipboardText() {
  return false;
}

char *SDL_GetClipboardText() {
  return "";
}

bool SDL_HasScreenKeyboardSupport() {
  return false;
}

void SDL_SaveBMP(SDL_Surface *surface, const char *file) {
}

const char *SDL_GetCurrentVideoDriver() {
  return "WatcomGL";
}

const char *SDL_GetCurrentAudioDriver() {
  return "AudioSilence";
}

SDL_Result SDL_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect, Uint32 format, void *pixels, int pitch) {
  return SDL_ERROR;
}

SDL_RWops *SDL_RWFromMem(void *mem, int size) {
  return NULL;
}

SDL_RWops *SDL_RWFromConstMem(const void *mem, int size) {
  return NULL;
}

Sint64 SDL_RWseek(SDL_RWops *context, Sint64 offset, int whence) {
  return 0;
}

SDL_Result SDL_RWclose(SDL_RWops *context) {
  return SDL_ERROR;
}

SDL_Surface *SDL_LoadBMP_RW(SDL_RWops * src, int freesrc) {
  return NULL;
}

#ifdef __cplusplus
}
#endif // __cplusplus

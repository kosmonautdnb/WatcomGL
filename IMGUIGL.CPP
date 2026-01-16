#include "gl.h"
#include "imguigl.hpp"

#ifdef __WATCOMC__
#include <i86.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <fcntl.h>
#include <stdio.h>
#include <io.h>
#include <go32.h>
#include <signal.h>
#include <dos.h>
#endif // __DJGPP__

#define FASTIMGUIGL // define that for non polygonal drawing (rectangles) (ImGui just supports GL_NEAREST sampling for (QUAD)textures if enabled)
#define DISABLECTRLCBREAK // program doesn't close on ctrl c
//#define SHIFTCURSORFORMOUSEWHEEL // enables SHIFT+UP/SHIFT+DOWN as mouse scrollwheel "keys"

extern int glFrameBufferWidth;
extern int glFrameBufferHeight;

double mouseX = 60, mouseY = 60;
double mouseDeltaX = 0, mouseDeltaY = 0;
double mouseSpeed = 1.0;
int mouseButtons = 0;
int currentKey = 0;
GLboolean currentKeyAlt = GL_FALSE;
GLboolean currentKeyCtrl = GL_FALSE;
GLboolean currentKeyShift = GL_FALSE;
char *clipBoardString = NULL;
GLuint g_FontTexture;
ImVec2 imGuiWhitePixel();

static void drawMouseArrow(int mouseX, int mouseY) {
  const int width = 8;
  const int height = 8;
  const char *bitmap =
  "60000000"
  "98900000"
  "98889000"
  "09888990"
  "09888889"
  "00989999"
  "00989000"
  "00099000"
  ;
  //mouseX -= glFrameBufferWidth*0.5f-glFrameBufferWidth*0.5f*glGetZoomX();
  bool first = true;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      char c = bitmap[x+y*width];
      if (c != '0') {
        unsigned int color = 0xffffffff;
        if (c == '9')
          color = 0xff000000;
        if (c == '6')
          color = 0xffffffff;
        glPixel(first,0.5+0.1,0.5+0.1,1,x+mouseX,y+mouseY,color);
        first = false;
      }
    }
  }
}

#ifdef DISABLECTRLCBREAK
#ifdef __WATCOMC__
static void disableControlBreak() {
  union REGS regs;
  regs.h.ah = 0x33;
  regs.h.al = 1;
  regs.h.dl = 0;
  int386(0x21, &regs, &regs);
}
#else // __WATCOMC__
#ifdef __DJGPP__
static void exitProgram(int signal) {
  glDone(); printf("CTRL+BREAK pressed. Sadly it seems there is no docu on how to disable this !properly!.\n"); exit(0);
}
static void disableControlBreak() {
  setmode(fileno(stdin),O_BINARY); // CTRL-C is normal again
  signal(SIGINT,exitProgram); // CTRL-BREAK exits, due to invalid keyboard after CTRL-BREAK
}
#else // __DJGPP__
static void disableControlBreak() {
}
#endif // __DJGPP__
#endif // __WATCOMC__
#else // DISABLECTRLCBREAK
static void disableControlBreak() {
}
#endif // DISABLECTRLCBREAK

typedef struct IMTRI {
  ImDrawVert *v[3];
  int flags;
  bool noTexture;
  bool isTransparentColor;
  ImDrawVert *minv;
  ImDrawVert *maxv;
  ImVec4 ClipRect;
  unsigned int texture;
} IMTRI;

static void drawIMTRI(const IMTRI *t) {
  if (t->v[0] == NULL) return;
  if (t->flags == 0) {
    if (t->noTexture) {
      glDisable(GL_TEXTURE_2D); 
      glDisable(GL_ALPHA_TEST);
      if (t->isTransparentColor) 
        glEnable(GL_BLEND);
      else
        glDisable(GL_BLEND);
    }
    else {
      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glEnable(GL_ALPHA_TEST);
//      glBindTexture(GL_TEXTURE_2D, t->texture);
    }
    const float SUBPIXEL = -0.25f; // the vertices that arrive here from imgui seem to have many times a pixel center of (0.5,0.4), and sometimes also some different ones, -0.25 was just working well here. todo: maybe look into this again
    for (int i = 0; i < 3; i++) {
      ImDrawVert *a = t->v[i];
      glColor4ubv((GLubyte*)&a->col);
      glTexCoord2f(a->uv.x, a->uv.y);
      glVertex2f(a->pos.x+SUBPIXEL,a->pos.y+SUBPIXEL);
    }
  }
}

#ifdef FASTIMGUIGL
static bool isIMRECT(IMTRI *t0, IMTRI *t1) {
  if (t0->v[0] == NULL || t1->v[0] == NULL) return false;
  if (t0->flags != t1->flags) return false;
  if (t0->noTexture != t1->noTexture) return false;
  if (t0->isTransparentColor != t1->isTransparentColor) return false;
  int sameVertices = 0;
  int sameVerticet0[2];
  int sameVerticet1[2];
  const bool hasTexture = !t0->noTexture;
  //if (hasTexture && t0->texture != t1->texture) return false;
  for (int i = 0; i < 3; i++) {
    const ImDrawVert *a = t0->v[i];
    for (int j = 0; j < 3; j++) {
      const ImDrawVert *b = t1->v[j];
      if (a->pos.x != b->pos.x) continue;
      if (a->pos.y != b->pos.y) continue;
      if (a->col != b->col) return false;
      if (hasTexture && (a->uv.x != b->uv.x)) continue;
      if (hasTexture && (a->uv.y != b->uv.y)) continue;
      if (sameVertices>=2) return false;
      sameVerticet0[sameVertices] = i;
      sameVerticet1[sameVertices] = j;
      sameVertices++;
    }
  }
  if (sameVertices == 2) {
    int p0 = 0;
    int p1 = 0;
    for (int i = 1; i < 3; i++) {
      bool foundt0 = true;
      bool foundt1 = true;
      for (int j = 0; j < sameVertices; j++) {
        if (sameVerticet0[j] == i) foundt0 = false;
        if (sameVerticet1[j] == i) foundt1 = false;
      }
      if (foundt0) p0 = i;
      if (foundt1) p1 = i;
    }
    if (hasTexture) {
      double t0x[2],t0y[2];
      double t1x[2],t1y[2];
      t0x[0] = t0->v[(p0+1)%3]->uv.x-t0->v[p0]->uv.x;
      t0x[1] = t0->v[(p0+2)%3]->uv.x-t0->v[p0]->uv.x;
      t0y[0] = t0->v[(p0+1)%3]->uv.y-t0->v[p0]->uv.y;
      t0y[1] = t0->v[(p0+2)%3]->uv.y-t0->v[p0]->uv.y;
      t1x[0] = t1->v[(p1+1)%3]->uv.x-t1->v[p1]->uv.x;
      t1x[1] = t1->v[(p1+2)%3]->uv.x-t1->v[p1]->uv.x;
      t1y[0] = t1->v[(p1+1)%3]->uv.y-t1->v[p1]->uv.y;
      t1y[1] = t1->v[(p1+2)%3]->uv.y-t1->v[p1]->uv.y;
      int found = 0;
      for (int i = 0; i < 2; i++) {
        if (t0x[i] == 0 && t0y[i] != 0) found++;
        if (t0x[i] != 0 && t0y[i] == 0) found++;
        if (t1x[i] == 0 && t1y[i] != 0) found++;
        if (t1x[i] != 0 && t1y[i] == 0) found++;
      }
      if (found != 4) return false;
    }

    double minx,miny,maxx,maxy;
    minx = t0->v[p0]->pos.x; if (t1->v[p1]->pos.x < minx) minx = t1->v[p1]->pos.x;
    miny = t0->v[p0]->pos.y; if (t1->v[p1]->pos.y < miny) miny = t1->v[p1]->pos.y;
    maxx = t0->v[p0]->pos.x; if (t1->v[p1]->pos.x > maxx) maxx = t1->v[p1]->pos.x;
    maxy = t0->v[p0]->pos.y; if (t1->v[p1]->pos.y > maxy) maxy = t1->v[p1]->pos.y;
    for (int k = 0; k < 3; k++) {
      if ( (t0->v[k]->pos.x != minx) && (t0->v[k]->pos.x != maxx) ) return false;
      if ( (t0->v[k]->pos.y != miny) && (t0->v[k]->pos.y != maxy) ) return false;
      if ( (t1->v[k]->pos.x != minx) && (t1->v[k]->pos.x != maxx) ) return false;
      if ( (t1->v[k]->pos.y != miny) && (t1->v[k]->pos.y != maxy) ) return false;
    }
    if (t0->v[p0]->pos.x == minx && t1->v[p1]->pos.x == maxx && t0->v[p0]->pos.y == maxy && t1->v[p1]->pos.y == miny) {
      t0->minv = t0->v[p0]; t1->minv = t0->v[p0];
      t0->maxv = t1->v[p1]; t1->maxv = t1->v[p1];
      return true;
    }
    if (t0->v[p0]->pos.x == maxx && t1->v[p1]->pos.x == minx && t0->v[p0]->pos.y == miny && t1->v[p1]->pos.y == maxy) {
      t0->minv = t1->v[p1]; t1->minv = t1->v[p1];
      t0->maxv = t0->v[p0]; t1->maxv = t0->v[p0];
      return true;
    }
  }
  return false;
}

static void drawIMRECT(const IMTRI *t0, const IMTRI *t1) {
  if (t0->flags != 0) return;
  const float R = ImGui::GetIO().DisplaySize.x;
  const float B = ImGui::GetIO().DisplaySize.y;
  const float SUBPIXEL = 0.f; // seems there is an offset currently ?somehow ImGui specific?
  const float minx = t0->minv->pos.x+SUBPIXEL;
  const float maxx = t0->maxv->pos.x+SUBPIXEL;
  const float miny = t0->maxv->pos.y+SUBPIXEL;
  const float maxy = t0->minv->pos.y+SUBPIXEL;
  if (maxy-miny<=0) return;
  if (maxx-minx<=0) return;

  const int textureWidth = glGetTextureWidth(t0->texture);
  const int textureHeight = glGetTextureHeight(t0->texture);
  unsigned int *texture = glGetTexturePointer(t0->texture);

  const float tx0 = t0->minv->uv.x * textureWidth;
  const float ty0 = t0->maxv->uv.y * textureHeight;
  const float tx1 = t0->maxv->uv.x * textureWidth;
  const float ty1 = t0->minv->uv.y * textureHeight;
  const float txa = (tx1 - tx0)/(maxx-minx);
  const float tya = (ty1 - ty0)/(maxy-miny);
  float tx  = tx0;
  float ty  = ty0;

  int px = (int)minx;
  int py = (int)miny;
  int lx = (int)maxx - px;
  int ly = (int)maxy - py;

  int left = 0;
  int top = 0;
  int right = R;
  int bottom = B;
  if (t0->ClipRect.x+SUBPIXEL > left) left = (int)t0->ClipRect.x+SUBPIXEL;
  if (t0->ClipRect.z+SUBPIXEL < right) right = (int)t0->ClipRect.z+SUBPIXEL;
  if (t0->ClipRect.y+SUBPIXEL > top) top = (int)t0->ClipRect.y+SUBPIXEL;
  if (t0->ClipRect.w+SUBPIXEL < bottom) bottom = (int)t0->ClipRect.w+SUBPIXEL;

  if (px >= right) return;
  if (py >= bottom) return;

  if (px < left) {
    lx -= left-px;
    tx += (left-px) * txa;
    px = left;
  }

  if (py < top) {
    ly -= top-py;
    ty += (top-py) * tya;
    py = top;
  }

  if (px + lx > right) {
    lx -= (px + lx) - right;
  }

  if (py + ly > bottom) {
    ly -= (py + ly) - bottom;
  }

  if (lx <= 0) return;
  if (ly <= 0) return;

  const bool noTexture = (texture == NULL) || t0->noTexture;
 
  const unsigned int color = t0->v[0]->col;
  const int textureWidth2 = textureWidth<<12;
  for (int y = 0; y < ly; y++) {
    unsigned int *write = &glFrameBuffer[(y+py)*glFrameBufferWidth + px];
    if (noTexture && (!t0->isTransparentColor)) {
      for (int x = 0; x < lx; x++) {
        *write++ = color;
      }
    }
    if (noTexture && (t0->isTransparentColor)) {
      const int a = (color>>24) & 255;
      for (int x = 0; x < lx; x++) {
        const unsigned int rgba = *write;
        const int r = ((((color & 255) - (rgba & 255))*a)>>8)+(rgba & 255);
        const int g = (((((color>>8) & 255) - ((rgba>>8) & 255))*a)>>8)+((rgba>>8) & 255);
        const int b = (((((color>>16) & 255) - ((rgba>>16) & 255))*a)>>8)+((rgba>>16) & 255);
        *write++ = r|(g<<8)|(b<<16)|0xff000000;
      }
    }
    if (!noTexture) {
      int txp = (int)(tx*4096.0);
      int txpa = (int)(txa*4096.0);
      int typ = (int)(ty + tya * y);
      if (typ < 0) typ = 0;
      if (typ >= textureHeight) typ = textureHeight-1;
      unsigned int *texp = texture + typ * textureWidth;
      if (color == 0xffffffff) {
        for (int x = 0; x < lx; x++) {
          const unsigned int rgba = *write;
          const unsigned int tex = ((unsigned int)txp<textureWidth2)?texp[txp>>12]:0xffffffff;
          const int a = (tex>>24) & 255;
          const int r = ((((tex & 255) - (rgba & 255))*a)>>8)+(rgba & 255);
          const int g = (((((tex>>8) & 255) - ((rgba>>8) & 255))*a)>>8)+((rgba>>8) & 255);
          const int b = (((((tex>>16) & 255) - ((rgba>>16) & 255))*a)>>8)+((rgba>>16) & 255);
          *write++ = r|(g<<8)|(b<<16)|0xff000000;
          txp += txpa;
        }
      } else {
        for (int x = 0; x < lx; x++) {
          const unsigned int rgba = *write;
          const unsigned int tex = ((unsigned int)txp<textureWidth2)?texp[txp>>12]:0xffffffff;
          const int rc = ((tex & 255)*(color & 255))>>8;
          const int gc = (((tex>>8) & 255)*((color>>8) & 255))>>8;
          const int bc = (((tex>>16) & 255)*((color>>16) & 255))>>8;
          const int a = (((tex>>24) & 255)*((color>>24) & 255))>>8;
          const int r = (((rc - (rgba & 255))*a)>>8)+(rgba & 255);
          const int g = (((gc - ((rgba>>8) & 255))*a)>>8)+((rgba>>8) & 255);
          const int b = (((bc - ((rgba>>16) & 255))*a)>>8)+((rgba>>16) & 255);
          *write++ = r|(g<<8)|(b<<16)|0xff000000;
          txp += txpa;
        }
      }
    }
  }
}
#else // FASTIMGUIGL
static bool isIMRECT(IMTRI *t0, IMTRI *t1) {
  return false;
}
static void drawIMRECT(const IMTRI *t0, const IMTRI *t1) {
  return;
}
#endif // FASTIMGUIGL

static void ImImpl_RenderDrawLists(ImDrawData* draw_data)
{ 
  glPushAttrib(0xffff);
  
  const float L = 0.0f;
  const float R = ImGui::GetIO().DisplaySize.x;
  const float B = ImGui::GetIO().DisplaySize.y;
  const float T = 0.0f;
  const float mvp[4][4] = 
  {
    { 2.0f/(R-L),	0.0f, 0.0f, 0.0f },
    { 0.0f, 2.0f/(T-B), 0.0f, 0.0f },
    { 0.0f, 0.0f, -1.0f, 0.0f },
    { -(R+L)/(R-L), -(T+B)/(T-B), 0.0f, 1.0f },
  };
  
  // Setup orthographic projection
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glLoadMatrixf(&mvp[0][0]);
  //glOrtho(0,R,B,0,-1,1);       

  // Setup render state: alpha-blending enabled, no face culling, no depth testing
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 10.f/255.f);
  //glDisable(GL_BLEND);
  //glDisable(GL_ALPHA_TEST);

  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDepthMask(GL_FALSE);
  glEnable(GL_SCISSOR_TEST);
  
  glEnable(GL_TEXTURE_2D); 
  glBindTexture(GL_TEXTURE_2D, g_FontTexture);

  int viewPort[4]; glGetIntegerv(GL_VIEWPORT,viewPort);
  int width = viewPort[2];
  int height = viewPort[3];
  ImVec2 white = imGuiWhitePixel();

  glVertex4f(0,0,0,1);

  for (int n = 0; n < draw_data->CmdListsCount; n++)
  {
      const ImDrawList* cmd_list = draw_data->CmdLists[n];
      const ImDrawIdx* idx_buffer = &cmd_list->IdxBuffer.front();

      if (cmd_list->VtxBuffer.size() == 0)
        continue;

      ImDrawVert *vtx_data = (ImDrawVert *)&cmd_list->VtxBuffer[0];

      IMTRI lastTri;
      lastTri.v[0] = NULL;

      for (const ImDrawCmd* pcmd = cmd_list->CmdBuffer.begin(); pcmd != cmd_list->CmdBuffer.end(); pcmd++)
      {
          if (pcmd->UserCallback)
          {
              pcmd->UserCallback(cmd_list, pcmd);
          }
          else
          {
              glBindTexture(GL_TEXTURE_2D, (GLuint)pcmd->TextureId);
              glScissor((int)pcmd->ClipRect.x, (int)(height - pcmd->ClipRect.w), (int)(pcmd->ClipRect.z - pcmd->ClipRect.x), (int)(pcmd->ClipRect.w - pcmd->ClipRect.y));
              //glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, GL_UNSIGNED_SHORT, idx_buffer);
                glBegin(GL_TRIANGLES);
                for (int j = 0; j < pcmd->ElemCount; j+=3) {
                  int i;
                  bool noTexture = false;
                  int flags = 1 | 2 | 4 | 8 | 16;
                  bool isTransparentColor = false;
                  for (i = j; i < j + 3; i++) {
                    ImDrawVert &a = vtx_data[idx_buffer[i]];
                    if (a.uv.x==white.x&&a.uv.y==white.y) noTexture = true;
                    if (a.pos.x >= pcmd->ClipRect.x) flags &= 31-1;
                    if (a.pos.y >= pcmd->ClipRect.y) flags &= 31-2;
                    if (a.pos.x < pcmd->ClipRect.z) flags &= 31-4;
                    if (a.pos.y < pcmd->ClipRect.w) flags &= 31-8;
                    if (((a.col>>24)&255) > 0) flags &= 31 - 16;
                    if (((a.col>>24)&255) < 250) isTransparentColor = true;
                  }
                  IMTRI t;
                  t.flags = flags;
                  t.noTexture = noTexture;
                  t.isTransparentColor = isTransparentColor;
                  t.v[0] = &vtx_data[idx_buffer[j+0]];
                  t.v[1] = &vtx_data[idx_buffer[j+1]];
                  t.v[2] = &vtx_data[idx_buffer[j+2]];
                  t.texture = (GLuint)pcmd->TextureId;
                  if (isIMRECT(&t,&lastTri)) {
                    t.ClipRect = pcmd->ClipRect; lastTri.ClipRect = pcmd->ClipRect;
                    drawIMRECT(&t,&lastTri);
                    lastTri.v[0] = NULL;
                  } else {
                    drawIMTRI(&lastTri);
                    lastTri = t;
                  }
                }
                drawIMTRI(&lastTri);
                lastTri.v[0] = NULL;
                glEnd();
          }
          idx_buffer += pcmd->ElemCount;
      }
  }

  glDisable(GL_BLEND);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_SCISSOR_TEST);
  glScissor(0,0,glFrameBufferWidth,glFrameBufferHeight);
  drawMouseArrow(mouseX, mouseY);
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glPopAttrib();
}

static const char* ImImpl_GetClipboardTextFn()
{
  return clipBoardString;
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
  const char *text_end = text + strlen(text);
  if (clipBoardString != NULL) {delete[] clipBoardString; clipBoardString = NULL;}
  clipBoardString = new char[text_end - text + 1];
  memcpy(clipBoardString, text, text_end-text);
  clipBoardString[text_end-text] = 0;
}

void ImGui_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bits for OpenGL3 demo because it is more likely to be compatible with user's existing shader.

    glGenTextures(1, &g_FontTexture);
    glBindTexture(GL_TEXTURE_2D, g_FontTexture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    // Store our identifier
    io.Fonts->TexID = (void *)g_FontTexture;

    // Cleanup (don't clear the input data if you want to append new fonts later)
    io.Fonts->ClearInputData();
    io.Fonts->ClearTexData();
}

static int translate(int c) {
  switch(c) {
  case GL_VK_TAB_SHIFT: c = GL_VK_TAB; break;
  case GL_VK_LEFT_CTRL: c = GL_VK_LEFT; break;
  case GL_VK_RIGHT_CTRL: c = GL_VK_RIGHT; break;
  }
  if (c > 255) return (c >> 8)+256;
  if (c < 32) return c + 512-32;
  return c;
}

void InitImGui(int w, int h)
{
  disableControlBreak();

  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)w, (float)h); // sadly this +1 is needed it seems (font rendering)
  io.DeltaTime = 1.0f/60.0f;
  io.KeyMap[ImGuiKey_Tab] = translate(GL_VK_TAB);
  io.KeyMap[ImGuiKey_LeftArrow] = translate(GL_VK_LEFT);
  io.KeyMap[ImGuiKey_RightArrow] = translate( GL_VK_RIGHT);
  io.KeyMap[ImGuiKey_UpArrow] = translate(GL_VK_UP);
  io.KeyMap[ImGuiKey_DownArrow] = translate(GL_VK_DOWN);
  io.KeyMap[ImGuiKey_Home] = translate(GL_VK_HOME);
  io.KeyMap[ImGuiKey_End] = translate(GL_VK_END);
  io.KeyMap[ImGuiKey_Delete] = translate(GL_VK_DELETE);
  io.KeyMap[ImGuiKey_Backspace] = translate(GL_VK_BACKSPACE);
  io.KeyMap[ImGuiKey_Enter] = translate(GL_VK_ENTER);
  io.KeyMap[ImGuiKey_Escape] = translate(GL_VK_ESCAPE);
  io.KeyMap[ImGuiKey_PageUp] = translate(GL_VK_PAGEUP);
  io.KeyMap[ImGuiKey_PageDown] = translate(GL_VK_PAGEDOWN);
  io.KeyMap[ImGuiKey_A] = translate(GL_VK_A_CTRL);
  io.KeyMap[ImGuiKey_Z] = translate(GL_VK_Z_CTRL);
  io.KeyMap[ImGuiKey_Y] = translate(GL_VK_Y_CTRL);
  io.KeyMap[ImGuiKey_C] = translate(GL_VK_C_CTRL);
  io.KeyMap[ImGuiKey_V] = translate(GL_VK_V_CTRL);
  io.KeyMap[ImGuiKey_X] = translate(GL_VK_X_CTRL);
  
  io.RenderDrawListsFn = ImImpl_RenderDrawLists;
  io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
  io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
  
  // Load font texture
  ImGui_CreateFontsTexture();
  
  io.IniSavingRate = -1; // disable ini saving, because we are on usb sticks which may wear out
  ImGuiStyle &s = ImGui::GetStyle();
  s.WindowRounding = 5.f * 0; // rounding disables FASTIMGUIGL optimizations for larger parts of the windows
  s.FramePadding = ImVec2(2,2);
  s.ItemSpacing = ImVec2(3,3);
  s.WindowPadding = ImVec2(3,3);
  s.Colors[ImGuiCol_WindowBg] = ImVec4(0,0,0,100.0);
  s.Colors[ImGuiCol_TitleBg] = ImVec4(0x80/255.f,0,0x40/255.f,1.f);
  s.Colors[ImGuiCol_TitleBgActive] = ImVec4(0xff/255.f,0,0x80/255.f,1.f);
  s.Colors[ImGuiCol_CloseButton] = ImVec4(0xff/255.f*0.15f,0,0x40/255.f*0.0f,1.f);
  s.Colors[ImGuiCol_Button] = ImVec4(0xff/255.f*0.45f,0,0x40/255.f*0.2f,1.f);
  s.Colors[ImGuiCol_Text] = ImVec4(1,1,1,1);
  s.Colors[ImGuiCol_FrameBg] = ImVec4(0.3,0.3,0.3,1);
  // these where added for EGA
  s.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.8,0.8,0.8,1);
  s.Colors[ImGuiCol_CheckMark] = ImVec4(0.0,1.0,1.0,1);
  s.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.5,0.5,1.0,1);
}

static void ImGuiKeyBoardInput() {
  currentKey = glNextKey();
  glSpecialKeys(&currentKeyShift,&currentKeyCtrl,&currentKeyAlt);

#ifdef SHIFTCURSORFORMOUSEWHEEL
  if (currentKeyShift) {
    if (currentKey == GL_VK_UP) {currentKeyShift=false; ImGui::GetIO().MouseWheel = 1; currentKey = 0;}
    if (currentKey == GL_VK_DOWN) {currentKeyShift=false; ImGui::GetIO().MouseWheel = -1; currentKey = 0;}
  }
#endif

  for (int i = 0; i < 512; i++)
    ImGui::GetIO().KeysDown[i]=false;

  if (currentKey != 0) {
    int c = translate(currentKey);
  
    if (c<256) {
      ImGui::GetIO().AddInputCharacter(c);
      ImGui::GetIO().KeysDown[c]=true;
    } else  
      ImGui::GetIO().KeysDown[c]=true;
  }

  ImGui::GetIO().KeyShift = currentKeyShift;
  ImGui::GetIO().KeyCtrl = currentKeyCtrl;
  ImGui::GetIO().KeyAlt = currentKeyAlt;
}

void ImGuiNewFrame() {
  ImGuiIO& io = ImGui::GetIO();
  static double lastTime = glSeconds();
  double time = glSeconds();
  double deltaTime = time - lastTime;
  io.DeltaTime = deltaTime > 0 ? deltaTime : 1.f/30.f;
  lastTime = time;
  mouseButtons = glMouseButtons();
  glNextMouseDelta(&mouseDeltaX, &mouseDeltaY);
  mouseX += mouseDeltaX * mouseSpeed; mouseY += mouseDeltaY * mouseSpeed;
  if (mouseX < 0) mouseX = 0;
  if (mouseY < 0) mouseY = 0;
  if (mouseX > glFrameBufferWidth-1) mouseX = glFrameBufferWidth-1;
  if (mouseY > glFrameBufferHeight-1) mouseY = glFrameBufferHeight-1;
  io.MousePos = ImVec2(mouseX,mouseY);
  io.MouseDown[0] = mouseButtons & 1;
  io.MouseDown[1] = mouseButtons & 2;
  io.MouseWheel = (signed char)((mouseButtons>>8)&0xff);
  ImGuiKeyBoardInput();
  ImGui::NewFrame();
}

static int XRES = 640;
static int YRES = 480;

void ImGuiDemo() {
  glVesa(XRES,YRES,32);

  InitImGui(XRES, YRES);

  do
  {
    ImGuiNewFrame();

    // 2) ImGui usage
    static bool show_test_window = false;
    static bool show_another_window = false;
    static float f;
    ImGui::Text("Hello, world!");
    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
    if (ImGui::Button("Test Window")) show_test_window = !show_test_window;
    if (ImGui::Button("Another Window")) show_another_window = !show_another_window;
    
    // Calculate and show framerate
    #define BUCKETS 10
    static float ms_per_frame[BUCKETS] = { 0 };
    static int ms_per_frame_idx = 0;
    static float ms_per_frame_accum = 0.0f;
    ms_per_frame_accum -= ms_per_frame[ms_per_frame_idx];
    ms_per_frame[ms_per_frame_idx] = ImGui::GetIO().DeltaTime * 1000.0f;
    ms_per_frame_accum += ms_per_frame[ms_per_frame_idx];
    ms_per_frame_idx = (ms_per_frame_idx + 1) % BUCKETS;
    const float ms_per_frame_avg = ms_per_frame_accum / BUCKETS;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", ms_per_frame_avg, 1000.0f / ms_per_frame_avg);
    
    if (show_test_window)
    {     
      // More example code in ShowTestWindow()
      ImGui::ShowTestWindow(&show_test_window);
    }
    
    if (show_another_window)
    {
      ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
      ImGui::Text("Hello");
      static char buffer[100];
      ImGui::InputText("Hello", buffer, 100);
      ImGui::End();
    }

    glClearColor(10.f/255.f,40.f/255.f,30.f/255.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glRefresh();
  } while(currentKey!=GL_VK_ESCAPE);
  glDone();
}
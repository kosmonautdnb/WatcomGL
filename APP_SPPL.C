/*
  Speaker Tracker Player
  --
  A simple player for PC Speaker (the Beeper) Music created with Speaker Tracker. It's for the exported format of Speaker Tracker.
  --
*/
/*
  MIT License

  Copyright (c) 2026 Stefan Mader

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
// WatcomGL can be found here: https://github.com/kosmonautdnb/WatcomGL
// Some additional Libraries for WatcomC can be found here: https://github.com/kosmonautdnb/WatcomLibs

#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __WATCOMC__
#include <i86.h>
#include <dos.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#endif // __DJGPP__

unsigned int timerIrqTicks = 0;

#define SONGHEADERSIZE 5
unsigned short songHeader[SONGHEADERSIZE] = {0};
unsigned short *patternData = NULL;

#define TIMERTICKS (songHeader[0])
#define SONGLENGTH (songHeader[1])
#define PATTERNLENGTH (songHeader[2])
#define LOOPSTART (songHeader[3])
#define LOOPEND (songHeader[4])
#define PATTERNDATALENGTH (SONGLENGTH*PATTERNLENGTH*TIMERTICKS)

volatile int playPos = 0;
volatile int finished = 0;
volatile int speaker_on = 0;

void speaker_freq_direct(int a) {
  outp(0x43,0xb6); // set speaker timer 2 to square wave
  outp(0x42,a & 255);
  outp(0x42,(a>>8) & 255);
}

void speaker_tone_on() {
  unsigned char p61 = inp(0x61);
  p61 |= 0x03; // bit 0 = timer 2 output to speaker, bit 1 = speaker data on
  outp(0x61,p61);
}

void speaker_tone_off() {
  unsigned char p61 = inp(0x61);
  p61 &= 255 - 0x03; // bit 0 = timer 2 output to speaker, bit 1 = speaker data on
  outp(0x61,p61);
}

void speaker_tone_on2() {
  if (!speaker_on) {speaker_on = 1; speaker_tone_on();}
}

void speaker_tone_off2() {
  if (speaker_on) {speaker_on = 0; speaker_tone_off();}
}

void speaker_freq_direct2(int a) {
  static int lastFreq = 0;
  if (a != lastFreq) {lastFreq = a; speaker_freq_direct(a);}
}

static void timerIrqHandler() {
  unsigned short a;
  if (finished) 
    speaker_tone_off2();
  else {
    if (playPos<0||playPos>=PATTERNDATALENGTH) {speaker_tone_off2();return;}
    a = patternData[playPos];
    if (a == 0) 
      speaker_tone_off2();
    else {
      speaker_freq_direct2(a);
      speaker_tone_on2();
    }
    playPos++;
    if (playPos == LOOPEND*PATTERNLENGTH*TIMERTICKS) playPos = LOOPSTART*PATTERNLENGTH*TIMERTICKS;
    if (playPos >= PATTERNDATALENGTH) {
      speaker_tone_off2(); 
      finished = 1;
    }
  }
  timerIrqTicks++;
}

// >>>> --- machine near stuff for WatcomC and DJGPP
#ifdef __WATCOMC__
void uninstallTimerIrqHandler();
typedef void (__interrupt __far* TimerIrqHandler)();
static TimerIrqHandler oldTimerIrqHandler = NULL;
static void (__interrupt __far newTimerIrqHandler) (void) {
  timerIrqHandler();
  oldTimerIrqHandler();
}                                               
void installTimerIrqHandler() {
  if (oldTimerIrqHandler != NULL)
    uninstallTimerIrqHandler();
  oldTimerIrqHandler = _dos_getvect(0x08);
  _dos_setvect(0x08,newTimerIrqHandler);
  atexit(uninstallTimerIrqHandler);
}
void uninstallTimerIrqHandler() {
  if (oldTimerIrqHandler != NULL) {
    _dos_setvect(0x08,oldTimerIrqHandler);
    oldTimerIrqHandler = NULL;
  }
}
#endif // __WATCOMC__
#ifdef __DJGPP__
#define LOCK_VARIABLE(x) _go32_dpmi_lock_data((void*)&x,(long)sizeof(x));
#define LOCK_FUNCTION(x,__s__) _go32_dpmi_lock_code((void*)x,(long)__s__);
static _go32_dpmi_seginfo old_timerIrq_handler;
static _go32_dpmi_seginfo new_timerIrq_handler;
static bool timerIrq_handler_installed = false;
void uninstallTimerIrqHandler() {
  if (timerIrq_handler_installed) {
    timerIrq_handler_installed = false;
    _go32_dpmi_set_protected_mode_interrupt_vector(0x08,&old_timerIrq_handler);
  }
}
void installTimerIrqHandler() {
  if (timerIrq_handler_installed) 
    uninstallTimerIrqHandler();
  _go32_dpmi_get_protected_mode_interrupt_vector(0x08,&old_timerIrq_handler); 
  timerIrq_handler_installed = true;
  new_timerIrq_handler.pm_offset = (unsigned long)timerIrqHandler;
  new_timerIrq_handler.pm_selector = _go32_my_cs();
  _go32_dpmi_chain_protected_mode_interrupt_vector(0x08,&new_timerIrq_handler);
  LOCK_FUNCTION(timerIrqHandler,2048);
  LOCK_VARIABLE(timerIrqTicks);
  LOCK_VARIABLE(playPos);
  LOCK_VARIABLE(finished);
  LOCK_VARIABLE(speaker_on);
}
#endif //__DJGPP__

#ifdef __WATCOMC__
unsigned short int16_1(); // we need the zero flag it's not well exposed by the watcom api sorry that's why some asm here
#pragma aux int16_1="mov ah,0x01""int 0x16""jz do""mov ax,0""jmp dont""do:""mov ax,1""dont:" value[ax] modify[ax bx cx dx]
#endif
#ifdef __DJGPP__
unsigned short int16_1() {
  unsigned short ret;
  __asm__ __volatile__("\tmovb $1,%%ah\n""\tint $0x16\n""\tjz 1f\n""\tmovw $0,%%ax\n""\tjmp 2f\n""1:\n""\tmovw $1,%%ax\n""2:\n" : "=a" (ret));
  return ret;
}
#endif

// <<<< --- machine near stuff for WatcomC and DJGPP

void playMusicTillKeypressOrSongEnd() {
  playPos = 0;
  installTimerIrqHandler();
  while(int16_1()) {
    if (finished) break;
  }
  if (!int16_1()) getch();
  uninstallTimerIrqHandler();
  speaker_tone_off();
}


#define BUFFLEN 2048
char buffer[BUFFLEN] = {0};

int loadSong(const char *fileName) {
  FILE *in;
  int songPos = 0;
  int header = 1;
  int v;
  int patternPos = 0;
  const char *token;

  in = fopen(fileName,"r");
  if (in == NULL) {
    printf("Speaker Tracker Player error: file<%s> not found.\n",fileName);
    exit(0);
  }
  while(fgets(buffer,BUFFLEN,in)) {
    if (header) {
      if (strcmp(buffer,"// SPEAKER TRACKER FILE\n")!= 0) {
        printf("Speaker Tracker Player error: file<%s> header is not \"// SPEAKER TRACKER FILE\"\n",fileName);
        printf("it is:<%s>\n", buffer);
        exit(0);
      }
      header = 0;
    }
    if (buffer[0] == 0 || buffer[0] == '\n' || (buffer[0]=='/' && buffer[1]=='/')) 
      continue;
    if (buffer[strlen(buffer)-1]=='\n') buffer[strlen(buffer)-1]=0;
    if (songPos < SONGHEADERSIZE) {
      // read header
      if (sscanf(buffer,"%d,",&v)==1) {
        //printf("%d\n",v);
        songHeader[songPos] = v;
      }
      songPos++;
      if (songPos == SONGHEADERSIZE) {
        patternData = (unsigned short *)malloc(PATTERNDATALENGTH*sizeof(unsigned short));
      }
    } else {
      // read pattern data
      token = strtok(buffer,",");
      while (token != NULL) {
        if (patternPos < PATTERNDATALENGTH) {
          patternData[patternPos] = atoi(token);
        } else {
          //printf("%s\n",token);
        }
        token = strtok(NULL,",");
        patternPos++;
      }
    }
  }
  fclose(in);
  return (patternPos == (PATTERNDATALENGTH+1)) ? 1 : 0;
}

void freeSong() {
  if (patternData != NULL) {free(patternData); patternData = NULL;}
}
                                                      
int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("------------------------------\n");
    printf("Speaker Tracker Player v1.0\n");
    printf("------------------------------\n");
    printf("Usage: player.exe <song.raw>\n");
    printf("<song.raw> is a songfile exported by Speaker Tracker v1.0\n");
    printf("------------------------------\n");
    exit(0);
  }
  if (!loadSong(argv[1])) {
    printf("Speaker Tracker Player error: file<%s> some problem with the song data.\n",argv[1]);
    freeSong();
    exit(0);
  }
  playMusicTillKeypressOrSongEnd();
  freeSong();
  return 0;
}

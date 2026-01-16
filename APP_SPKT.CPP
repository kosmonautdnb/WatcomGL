/*
  Speaker Tracker
  --
  This is a simple demo app that may be used as a tracker for the PC-Speaker (the beeper).
  It outputs one channel / frequency at a time and doesn't reprogram the timers speed, since that may be problematic with DOS if not done 100% propper.
  It's just fastcoded and not clean to look at...
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "GL.H"
#include "IMGUI.H"
#include "IMGUIGL.HPP"
#include "STRING.HPP"
#include "DOS.HPP"
#include "SPEAKER.HPP"
#include "FILEDLG.HPP"
#include "GLEGA.HPP"

#ifdef __WATCOMC__
#include <i86.h>
#include <dos.h>
#include <conio.h>
#endif // __WATCOMC__
#ifdef __DJGPP__
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#endif // __DJGPP__

#define XRES 640
#define YRES 480
#define WINDOWHEIGHT 410
#define ALSOUSEADLIB

#define NOTE_C4 261.63
#define CHANNELS 3
#define CENTS 100
#define MAXSONGLENGTH 100
#define MAXPATTERNS 1000
#define MAXPATTERNLENGTH 128
#define MAXINSTRUMENTS 100
#define MAXINSTRUMENTLENGTH 32
#define MAXTICKS 5
#define MINCENTS (-12*CENTS*10)
#define MAXCENTS (12*CENTS*10)
#define VK_INSERT (82+256)

volatile int shouldBeep = 0;
volatile double shouldBeepFrequency = 0;
volatile int timerIrqTicks = 0;
volatile bool player_isPlaying = false;
volatile bool player_isEditMode = true;
volatile bool player_initial = false;
volatile bool player_scroll = false;
volatile bool player_patternLoop = false;
volatile int player_editInstrumentPos = 0;
volatile int player_trackPos = 0;
volatile int player_patternPos = 0;
volatile int player_fullPos = 0;
volatile int player_timerTicks = 0;
volatile int player_ticker = 0;
volatile int player_mute = 0;
volatile bool player_useAdlib = false;
volatile double player_hzAdlib = 0;
bool player_requestSongUpdate = false;
volatile int scrollTo = -1;

typedef struct PlayNote {
  unsigned short freq; // or 0
} PlayNote;

#define PLAYERMAXSONGLENGTHRAW (MAXSONGLENGTH*MAXPATTERNLENGTH)
#define PLAYERMAXSONGLENGTH (MAXSONGLENGTH*MAXPATTERNLENGTH*MAXTICKS)

typedef struct INSTRUMENT {
  int loopStart;
  int loopEnd;
  int cents[MAXINSTRUMENTLENGTH];
} INSTRUMENT;

typedef struct PlayData {
  int timerTicks;
  int songLength;
  int patternLength;
  int loopStart;
  int loopEnd;
  PlayNote song[PLAYERMAXSONGLENGTH];
  INSTRUMENT editInstrument;
  double editInstrumentCents;
} PlayData;

PlayData player_currentData;
PlayData player_nextData;

volatile bool player_speakerOn = false;
volatile int player_speakerVal = 0;

// >>>>------ ADLIB

#define WAITPORT 0x388
#define ADLIBPORT 0x388
#define ADLIBDATAPORT (ADLIBPORT+1)

void selectAdlibReg(unsigned char reg) {
#ifdef ALSOUSEADLIB
  outp(ADLIBPORT,reg);
  for (int i = 0; i < 6; i++) inp(WAITPORT);  // wait 12 clock cycles
#endif // ALSOUSEADLIB
}

void writeAdlibData(unsigned char data) {
#ifdef ALSOUSEADLIB
  outp(ADLIBDATAPORT,data);
  for (int i = 0; i < 6*7; i++) inp(WAITPORT);  // wait 84 clock cycles
#endif // ALSOUSEADLIB
}

void adlib(unsigned char reg, unsigned char value) {selectAdlibReg(reg);writeAdlibData(value);}

void clearAdlib() {
  adlib(1,0x20); // common initialization step
  for (int i = 0xf5; i >= 1; i--)
    adlib(i,0);
  adlib(1,0x20); // common initialization step
}

void adlib_tone_freq2(double hz, bool keyOn) {
  if (player_useAdlib) {
    player_hzAdlib = hz;
    // fnum[0-1023]=hz*pow(2,20-block)/49716;
    int block = 0;
    unsigned int fnum = 0;
    bool found = false;
    for (block = 7; block>=0; block--) {
      fnum = (int)(hz*(1<<(20-block))/49715.90277); // 3579545.0/72.0 = MasterClockFrequency/72 = 49715.90277
      // frequency = fnum * 2^block * (master_clock/72) / 2^20
      // fnum = frequency * 2^20 / (2^block * (master_clock/72))
      if ((fnum>=0)&&(fnum<=1023)) {
        found = true;
        break;
      }
    }
    if (found) {
      int on = keyOn ? 1 : 0;
      adlib(0xa0,fnum & 0xff); // Operator 0, Channel 0 // lower frequency bits
      adlib(0xb0,(fnum>>8)|(block<<2)|(on<<5)); // Operator 0, Channel 0 // higher frequency bits (+block + keyon)
    }
  }
}

void adlib_tone_on() {
  if (player_useAdlib) {
    adlib_tone_freq2(player_hzAdlib, true);
    adlib(0x40,31); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
    adlib(0x43,0); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
  }
}

void adlib_tone_off() {
  // adlib_tone_off is always called (no player_useAdlib here)
  double k = player_hzAdlib;
  adlib_tone_freq2(0, false);
  player_hzAdlib = k;
  adlib(0x40,63); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
  adlib(0x43,63); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
}

void startAdlibSpeakerMode() {
  int attack = 0x0f;
  int decay = 0x00;
  int sustain = 0x0f;
  int release = 0x00;
  // github.com/IS4Code/pcsconv
  // modulator
  adlib(0x20,1); // Operator 0, Channel 0 // Modulator/Carrier Register multiple 1 is standard here
  adlib(0x40,0); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
  adlib(0x60,(attack<<4)|(decay & 0x0f));
  adlib(0x80,(sustain<<4)|(release & 0x0f));
  adlib(0xe0,0); // Operator 0, Channel 0 // Waveform Register, this needs bit 5 in register 1 to take effect, else simple sine 0 = sine wave
  // carrier
  adlib(0x23,1); // Operator 0, Channel 0 // Modulator/Carrier Register multiple 1 is standard here
  adlib(0x43,0); // Operator 0, Channel 0 // Volume 0 is loudest, 63 is almost off
  adlib(0x63,(attack<<4)|(decay & 0x0f));
  adlib(0x83,(sustain<<4)|(release & 0x0f));
}

// <<<<------ ADLIB


void tone_on() {
  if (player_useAdlib) {
    adlib_tone_on();
    speaker_tone_off();
  } else {
    speaker_tone_on();
    adlib_tone_off();
  }
}

void tone_off() {
  adlib_tone_off();
  speaker_tone_off();
}

void speaker_tone_on2() {
  if (!player_speakerOn) {player_speakerOn = true; tone_on();}
}

void speaker_tone_off2() {
  if (player_speakerOn) {player_speakerOn = false; tone_off();}
}

void speaker_tone_freq2(double hz) {
  if (hz == 0) return;
  int a = (int)(1193180 / hz);
  if (a != player_speakerVal) {
    if (player_useAdlib) {
      adlib_tone_freq2(hz, false);
    } else {
      outp(0x43,0xb6);
      outp(0x42,a & 255);
      outp(0x42,(a>>8) & 255);
    }
  }
  player_speakerVal = a;
}

void speaker_tone_direct(int freq) {
  if (freq!=0) speaker_tone_freq2((double)1193180/freq);
//  int a = freq;
//  if (a != player_speakerVal) {
//    outp(0x43,0xb6);
//    outp(0x42,a & 255);
//    outp(0x42,(a>>8) & 255);
//  }
//  player_speakerVal = a;
}


static void timerIrqHandler() {
  if (player_mute != 0) {
    speaker_tone_off2();
    timerIrqTicks++;
    return;
  }
  if (shouldBeep>0) {
    if (player_editInstrumentPos < 0) player_editInstrumentPos = 0;
    if (player_editInstrumentPos >= MAXINSTRUMENTLENGTH) player_editInstrumentPos = MAXINSTRUMENTLENGTH-1;
    double centAdd = player_currentData.editInstrument.cents[player_editInstrumentPos];
    player_editInstrumentPos++;
    if (player_editInstrumentPos >= player_currentData.editInstrument.loopEnd) {
      player_editInstrumentPos = player_currentData.editInstrument.loopStart;
    }
    if (centAdd < MINCENTS) centAdd = MINCENTS;
    if (centAdd > MAXCENTS) centAdd = MAXCENTS;
    double cents = player_currentData.editInstrumentCents;
    if (centAdd <= MINCENTS) {
      speaker_tone_off2();     
    } else {
      int centsHere = (int)(cents + centAdd);
      double freq = NOTE_C4*pow(2.0,1.0/(12.0*CENTS)*centsHere);
      speaker_tone_freq2(freq);
      speaker_tone_on2();
    }
    shouldBeep--;
    if (shouldBeep==0) speaker_tone_off2();
  } else {
    if (player_isPlaying) {
      player_ticker--;
      if (player_ticker<=0 || player_ticker>1000 || player_initial) {
        player_ticker = player_currentData.timerTicks;
        if (!player_initial) 
          player_patternPos++; 
        player_initial = false;      
        if (player_patternPos>=player_currentData.patternLength) {
          player_patternPos = 0;
          if (!player_patternLoop) {
            player_trackPos++;
            if (player_trackPos == player_currentData.loopEnd) player_trackPos = player_currentData.loopStart;
            if (player_trackPos>=player_currentData.songLength) player_trackPos = 0;
            if (player_trackPos<0) player_trackPos = 0;
          }
        }
        if (player_trackPos<0) player_trackPos = 0;
        if (player_trackPos>=player_currentData.songLength) player_trackPos = player_currentData.songLength-1;
        if (player_patternPos<0) player_patternPos = 0;
        if (player_patternPos>=player_currentData.patternLength) player_patternPos=player_currentData.patternLength-1;
        if (player_scroll) scrollTo = player_patternPos;
        player_fullPos = player_trackPos*player_currentData.patternLength+player_patternPos;
        if (player_fullPos < 0) player_fullPos = 0;
        if (player_fullPos >= PLAYERMAXSONGLENGTHRAW) player_fullPos = PLAYERMAXSONGLENGTHRAW-1;
        player_fullPos *= player_timerTicks;
      }
      PlayNote n = player_currentData.song[player_fullPos];
      player_fullPos++;
      int freq = n.freq;
      if (freq == 0) {
        speaker_tone_off2();
      } else {
        speaker_tone_direct(freq);
        speaker_tone_on2();
      }
    } else {
      speaker_tone_off2();
    }
  }
  timerIrqTicks++;
}

void fullStop() {
  player_isPlaying = false;
  shouldBeep = 0;
  speaker_tone_off2();
  speaker_tone_off();
  adlib_tone_off();
}

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
  LOCK_VARIABLE(shouldBeep);
  LOCK_VARIABLE(shouldBeepFrequency);
  LOCK_VARIABLE(timerIrqTicks);
  LOCK_VARIABLE(player_currentData);
  LOCK_VARIABLE(player_isPlaying);
  LOCK_VARIABLE(player_isEditMode);
  LOCK_VARIABLE(player_initial);
  LOCK_VARIABLE(player_scroll);
  LOCK_VARIABLE(player_patternLoop);
  LOCK_VARIABLE(player_trackPos);
  LOCK_VARIABLE(player_fullPos);
  LOCK_VARIABLE(player_patternPos);
  LOCK_VARIABLE(player_timerTicks);
  LOCK_VARIABLE(player_ticker);
  LOCK_VARIABLE(scrollTo);
  LOCK_VARIABLE(player_mute);
  LOCK_VARIABLE(player_editInstrumentPos);
  LOCK_VARIABLE(player_useAdlib);
  LOCK_VARIABLE(player_hzAdlib);
}
#endif //__DJGPP__

float BPM = 160.f;

int SONGLENGTH = 1;
int PATTERNLENGTH = 32;

int patterns[CHANNELS][MAXSONGLENGTH] = {0};

typedef struct PATTERN {
  int note;
  int effect;
} PATTERN;

PATTERN pat[MAXPATTERNS][MAXPATTERNLENGTH];

INSTRUMENT instruments[MAXINSTRUMENTS];

const double timerFrequency = 1193180.0 / 65536.0;
const double oneBPMHz = 1.0 / 60.0;
const double ticksPerBPM = 1.0/4.0;

#define SONGNAMELENGTH 20
char songName[SONGNAMELENGTH] = {"<New song>"};

#define AUTHORNAMELENGTH 16
char authorName[AUTHORNAMELENGTH] = {"<Author>"};

int loopBegin = -1;
int loopEnd = -1;

ImVec4 DescColumnCol(0.5,0.5,0.5,1);
ImVec4 PlayColumnCol(1.0,0,0,1);
ImVec4 WhiteColumnCol(1.0,1.0,1.0,1);
ImVec4 SelectedColumnCol(0.0,0.5,1.0,1);
ImVec4 InvalidCol(0.2,0.2,0.2,1);

int selectionStartPattern = -1;
int selectionStartPos = -1;
int selectionStartSide = -1;
int selectionEndPos = -1;
int selectionEndSide = -1;

int currentOctave = 4;
int currentCents = 0;
int currentInstrument = 0;

PATTERN copyPasteBuffer[MAXPATTERNLENGTH];
int copyPasteSideStart = 0;
int copyPasteSideEnd = 0;
int copyPastePosStart = 0;
int copyPastePosEnd = 0;

int copyPatternFrom = 0;
int copyPatternTo = 0;
bool oneShotNotes = false;

bool exportFile = false;
bool loadFile = false;
bool saveFile = false;

String exportFileName = "song.raw";
String saveFileName = "song.spk";
String loadFileName = "song.spk";

int setShouldBeep = 0;

int bpmToTimerTicks(double bpm) {
  if (bpm <= 1) return -1;
  return (int)floor(timerFrequency/(bpm*oneBPMHz)*ticksPerBPM+0.5);
}

double timerTicksToBPM(int ticks) {
  return ((double)timerFrequency/((double)ticks/ticksPerBPM))/oneBPMHz;
}

double correctBPM(double BPM) {
  player_timerTicks = bpmToTimerTicks(BPM);
  if (player_timerTicks < 1) player_timerTicks = 1;
  if (player_timerTicks > MAXTICKS) player_timerTicks = MAXTICKS;
  return timerTicksToBPM(player_timerTicks);
}

double getPlayTime(int songPos, int patternPos) {
  double v = songPos * PATTERNLENGTH + patternPos;
  return (v * player_timerTicks) * (1.0 / timerFrequency);
}

int getPlayPatternPos() {
  int pos = player_patternPos;
  if (pos < 0) pos = 0;
  if (pos >= PATTERNLENGTH) pos = PATTERNLENGTH-1;
  return pos;
}

void setPlayPatternPos(int pos) {
  if (pos < 0) pos = 0;
  if (pos >= PATTERNLENGTH) pos = PATTERNLENGTH-1;
  player_patternPos = pos;
}

int getPlayTrackPos() {
  int pos = player_trackPos;
  if (pos >= SONGLENGTH) pos = SONGLENGTH-1;
  if (pos < 0) pos = 0;
  return pos;
}

void setPlayTrackPos(int pos) {
  if (pos >= SONGLENGTH) pos = SONGLENGTH-1;
  if (pos < 0) pos = 0;
  player_trackPos = pos;
}

void setPlaying(bool onOff) {
  player_isPlaying = onOff;
  if (player_isPlaying) {player_ticker = 0; player_initial = true;}
}

void playFromCursor() {
  setPlayPatternPos(selectionStartPos);
  setPlaying(true);
}

int getCurrentPatternForColumn(int column) {
  int songPos = getPlayTrackPos();
  if ((songPos < 0) || (songPos > SONGLENGTH)) return 0;
  int pattern = patterns[column][songPos];
  if (pattern < 0) pattern = 0;
  if (pattern >= MAXPATTERNS) pattern = 0;
  return pattern;
}

void setPatternValue(int value,int patternNr,int patternPos,int patternSide) {
  if ((patternNr < 0) || (patternNr >= MAXPATTERNS)) return;
  if ((patternPos < 0) || (patternPos >= PATTERNLENGTH)) return;
  if ((patternSide < 0) || (patternSide >= 3)) return;
  if (patternSide == 0) pat[patternNr][patternPos].note = value;
  if (patternSide == 1) pat[patternNr][patternPos].effect = value;
}

void setPatternValueWithInstrument(int note, int instrument, int patternNr,int patternPos) {
  setPatternValue(note, patternNr, patternPos, 0);
  instrument++;
  if (note == 0) instrument = 0;
  setPatternValue(instrument, patternNr, patternPos, 1);
}

int getPatternValue(int patternNr,int patternPos,int patternSide) {
  if ((patternNr < 0) || (patternNr >= MAXPATTERNS)) return 0;
  if ((patternPos < 0) || (patternPos >= PATTERNLENGTH)) return 0;
  if ((patternSide < 0) || (patternSide >= 3)) return 0;
  if (patternSide == 1) return pat[patternNr][patternPos].effect;
  return pat[patternNr][patternPos].note;
}

void setSelection(int pattern, int pos, int side) {
  bool shiftPressed = ImGui::GetIO().KeyShift; // no other way to access shift?
  if (!shiftPressed) {
    selectionStartPattern = pattern;
    selectionStartPos = pos;
    selectionStartSide = side;
    selectionEndPos = pos+1;
    selectionEndSide = side+1;
  } else {
    if (pos < selectionStartPos) pos = selectionStartPos;
    selectionEndPos = pos+1;
    selectionEndSide = side+1;
  }
}

void selectColumn(int column) {
  selectionStartPattern = getCurrentPatternForColumn(column);
  selectionStartSide = 0;
  selectionEndSide = 1;
}

bool isSelected(int pattern, int pos, int side) {
  if (pattern == selectionStartPattern && pos >= selectionStartPos && pos < selectionEndPos && side >= selectionStartSide && side < selectionEndSide)
    return true;
  return false;
}

void moveSelectionX(int colAdd) {
  int found = -1;
  for (int i = 0; i < CHANNELS; i++) {
    if (getCurrentPatternForColumn(i)==selectionStartPattern) {
      found = i;
      break;
    }
  }
  if (found != -1) {
    found += colAdd;
    if (found < 0) {
      selectionStartSide = 0;
      found = 0;
    }
    if (found >= CHANNELS) {
      selectionStartSide = 2;
      found = CHANNELS-1;
    }
    selectionStartPattern = getCurrentPatternForColumn(found);
  }          
}

void deleteOnePatternRow() {
  int pattern = selectionStartPattern;
  if (pattern<0) return;
  if (pattern>=MAXPATTERNS) return;

  selectionStartPos--;
  if (selectionStartPos<0) selectionStartPos = 0;
  selectionEndPos--;
  if (selectionEndPos<selectionStartPos+1) selectionEndPos=selectionStartPos+1;

  int pos = selectionStartPos;
  if (pos < 0) return;
  if (pos >= PATTERNLENGTH) return;
  for (int i = pos; i < PATTERNLENGTH; i++) {
    PATTERN replaceEntry;
    if (i < PATTERNLENGTH-1) 
      replaceEntry = pat[pattern][i+1];
    else
      memset(&replaceEntry,0,sizeof(replaceEntry));
    pat[pattern][i] = replaceEntry;
  }

  scrollTo = selectionStartPos;
}

void insertOnePatternRow() {
  int pattern = selectionStartPattern;
  if (pattern<0) return;
  if (pattern>=MAXPATTERNS) return;
  int pos = selectionStartPos;
  if (pos < 0) return;
  if (pos >= PATTERNLENGTH) return;
  for (int i = PATTERNLENGTH-1; i >= pos ; i--) {
    PATTERN replaceEntry;
    if (i != pos) 
      replaceEntry = pat[pattern][i-1];
    else
      memset(&replaceEntry,0,sizeof(replaceEntry));
    pat[pattern][i] = replaceEntry;
  }
}

void advanceSelection(int selectionMode, int sideAdd, int posAdd) {
  bool shiftPressed = ImGui::GetIO().KeyShift; // no other way to access shift?

  if (!shiftPressed || selectionMode == 1) {
    if (sideAdd != 0) {
      selectionStartSide += sideAdd;
      if (selectionStartSide > 2) {selectionStartSide = 0; moveSelectionX(1);}
      if (selectionStartSide < 0) {selectionStartSide = 2; moveSelectionX(-1);}
    }
  
    if (posAdd != 0) {  
      selectionStartPos += posAdd;
      if (selectionStartPos >= PATTERNLENGTH) {selectionStartPos = PATTERNLENGTH - 1;}
      if (selectionStartPos < 0) {selectionStartPos = 0;}
      scrollTo = selectionStartPos;
    }
    selectionEndSide = selectionStartSide + 1;
    selectionEndPos = selectionStartPos + 1;
  } else {
    if (sideAdd != 0) {
      selectionEndSide += sideAdd;
      if (selectionEndSide<selectionStartSide+1) selectionEndSide = selectionStartSide+1;
      if (selectionEndSide>3) selectionEndSide = 3;
    }
    selectionStartSide = 0;
    selectionEndSide = 3;
    if (posAdd != 0) {
      selectionEndPos += posAdd;
      if (selectionEndPos < selectionStartPos+1) {selectionEndPos = selectionStartPos+1;}
      if (selectionEndPos > PATTERNLENGTH) {selectionEndPos = PATTERNLENGTH;}
      scrollTo = selectionEndPos;
    }
  }
}

bool isMultiLineSelection() {
  return abs(selectionEndPos-selectionStartPos)>1;
}

void copyToCopyPaste() {
  memcpy(copyPasteBuffer,pat[selectionStartPattern],sizeof(copyPasteBuffer));
  copyPastePosStart = selectionStartPos;
  copyPastePosEnd = selectionEndPos;
  copyPasteSideStart = selectionStartSide;
  copyPasteSideEnd = selectionEndSide;
  selectionEndSide = selectionStartSide + 1;

  selectionStartPos = selectionEndPos;
  if (selectionStartPos < 0) selectionStartPos = 0;
  if (selectionStartPos >= PATTERNLENGTH) selectionStartPos = PATTERNLENGTH - 1;
  selectionEndPos = selectionStartPos + 1;
}

void setPatElem(PATTERN *d, PATTERN *s, int side);

void copyFromCopyPaste() {
  //for (int side = copyPasteSideStart; side < copyPasteSideEnd; side++) {
  for (int side = 0; side < 3; side++) {
    for (int pos = copyPastePosStart; pos < copyPastePosEnd; pos++) {
      int dPos = pos-copyPastePosStart+selectionStartPos;
      if (dPos >= 0 && dPos < PATTERNLENGTH) {
        setPatElem(&pat[selectionStartPattern][dPos],&copyPasteBuffer[pos],side);
      }
    }
  }
  selectionStartPos = copyPastePosEnd-copyPastePosStart+selectionStartPos;
  if (selectionStartPos < 0) selectionStartPos = 0;
  if (selectionStartPos >= PATTERNLENGTH) selectionStartPos = PATTERNLENGTH - 1;
  selectionEndPos = selectionStartPos + 1;
}

const char *getNoteString(int patternNote) {
  if (patternNote<=0) return "---";
  if (patternNote==1) return "===";
  patternNote -= 2;
  int cents = patternNote % CENTS;
  int note = (patternNote / CENTS) % 12;
  int octave = patternNote / CENTS / 12;
  static char noteString[4];
  static const char *notes[12] = {
    "C ",
    "C#",
    "D ",
    "D#",
    "E ",
    "F ",
    "F#",
    "G ",
    "G#",
    "A ",
    "A#",
    "H ",
  };
  noteString[2] = octave + '0';
  noteString[3] = 0;
  memcpy(noteString,notes[note],2);
  return noteString;
}

const char *getEffectsString(int patternEffect) {
  if (patternEffect==0) return "---";
  patternEffect--;
  if (patternEffect<0) patternEffect = 0;
  if (patternEffect>=MAXINSTRUMENTS) patternEffect = MAXINSTRUMENTS-1;
  static char effectsString[4];
  sprintf(effectsString,"i%02d",patternEffect);
  return effectsString;
}

int getCents(int patternNote) {
  patternNote -= 2;
  if (patternNote<0) return patternNote;
  int cents = patternNote % CENTS;
  return cents;
}

int getNote(int patternNote) {
  patternNote -= 2;
  if (patternNote<0) return patternNote;
  int note = (patternNote / CENTS) % 12;
  return note;
}

int getOctave(int patternNote) {
  patternNote -= 2;
  if (patternNote<0) return patternNote;
  int octave = (patternNote / CENTS) / 12;
  return octave;
}

int withNote(int patternNote, int note) {
  patternNote -= 2;
  if (patternNote<0) patternNote = 0;
  if (note<0) return note+2;
  int cents = patternNote % CENTS;
  int octave = patternNote / CENTS / 12;
  return note*CENTS + cents + octave*CENTS*12 + 2;
}

int withOctave(int patternNote, int octave) {
  patternNote -= 2;
  if (patternNote<0) patternNote = 0;
  if (octave<0) return octave+2;
  int cents = patternNote % CENTS;
  int note = (patternNote / CENTS) % 12;
  return note*CENTS + cents + octave*CENTS*12 + 2;
}

int withCents(int patternNote, int cents) {
  patternNote -= 2;
  if (patternNote<0) patternNote = 0;
  if (cents<0) return cents+2;
  int note = (patternNote / CENTS) % 12;
  int octave = patternNote / CENTS / 12;
  return note*CENTS + cents + octave*CENTS*12 + 2;
}

void copyNote(int *d, int *s) {
  *d = withOctave(withNote(*d,getNote(*s)),getOctave(*s));
}

void copyCents(int *d, int *s) {
  *d = withCents(*d,getCents(*s));
}

void setPatElem(PATTERN *d, PATTERN *s, int side) {
  if (side == 0) copyNote(&d->note,&s->note);
  if (side == 1) d->effect = s->effect;
  if (side == 2) copyCents(&d->note,&s->note);
}

double playCents = 0;
int noteDur = 3; 
int tone(int note, int cents, int octave) {
  if (note < 0) note = 0;
  if (note > 11) note = 11;
  if (cents < 0) cents = 0;
  if (cents >= CENTS) cents = CENTS-1;
  if (octave < 0) octave = 0;
  if (octave > 9) octave = 9;

  setShouldBeep = noteDur;
  playCents = ((note+(octave-4)*12)*CENTS+cents);
  shouldBeepFrequency = NOTE_C4*pow(2.0,1.0/(12.0*CENTS)*playCents);
  player_editInstrumentPos = 0;
  player_requestSongUpdate = true;

  return note * CENTS + octave * CENTS * 12 + 2;
}

int getNoteValueForKey(int key, int octave, int cents) {
  if (key == ImGui::GetKeyIndex(ImGuiKey_Delete)) return 0;
  int value = -1;
  switch(key) {
  case 'q' : {value = tone(0,cents,octave);} break;
  case 'w' : {value = tone(2,cents,octave);} break;
  case 'e' : {value = tone(4,cents,octave);} break;
  case 'r' : {value = tone(5,cents,octave);} break;
  case 't' : {value = tone(7,cents,octave);} break;
  case 'y' :
  case 'z' : {value = tone(9,cents,octave);} break;
  case 'u' : {value = tone(11,cents,octave);} break;                       
  case '2' : {value = tone(1,cents,octave);} break;
  case '3' : {value = tone(3,cents,octave);} break;
  case '5' : {value = tone(6,cents,octave);} break;
  case '6' : {value = tone(8,cents,octave);} break;
  case '7' : {value = tone(10,cents,octave);} break;
  case 'x' : {value = tone(0,cents,octave-1);} break;
  case 'c' : {value = tone(2,cents,octave-1);} break;
  case 'v' : {value = tone(4,cents,octave-1);} break;
  case 'b' : {value = tone(5,cents,octave-1);} break;
  case 'n' : {value = tone(7,cents,octave-1);} break;
  case 'm' : {value = tone(9,cents,octave-1);} break;
  case ',' : {value = tone(11,cents,octave-1);} break;                       
  case 'd' : {value = tone(1,cents,octave-1);} break;
  case 'f' : {value = tone(3,cents,octave-1);} break;
  case 'h' : {value = tone(6,cents,octave-1);} break;
  case 'j' : {value = tone(8,cents,octave-1);} break;
  case 'k' : {value = tone(10,cents,octave-1);} break;
  case ' ' : {value = 1;} break;
  }
  if (value >= 0) player_requestSongUpdate = true;
  return value;
}

bool ImKeyPressed(int key) {
  return ImGui::IsKeyPressed(ImGui::GetKeyIndex(key));
}

int mouseClickOff = 0;
bool ImIsMouseClicked(int button) {
  mouseClickOff--;
  if (mouseClickOff>100 || mouseClickOff<0) mouseClickOff = 0;
  if (mouseClickOff != 0) return false;
  return ImGui::IsMouseClicked(button);
}

int ImKey() {
  for (int i = 0; i < 512; i++) {
    if (ImGui::IsKeyPressed(i)) return i;
  }
  return -1;
}

void ImBPMField() {
  if (ImGui::Button("-##BPMMINUS")) {
    int ticks = bpmToTimerTicks(BPM);
    BPM = correctBPM(timerTicksToBPM(ticks+1));
    player_requestSongUpdate = true;
  }
  ImGui::SameLine();
  if (ImGui::Button("+##BPMPLUS")) {
    int ticks = bpmToTimerTicks(BPM);
    BPM = correctBPM(timerTicksToBPM(ticks-1));
    player_requestSongUpdate = true;
  }
  ImGui::SameLine();
  ImGui::PushItemWidth(44);
  if (ImGui::InputFloat("BPM",&BPM,0,0,2)) {
    setPlaying(false);
    player_requestSongUpdate = true;
    BPM = correctBPM(BPM);
  }
  ImGui::PopItemWidth();
}

void ImSongNameField() {
  ImGui::PushItemWidth(7*SONGNAMELENGTH+2);
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,1,0.75,1));  
  if (ImGui::InputText("Song",songName,SONGNAMELENGTH)) {
  }
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();
}

void ImAuthorField() {
  ImGui::PushItemWidth(7*AUTHORNAMELENGTH+2);
  ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(1,1,0.75,1));  
  if (ImGui::InputText("Author",authorName,AUTHORNAMELENGTH)) {
  }
  ImGui::PopStyleColor();
  ImGui::PopItemWidth();
}

void ImTimeString() {
  ImGui::Text("%s",currentTimeString().c_str());
}

void ImSameLine() {
  ImGui::SameLine();
}

void ImSeparator() {
  ImGui::Separator();
}

void ImHorSeparator() {
  ImGui::Text("|");
}

void ImPlayButton() {
  if (ImGui::Button(player_isPlaying?"Stop##PLAYBUTTON":"Play##PLAYBUTTON")) {
    setPlaying(!player_isPlaying);
    if (player_isPlaying) {setPlayPatternPos(0); setPlayTrackPos(0);}
  }
}

void save(const char *fileName) {
  FILE *out = fopen(fileName,"w");
  fprintf(out,"SPKT FILE\n");
  fprintf(out,"%d\n",1);
  fprintf(out,"SONGNAME\n");
  fprintf(out,"%s\n", songName);
  fprintf(out,"AUTHOR\n");
  fprintf(out,"%s\n", authorName);
  fprintf(out,"BPM\n");
  fprintf(out,"%f\n", BPM);
  fprintf(out,"CHANNELS\n");
  fprintf(out,"%d\n", CHANNELS);
  fprintf(out,"TIMERTICKS\n");
  fprintf(out,"%d\n", player_timerTicks);
  fprintf(out,"TIMERFREQUENCY\n");
  fprintf(out,"%f\n", timerFrequency);
  fprintf(out,"SONGLENGTH\n");
  fprintf(out,"%d\n", SONGLENGTH);
  fprintf(out,"PATTERNLENGTH\n");
  fprintf(out,"%d\n", PATTERNLENGTH);
  fprintf(out,"LOOPSTART\n");
  fprintf(out,"%d\n", loopBegin);
  fprintf(out,"LOOPEND\n");
  fprintf(out,"%d\n", loopEnd);
  fprintf(out,"SONG\n");
  {for (int i = 0; i < SONGLENGTH; i++) {
    for (int j = 0; j < CHANNELS; j++) {
      fprintf(out,"%d,",patterns[j][i]);
    }
    fprintf(out,"\n");
  }}
  {for (int i = 0; i < MAXINSTRUMENTS; i++) {
    bool saveThis = false;
    INSTRUMENT *n = &instruments[i];
    if (n->loopStart!=0) saveThis = true;
    if (n->loopEnd!=0) saveThis = true;
    for (int j = 0; j < MAXINSTRUMENTLENGTH; j++) {
      if (n->cents[j] != 0) saveThis = true;
    }
    if (saveThis) {
      fprintf(out,"INSTRUMENT\n");
      fprintf(out,"ID\n");
      fprintf(out,"%d\n",i);
      fprintf(out,"LOOPBEGIN\n");
      fprintf(out,"%d\n",n->loopStart);
      fprintf(out,"LOOPEND\n");
      fprintf(out,"%d\n",n->loopEnd);
      fprintf(out,"CENTS\n");
      fprintf(out,"%d\n",MAXINSTRUMENTLENGTH);
      for (int j = 0; j < MAXINSTRUMENTLENGTH; j++) {
        fprintf(out,"%d\n",n->cents[j]);
      }
    }
  }}
  {for (int i = 0; i < MAXPATTERNS; i++) {
    bool saveThis = false;
    PATTERN *p = pat[i];
    for (int j = 0; j < PATTERNLENGTH; j++) {
      if (p[j].note != 0) saveThis = true;
      if (p[j].effect != 0) saveThis = true;
    }
    if (saveThis) {
      fprintf(out,"PATTERN\n");
      fprintf(out,"%d\n",i);
      for (int j = 0; j < PATTERNLENGTH; j++) {
        fprintf(out,"%d,%d,\n",p[j].note,p[j].effect);
      }
    }
  }}
  fclose(out);
}

void ImSaveButton() {
  if (ImGui::Button("Save song")) {
    saveFile = true;
  }
}

#define BUFFER_SIZE 1000
char buffer[BUFFER_SIZE];
void clearSong();

#define GETINT(__v__,__name__) if (strcmp(buffer,__name__)==0) {fgets(buffer,BUFFER_SIZE,in); removeLF(buffer); __v__ = atoi(buffer); continue;}
#define GETDOUBLE(__v__,__name__) if (strcmp(buffer,__name__)==0) {fgets(buffer,BUFFER_SIZE,in); removeLF(buffer); __v__ = (double)atof(buffer); continue;}
#define GETSTRING(__v__,__name__) if (strcmp(buffer,__name__)==0) {fgets(buffer,BUFFER_SIZE,in); removeLF(buffer); __v__ = String(buffer); continue;}
#define IFREGION(__name__) if (strcmp(buffer,__name__)==0)

void removeLF(char *b) {
  if (b[strlen(b)-1] == '\n') b[strlen(b)-1] = 0;
}

void load(const char *fileName) {
  int _SPKT_FILE = 0;
  String _SONGNAME = "<Song name>";
  String _AUTHOR = "<Author>";
  double _BPM = 0;
  int _CHANNELS = 0;
  int _TIMERTICKS = 0;
  double _TIMERFREQUENCY = 0;
  int _SONGLENGTH = 0;
  int _PATTERNLENGTH = 0;
  int _LOOPSTART = -1;
  int _LOOPEND = -1;

  FILE *in = fopen(fileName,"r");
  // HEADER
  while(fgets(buffer,BUFFER_SIZE,in)) {
    removeLF(buffer);
    GETINT(_SPKT_FILE,"SPKT FILE");
    GETSTRING(_SONGNAME,"SONGNAME");
    GETSTRING(_AUTHOR,"AUTHOR");
    GETDOUBLE(_BPM,"BPM");
    GETINT(_CHANNELS,"CHANNELS");
    GETINT(_TIMERTICKS,"TIMERTICKS");
    GETDOUBLE(_TIMERFREQUENCY,"TIMERFREQUENCY");
    GETINT(_SONGLENGTH,"SONGLENGTH");
    GETINT(_PATTERNLENGTH,"PATTERNLENGTH");
    GETINT(_LOOPSTART,"LOOPSTART");
    GETINT(_LOOPEND,"LOOPEND");
  }
  fclose(in);

  if (_SPKT_FILE==1) {
    clearSong();
    sprintf(songName,"%s",_SONGNAME.c_str());
    sprintf(authorName,"%s",_AUTHOR.c_str());
    BPM = correctBPM(_BPM);
    player_timerTicks = _TIMERTICKS;
    if (player_timerTicks < 1) player_timerTicks = 1;
    if (player_timerTicks > MAXTICKS) player_timerTicks = MAXTICKS;
    SONGLENGTH = _SONGLENGTH;
    if (SONGLENGTH < 1) SONGLENGTH = 1;
    if (SONGLENGTH > MAXSONGLENGTH) SONGLENGTH = MAXSONGLENGTH;
    PATTERNLENGTH = _PATTERNLENGTH;
    if (PATTERNLENGTH < 1) PATTERNLENGTH = 1;
    if (PATTERNLENGTH > MAXPATTERNLENGTH) PATTERNLENGTH = MAXPATTERNLENGTH;
    loopBegin = _LOOPSTART;
    loopEnd = _LOOPEND;
    if (loopBegin < 0 && loopEnd>=0) loopBegin = 0;
    // FOOTER
    FILE *in = fopen(fileName,"r");
    while(fgets(buffer,BUFFER_SIZE,in)) {
      removeLF(buffer);
      IFREGION("SONG") {
        {for (int i = 0; i < SONGLENGTH; i++) {
          fgets(buffer,BUFFER_SIZE,in); removeLF(buffer);
          sscanf(buffer,"%d,%d,%d,",&patterns[0][i],&patterns[1][i],&patterns[2][i]);
        }}
        continue;
      }
      IFREGION("INSTRUMENT") {
        int ins = 0;
        int l = 0;
        int e = 0;
        while(fgets(buffer,BUFFER_SIZE,in)) {
          removeLF(buffer);
          GETINT(ins,"ID");
          GETINT(l,"LOOPBEGIN");
          GETINT(e,"LOOPEND");
          IFREGION("CENTS") {break;}
        }
        if (l < 0) l = 0;
        if (e < 0) e = 0;
        if (l > MAXINSTRUMENTLENGTH) l = MAXINSTRUMENTLENGTH;
        if (e > MAXINSTRUMENTLENGTH) e = MAXINSTRUMENTLENGTH;
        if (ins < 0) ins = 0;
        if (ins >= MAXINSTRUMENTS) ins = MAXINSTRUMENTS-1;
        instruments[ins].loopStart = l;
        instruments[ins].loopEnd = e;
        fgets(buffer,BUFFER_SIZE,in); removeLF(buffer); int insC = atoi(buffer);
        if (insC < 0) insC = 0;
        if (insC > MAXINSTRUMENTLENGTH) insC = MAXINSTRUMENTLENGTH;
        for (int i = 0; i < insC; i++) {
          fgets(buffer,BUFFER_SIZE,in); removeLF(buffer);
          instruments[ins].cents[i] = atoi(buffer);
        }
        continue;
      }
      IFREGION("PATTERN") {
        fgets(buffer,BUFFER_SIZE,in);removeLF(buffer); int pattern = atoi(buffer);
        if (pattern < 0) pattern = 0;
        if (pattern >= MAXPATTERNS) pattern = MAXPATTERNS-1;
        for (int i = 0; i < PATTERNLENGTH; i++) {
          fgets(buffer,BUFFER_SIZE,in); removeLF(buffer);
          sscanf(buffer,"%d,%d,",&pat[pattern][i].note,&pat[pattern][i].effect);
        }
        continue;
      }
    }
    fclose(in);
  }

}

void ImLoadButton() {
  if (ImGui::Button("Load song")) {
    loadFile = true;
  }
}

void export_(const char *fileName) {
  PlayData *p = &player_currentData;
  FILE *out = fopen(fileName,"w");
  fprintf(out,"// SPEAKER TRACKER FILE\n");
  fprintf(out,"// A simple one note at a time Speaker Tracker without reprogramming the timer speed.\n");
  fprintf(out,"// You can find the newest version of Speaker Tracker here: https://github.com/kosmonautdnb/WatcomLibs\n");
  fprintf(out,"// APP_SPKT.CPP\n\n");
  fprintf(out,"// use it like:\n");
  fprintf(out,"// unsigned short song[]={\n");
  fprintf(out,"//   #include \"song.exp\"\n");
  fprintf(out,"// };\n\n");
  fprintf(out,"// -- timer ticks per note\n");
  fprintf(out,"%d,\n", p->timerTicks & 0xffff);
  fprintf(out,"// -- song length (count of consecutive patterns)\n");
  fprintf(out,"%d,\n",p->songLength & 0xffff);
  fprintf(out,"// -- pattern length in notes\n");
  fprintf(out,"%d,\n", p->patternLength & 0xffff);
  fprintf(out,"// -- loop start in patterns\n");
  fprintf(out,"%d,\n", p->loopStart & 0xffff);
  fprintf(out,"// -- loop end in patterns\n");
  fprintf(out,"%d,\n", p->loopEnd & 0xffff);
  fprintf(out,"// Here follows the song data as timerTicks*songLength*patternLength\n");
  fprintf(out,"// unsigned shorts to be set to the PIT timer2 lo/hi. 0 means tone off, all other values (are two bytes (lo/hi)) with tone on.\n");
  int k = p->timerTicks*p->songLength*p->patternLength;
  for (int i = 0; i < k; i += 16) {
    for (int j = 0; j < 16 && (i+j)<k; j++) {
      if (((i + j) % (p->timerTicks*p->patternLength))==0) fprintf(out,"\n// Song position %d\n",(i+j)/(p->timerTicks*p->patternLength));
      fprintf(out,"%d,",p->song[i+j].freq & 0xffff);
    }
    fprintf(out,"\n");
  }
  fprintf(out,"\n// End marker. 0xffff\n");
  fprintf(out,"65535,\n");
  fprintf(out,"// ..\n");
  fclose(out);
}

void ImExportButton() {
  if (ImGui::Button("Export song")) {
    exportFile = true;
  }
}

void ImSongLength() {
  ImGui::PushItemWidth(60);
  if (ImGui::InputInt("Song length",&SONGLENGTH)) {
    player_requestSongUpdate = true;
    setPlaying(false);
  }
  if (SONGLENGTH<1) SONGLENGTH = 1;
  if (SONGLENGTH>MAXSONGLENGTH) SONGLENGTH = MAXSONGLENGTH;
  ImGui::PopItemWidth();
}

void ImPatternLength() {
  ImGui::PushItemWidth(60);
  if (ImGui::InputInt("Pattern length",&PATTERNLENGTH)) {
    player_requestSongUpdate = true;
    setPlaying(false);
  }
  if (PATTERNLENGTH<1) PATTERNLENGTH = 1;
  if (PATTERNLENGTH>MAXPATTERNLENGTH) PATTERNLENGTH = MAXPATTERNLENGTH;
  ImGui::PopItemWidth();
}

ImVec2 startOfInstrumentField;

void ImPatternEditField() {
  int columnWidth = 30;
  int numbersWidth = 16+16;
  int scrollWidth = 16;
  bool mouseRight = ImIsMouseClicked(1);
  int width = numbersWidth+columnWidth*CHANNELS+scrollWidth;
  int height = WINDOWHEIGHT/2;
  ImVec2 mp = ImGui::GetMousePos();
  ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::BeginChild("PatternsSub", ImVec2(width,height), true);
  ImVec2 mn = ImGui::GetCursorScreenPos();
  int mxx = mn.x + width;
  ImGui::Columns(CHANNELS);
  for (int col = 0; col < CHANNELS; col++) {
    ImGui::SetColumnOffset(-1,col * columnWidth + (col == 0 ? 0 : numbersWidth));
    ImGui::Text("%sCh%d", col==0?"Sng  ":"",col+1);
    for (int i = 0; i < SONGLENGTH+1; i++) {
      ImVec2 cp1 = ImGui::GetCursorScreenPos();
      char l = ' ';
      if (loopBegin == loopEnd) {
        loopBegin = -1;
        loopEnd = -1;
      }
      if (i == loopBegin) {l = 'l'; player_requestSongUpdate = true;}
      if (i == loopEnd) {l = 'e'; player_requestSongUpdate = true;}
      if (i < SONGLENGTH) {
        if (col == 0) {ImGui::TextColored(i == getPlayTrackPos() ? PlayColumnCol:DescColumnCol,"%c%c%02d", i == getPlayTrackPos() ? '>' : ' ', l, i); ImGui::SameLine();}
        ImGui::PushItemWidth(22);
        if (ImGui::InputInt((String("##IMPATTERN")+String::fromInt(col*MAXSONGLENGTH+i)).c_str(),&patterns[col][i],0)) {
          player_requestSongUpdate = true;
        }
        ImGui::PopItemWidth();
      } else {
        if (col == 0) {ImGui::TextColored(i == getPlayTrackPos() ? PlayColumnCol:DescColumnCol,"%c%c  ", i == getPlayTrackPos() ? '>' : ' ', l); ImGui::SameLine();}
        ImGui::Text("---");
      }
      ImVec2 cp2 = ImGui::GetCursorScreenPos();
      if ((mp.y >= cp1.y) && (mp.y < cp2.y) && (mp.x >= mn.x) && (mp.x < mxx)) {
        if (ImGui::IsKeyPressed('l')) {if (loopBegin==i) loopBegin = -1; else loopBegin = i;}
        if (ImGui::IsKeyPressed('L')) {if (loopEnd==i) loopEnd = -1; else loopEnd = i;}
        if (loopBegin < 0 && loopEnd>=0) loopBegin = 0;
        if (mouseRight) {
          setPlayTrackPos(i);
        }
      }
    }
    ImGui::NextColumn();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  startOfInstrumentField = ImGui::GetCursorScreenPos();
}

void ImInstrumentField() {
  int OFFSETY = -3;
  int columnWidth = 30;
  int numbersWidth = 16+16;
  int scrollWidth = 16;
  ImGui::SetCursorScreenPos(startOfInstrumentField);
  int width = numbersWidth+columnWidth*CHANNELS+scrollWidth;
  int height = WINDOWHEIGHT/2+OFFSETY;
  ImVec2 mp = ImGui::GetMousePos();
  ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::BeginChild("InstrumentSub", ImVec2(width,height), true);
  ImVec2 mn = ImGui::GetCursorScreenPos();
  int mxx = mn.x + width;
  ImGui::PushItemWidth(64);
  if (ImGui::InputInt("Instr",&currentInstrument)) {
  }
  ImGui::PopItemWidth();
  if (currentInstrument < 0) currentInstrument = 0;
  if (currentInstrument >= MAXINSTRUMENTS) currentInstrument = MAXINSTRUMENTS - 1;
  ImGui::Separator();
  ImGui::Text("Detune in cents.");
  for (int i = 0; i < MAXINSTRUMENTLENGTH+1; i++) {
    ImVec2 cp1 = ImGui::GetCursorScreenPos();
    if (i < MAXINSTRUMENTLENGTH) {
      ImGui::Text("%c%c%02d", i == instruments[currentInstrument].loopStart ? 'l':' ', i == instruments[currentInstrument].loopEnd ? 'e':' ',i);
      ImGui::SameLine();
      ImGui::PushItemWidth(50);
      if (ImGui::InputInt((String("##INSTRUMENTCENTS")+String::fromInt(i)).c_str(),&instruments[currentInstrument].cents[i],0)) {
        player_requestSongUpdate = true;
      } 
      if (instruments[currentInstrument].cents[i]<MINCENTS) instruments[currentInstrument].cents[i] = MINCENTS;
      if (instruments[currentInstrument].cents[i]>MAXCENTS) instruments[currentInstrument].cents[i] = MAXCENTS;
      ImGui::PopItemWidth();
      if (instruments[currentInstrument].cents[i]<=MINCENTS) {
        ImGui::SameLine();
        ImGui::Text("OFF");
      }
    } else {
      ImGui::Text(" %c%c  ---",i == instruments[currentInstrument].loopStart ? 'l':' ', i == instruments[currentInstrument].loopEnd ? 'e':' ');
    }

    ImVec2 cp2 = ImGui::GetCursorScreenPos();
    if ((mp.y >= cp1.y) && (mp.y < cp2.y) && (mp.x >= mn.x) && (mp.x < mxx)) {
      if (ImGui::IsKeyPressed('l')) {if (instruments[currentInstrument].loopStart==i) instruments[currentInstrument].loopStart = -1; else instruments[currentInstrument].loopStart = i;}
      if (ImGui::IsKeyPressed('L')) {if (instruments[currentInstrument].loopEnd==i) instruments[currentInstrument].loopEnd = -1; else instruments[currentInstrument].loopEnd = i;}
    }
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
}

void checkCurrentOctave() {
  if (currentOctave < 0) currentOctave = 0;
  if (currentOctave > 9) currentOctave = 9;
}

void noteInput(bool writeData) {
  int key = ImKey();
  bool shiftPressed = ImGui::GetIO().KeyShift; // no other way to access shift?
  int note = getNoteValueForKey(key, currentOctave, currentCents);
  if (note >= 0 && writeData) {
    setPatternValueWithInstrument(note,currentInstrument,selectionStartPattern,selectionStartPos);
    advanceSelection(-1,0,1);
    if (oneShotNotes) {
      if (getPatternValue(selectionStartPattern,selectionStartPos,0) == 0)
        setPatternValueWithInstrument(1,currentInstrument,selectionStartPattern,selectionStartPos);
    }
    player_requestSongUpdate = true;
  }
  if (!shiftPressed) {
    if (key == '+') currentOctave++;
    if (key == '-') currentOctave--;
  }
  checkCurrentOctave();
}

void handleTrackEditKeys(bool justBasicKeys) {
  if (justBasicKeys) return;
  bool shiftPressed = ImGui::GetIO().KeyShift; // no other way to access shift?
  if (ImKeyPressed(ImGuiKey_DownArrow)) {
    advanceSelection(-1,0,1);
  }
  if (ImKeyPressed(ImGuiKey_UpArrow)) {
    advanceSelection(-1,0,-1);
  }
  if (ImKeyPressed(ImGuiKey_PageDown)) {
    advanceSelection(-1,0,10);
  }
  if (ImKeyPressed(ImGuiKey_PageUp)) {
    advanceSelection(-1,0,-10);
  }
  if (ImKeyPressed(ImGuiKey_LeftArrow)) {
    advanceSelection(-1,-1,0);
  }
  if (ImKeyPressed(ImGuiKey_RightArrow)) {
    advanceSelection(-1,1,0);
  }
  if (!justBasicKeys) {
    int key = ImKey();
    if (ImKeyPressed(ImGuiKey_Backspace)) deleteOnePatternRow();
    if (ImGui::IsKeyPressed(VK_INSERT)) insertOnePatternRow();

    noteInput(true);
    if (shiftPressed) {
      int value = getPatternValue(selectionStartPattern,selectionStartPos,0);
      if ((key == '+' || key == '-') && getOctave(value) >= 0) {
        if (key == '+') {currentOctave = getOctave(value)+1;}
        if (key == '-') {currentOctave = getOctave(value)-1;}
        checkCurrentOctave();
        setPatternValue(withOctave(value,currentOctave),selectionStartPattern,selectionStartPos,0);
        player_requestSongUpdate = true;
      }
    }
    checkCurrentOctave();
    if (ImKeyPressed(ImGuiKey_C)) copyToCopyPaste();
    if (ImKeyPressed(ImGuiKey_V)) {copyFromCopyPaste(); player_requestSongUpdate = true;}
    if (key == 'o') {setPlaying(!player_isPlaying); if (player_isPlaying) {setPlayPatternPos(0); setPlayTrackPos(0);}}
    if (key == 'p') {setPlaying(!player_isPlaying);}
    if (key == '0') {playFromCursor();}
  }
}

void ImTestModeField() {
  ImGui::Text("Play/Testmode");
  if (!ImGui::GetIO().WantCaptureKeyboard) noteInput(false);
}

void ImTrackEditField() {
  if (!player_isEditMode) {ImTestModeField(); return;}
  int columnWidth = 50+16;
  int numbersWidth = 24+8;
  int scrollWidth = 16;
  ImVec2 mp = ImGui::GetMousePos();
  bool mouseLeft = ImIsMouseClicked(0);
  bool mouseRight = ImIsMouseClicked(1);
  int width = numbersWidth+columnWidth*CHANNELS+scrollWidth;
  int height = WINDOWHEIGHT;
  ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::BeginChild("TrackSub", ImVec2(width,height), true);
  ImVec2 mn,mx;
  ImGui::Columns(3);
  bool noEditField = ImKeyPressed(ImGuiKey_DownArrow) || ImKeyPressed(ImGuiKey_UpArrow); // the edit box should reset
  noEditField = noEditField || isMultiLineSelection();
  bool shiftPressed = ImGui::GetIO().KeyShift; // no other way to access shift?
  noEditField = noEditField || shiftPressed;

  for (int col = 0; col < CHANNELS; col++) {
    ImGui::SetColumnOffset(-1,col * columnWidth + (col == 0 ? 0 : numbersWidth));
    int pattern = getCurrentPatternForColumn(col);
    ImGui::Text("Prio %d",col);
    if (col == 0) ImGui::Text("Sng %d  Pt %d", getPlayTrackPos(), pattern); else ImGui::Text("Pt %d", pattern);
    for (int i = 0; i < PATTERNLENGTH; i++) {
      bool lineSelected = isSelected(pattern,i,0) || isSelected(pattern,i,1);
      if (scrollTo == i) {scrollTo = -1;ImGui::SetScrollHere();}
      ImVec4 colPlay = i == getPlayPatternPos() ? PlayColumnCol:DescColumnCol;
      ImVec4 colHere;
      colHere = colPlay;    
      if (col == 0) {
        ImGui::TextColored(colHere,"%c%03d", i == getPlayPatternPos() ? '>' : ' ',i); 
        mn = ImGui::GetItemRectMin(); mx = ImGui::GetItemRectMax(); if (mouseRight && mp.x >= mn.x && mp.x < mn.x+width && mp.y >= mn.y && mp.y < mx.y) {setPlayPatternPos(i);} 
        ImGui::SameLine();
      }
      colPlay = i == getPlayPatternPos() ? PlayColumnCol : pat[pattern][i].note == 0 ? DescColumnCol : WhiteColumnCol;
      colHere = isSelected(pattern,i,0) ? SelectedColumnCol : colPlay; ImGui::TextColored(colHere,getNoteString(pat[pattern][i].note)); mn = ImGui::GetItemRectMin(); mx = ImGui::GetItemRectMax(); if (mouseLeft && mp.x >= mn.x && mp.x < mx.x && mp.y >= mn.y && mp.y < mx.y) {setSelection(pattern,i,0);} 
      ImGui::SameLine();
      colPlay = i == getPlayPatternPos() ? PlayColumnCol: DescColumnCol;
      colHere = isSelected(pattern,i,1) ? SelectedColumnCol : colPlay; 
      if (!isSelected(pattern,i,1) || noEditField) {
        ImGui::TextColored(colHere,getEffectsString(pat[pattern][i].effect));
      } else {
        ImGui::SetKeyboardFocusHere();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0,0));
        ImGui::PushItemWidth(7*3+2);
        int v = pat[pattern][i].effect-1;
        if (v < 0) v = 0;
        if (ImGui::InputInt("##INSTRUMENTFIELD",&v,0)) {
          if (v < -1) v = -1; // -1 = clear
          if (v >= MAXINSTRUMENTS) v = MAXINSTRUMENTS-1;
          pat[pattern][i].effect = v+1;
          if (v >= 0) currentInstrument = v;
          player_requestSongUpdate = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
      }
      mn = ImGui::GetItemRectMin(); mx = ImGui::GetItemRectMax(); if (mouseLeft && mp.x >= mn.x && mp.x < mx.x && mp.y >= mn.y && mp.y < mx.y) {setSelection(pattern,i,1);} 
      ImGui::SameLine();
      int v = getCents(pat[pattern][i].note);
      colPlay = i == getPlayPatternPos() ? PlayColumnCol: (v!=0) ? WhiteColumnCol:DescColumnCol;
      bool e = isSelected(pattern,i,2);
      if (!isSelected(pattern,i,2) || noEditField) {
        ImGui::TextColored(v < 0 ? InvalidCol : e ? SelectedColumnCol : colPlay,"%02d",v < 0 ? 0 : v); 
      } else {
        if (v < 0) v = 0;
        ImGui::SetKeyboardFocusHere();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(0,0));
        ImGui::PushItemWidth(7*2+2);
        int vBefore = v;
        if (ImGui::InputInt("##CENTSFIELD",&v,0)) {
          if (v < 0) v = 0;
          if (v >= CENTS) v = CENTS-1;
          pat[pattern][i].note = withCents(pat[pattern][i].note,v);
          if (v != vBefore) tone(getNote(pat[pattern][i].note),v,getOctave(pat[pattern][i].note));
          player_requestSongUpdate = true;
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
      }
      mn = ImGui::GetItemRectMin(); mx = ImGui::GetItemRectMax(); if (mouseLeft && mp.x >= mn.x && mp.x < mx.x && mp.y >= mn.y && mp.y < mx.y) {setSelection(pattern,i,2);}
    }
    ImGui::NextColumn();
  }
  ImGui::EndChild();
  ImGui::PopStyleVar();
  if (ImKeyPressed(ImGuiKey_Tab)) advanceSelection(1,ImGui::GetIO().KeyShift?-1:1,0);
  handleTrackEditKeys(ImGui::GetIO().WantCaptureKeyboard);
}

void ImCopyPatternField() {
  if (ImGui::Button("Copy pattern")) {
    memcpy(pat[copyPatternTo],pat[copyPatternFrom],PATTERNLENGTH*sizeof(PATTERN));
  }
  ImGui::SameLine();
  ImGui::Text("from:");
  ImGui::SameLine();
  ImGui::PushItemWidth(30);
  if (ImGui::InputInt("##COPYPATTERNFROM",&copyPatternFrom,0)) {
  }
  if (copyPatternFrom<0) copyPatternFrom = 0;
  if (copyPatternFrom>=MAXPATTERNS) copyPatternFrom = MAXPATTERNS-1;
  ImGui::PopItemWidth();
  ImGui::SameLine();
  ImGui::Text("to:");
  ImGui::SameLine();
  ImGui::PushItemWidth(30);
  if (ImGui::InputInt("##COPYPATTERNTO",&copyPatternTo,0)) {
  }
  if (copyPatternTo<0) copyPatternTo = 0;
  if (copyPatternTo>=MAXPATTERNS) copyPatternTo = MAXPATTERNS-1;
  ImGui::PopItemWidth();
}

bool quit = false;

void ImExitButton() {
  static bool escapeOn = false;
  if (ImGui::Button("Exit##BUTTONEXIT") || ImKeyPressed(ImGuiKey_Escape)) {
    ImGui::OpenPopup("Exit##EXITMODAL");
    escapeOn = false;
  }
  if (ImGui::BeginPopupModal("Exit##EXITMODAL",0,ImGuiWindowFlags_AlwaysAutoResize)) {
    mouseClickOff = 10;
    ImGui::Text("Do you really want to exit?\n\n");
    ImGui::Separator();
    if (ImGui::Button("Exit##EXITEXITBUTTON") || ImKeyPressed(ImGuiKey_Enter)) { quit = true; ImGui::CloseCurrentPopup(); }
    ImGui::SameLine();
    if (ImGui::Button("Cancel##EXITCANCELBUTTON") || (escapeOn && ImKeyPressed(ImGuiKey_Escape))) { ImGui::CloseCurrentPopup(); }
    ImGui::EndPopup();
    escapeOn = true;
  }
}

void ImInfoField() {
  ImGui::PushStyleVar(ImGuiStyleVar_ChildWindowRounding, 0.0f);
  ImGui::BeginChild("InfoSub", ImVec2(0,WINDOWHEIGHT), true);

  ImGui::PushItemWidth(60);
  if (ImGui::InputInt("Octave",&currentOctave)) {
  }
  checkCurrentOctave();
  ImGui::SameLine();
  bool a;
  a = player_scroll;
  ImGui::Checkbox("Play scrolling", &a);
  player_scroll = a;
  int b = getPlayTrackPos();
  ImGui::InputInt("Sng Pos", &b);
  setPlayTrackPos(b);
  ImGui::SameLine();
  bool c = player_isEditMode;
  ImGui::Checkbox("Edit",&c);
  player_isEditMode = c;
  a = player_patternLoop;
  ImGui::Checkbox("Loop pattern", &a); ImGui::SameLine(); ImGui::Checkbox("One shot notes", &oneShotNotes);
  player_patternLoop = a;
  ImGui::Text("Key time");
  ImGui::SameLine();
  ImGui::InputInt("##NOTEDUR", &noteDur);
  if (noteDur < 1) noteDur = 1;
  if (noteDur > 500) noteDur = 500;
  ImGui::Text("Play time:%0.2fs,%d,%d", getPlayTime(player_trackPos,player_patternPos),player_trackPos,player_patternPos);
  if (ImGui::Button("Full stop")) {
    fullStop();
  }
#ifdef ALSOUSEADLIB
  ImGui::SameLine();
  if (player_useAdlib ? ImGui::Button("Use Beeper##BUTTONADLIB") : ImGui::Button("Use Adlib##BUTTONADLIB")) {
    fullStop();
    clearAdlib();
    player_useAdlib = !player_useAdlib;
    if (player_useAdlib) {
      clearAdlib();
      startAdlibSpeakerMode();
    }
  }
#endif //ALSOUSEADLIB
  ImGui::Text("Timer ticks:%d Timer freq:%0.2fHz", player_timerTicks, timerFrequency);
  ImCopyPatternField();
  ImGui::PopItemWidth();
  ImGui::Separator();
  ImGui::TextColored(SelectedColumnCol,"Instructions:");
  ImGui::Text("Mouse left=select, right=play pos.");
  ImGui::Text("Shift to select. A for fast mode.");
  ImGui::Text("Ctrl+C to copy. Ctrl+V to paste.");
  ImGui::Text("Cursor keys to move. Tab+ShiftTab.");
  ImGui::Text("PageUp+PageDown to move fast.");
  ImGui::Text("+/- to change octave.");
  ImGui::Text("Shift +/- to change note octave.");
  ImGui::Text("O toggle play with restart.");
  ImGui::Text("P toggle play. Shift+0,1,2 Prio.");
  ImGui::Text("0 play from cursor.");
  ImGui::Text("QWERTZU/QWERTYU/XCVBNM, for major.");
  ImGui::Text("23567/DFHJK for minor.");
  ImGui::Text("Space for note stop.");
  ImGui::Text("L,Shift+L for loop begin/end.");
  ImGui::EndChild();
  ImGui::PopStyleVar();
}

void ImInfoField2() {
  ImGui::PushItemWidth(60);
  if (ImGui::InputInt("Oct",&currentOctave)) {
  }
  checkCurrentOctave();
  ImGui::SameLine();
  int b = getPlayTrackPos();
  ImGui::InputInt("SPos", &b);
  setPlayTrackPos(b);
  ImGui::SameLine();
  bool a = player_patternLoop;
  ImGui::Checkbox("LoopP", &a);
  player_patternLoop = a;
  ImGui::SameLine();
  ImGui::InputInt("Instr",&currentInstrument);
  ImGui::PopItemWidth();
}

void clearSong() {
  player_isPlaying = false;
  player_isEditMode = true;
  BPM = correctBPM(160.f);
  SONGLENGTH = 1;
  PATTERNLENGTH = 32;
  loopBegin = -1;
  loopEnd = -1;
  memset(patterns,0,sizeof(patterns));
  memset(pat,0,sizeof(pat));
  memset(instruments,0,sizeof(instruments));
  memset(&player_currentData,0,sizeof(player_currentData));
  memset(&player_nextData,0,sizeof(player_nextData));
  selectionStartPattern = 0;
  selectionStartPos = 0;
  selectionStartSide = 0;
  selectionEndPos = 1;
  selectionEndSide = 1;
  scrollTo = -1;
  currentOctave = 4;
  currentCents = 0;
  player_initial = false;
  player_patternLoop = false;
  player_scroll = false;
  player_trackPos = 0;
  player_fullPos = 0;
  player_patternPos = 0;
  player_requestSongUpdate = true; // initial
  player_speakerOn = false;
  player_speakerVal = 0;
  player_editInstrumentPos = 0;
  //player_timerTicks = 0;
  player_ticker = 0;
  //player_mute = 0;
  player_useAdlib = false;
  player_hzAdlib = 0;

  memset(copyPasteBuffer,0,sizeof(copyPasteBuffer));
  copyPasteSideStart = 0;
  copyPasteSideEnd = 0;
  copyPastePosStart = 0;
  copyPastePosEnd = 0;
  copyPatternFrom = 0;
  copyPatternTo = 0;
  oneShotNotes = false;
  currentInstrument = 0;
  exportFile = false;
  loadFile = false;
  saveFile = false;
  playCents = 0;
  setShouldBeep = 0;
  noteDur = 3;
}

void generatePlayData(PlayData *d) {
  d->timerTicks = player_timerTicks;
  d->songLength = SONGLENGTH;;
  d->patternLength = PATTERNLENGTH;
  d->loopStart = loopBegin;
  d->loopEnd = loopEnd;
  memcpy(&d->editInstrument, &instruments[currentInstrument], sizeof(INSTRUMENT));
  d->editInstrumentCents = playCents;
  PlayNote *dst = d->song;
  double cts[CHANNELS];
  memset(cts,0,sizeof(cts));
  int ins[CHANNELS];
  memset(ins,0,sizeof(ins));
  int insPos[CHANNELS];
  memset(insPos,0,sizeof(insPos));
  for (int sng = 0; sng < SONGLENGTH; sng++) {
    for (int pos = 0; pos < PATTERNLENGTH; pos++) {
      for (int i = 0; i < d->timerTicks; i++) {
        if (i == 0) {
          {for (int ch = 0; ch < CHANNELS; ch++) {
            int pattern = patterns[ch][sng];
            int v = getPatternValue(pattern,pos,0);
            if (v < 2) {
              if (v == 1) cts[ch] = 0;
            } else {
              v -= 2;
              int cents =  v % CENTS;
              int note = (v / CENTS) % 12;
              int octave = v / CENTS / 12;
              if (octave < 0) octave = 0;
              if (octave > 9) octave = 9;
              cts[ch] = (note+(octave+1)*12)*CENTS+cents;
            }
            int i2 = getPatternValue(pattern,pos,1);
            if (i2 > 0) {ins[ch]=i2-1;insPos[ch]=0;}
          }}
        }

        double v = 0;
        {for (int ch = CHANNELS-1; ch >= 0; ch--) {
          if (cts[ch] != 0) {
            double insCents = ch<0?0:instruments[ins[ch]].cents[insPos[ch]];
            v = cts[ch]+insCents;
            if (v < 0) {
              if (insCents <= MINCENTS) 
                v = 0;
              else
                v = 1;
            }
            if (v != 0) break;
          }
        }}
  
        int k = 0;
        if (v != 0) {
          double freq = NOTE_C4*pow(2.0,1.0/(12.0*CENTS)*(v-1*12*CENTS-4*12*CENTS));
          if (freq < 1) freq = 1;
          k = (int)(1193180 / freq);
        }
        dst->freq = k;
        dst++;
        {for (int ch = 0; ch < CHANNELS; ch++) {
          insPos[ch]++;
          if (insPos[ch] == instruments[ins[ch]].loopEnd) insPos[ch]=instruments[ins[ch]].loopStart;
          if (insPos[ch] < 0) insPos[ch] = 0;
          if (insPos[ch] >= MAXINSTRUMENTLENGTH) insPos[ch] = MAXINSTRUMENTLENGTH-1;
        }}
      }
    }
  }
}

void requestSongUpdate() {
  if (player_requestSongUpdate) {
    player_requestSongUpdate = false;
    generatePlayData(&player_nextData);
    unsigned char *a = (unsigned char*)&player_currentData;
    unsigned char *b = (unsigned char*)&player_nextData;
    bool same = true;
    for (int i = 0; sizeof(player_currentData); i++) {
      if (*a++ != *b++) {same = false; break;}
    }
    if (!same) {
      int ticks = timerIrqTicks;
      while(ticks == timerIrqTicks) {;} // wait after the irq, we should have enough time after it to copy the data
      memcpy(&player_currentData,&player_nextData,sizeof(player_currentData));
    }
  }
  if (setShouldBeep != 0) {
    shouldBeep = setShouldBeep+1;
    setShouldBeep = 0;
  }
}

String fileDialog(bool loadInsteadOfSave, String *fileName = NULL) {
  FileDialog *dlg;
  dlg = new FileDialog();
  dlg->open(loadInsteadOfSave, "*.*");
  if (fileName != NULL) sprintf(dlg->saveFileName,fileName->c_str());

  while(true) {
    ImGuiNewFrame();
    ImGui::SetNextWindowPos(ImVec2(0,0));
    bool ret = dlg->display(); 
    glClearColor(10.f/255.f,40.f/255.f,30.f/255.f,1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui::Render();
    glRefresh();
    if (ret) break;
  }
  String ret = dlg->getSelectedFileName();
  if (fileName != NULL) *fileName = String(dlg->saveFileName);
  delete dlg;
  return ret;
}

bool lSwitch = true;
#define L(__v__) if (lSwitch) {__v__}
#define NL(__v__) if (!lSwitch) {__v__}

void ImLeaveFastModeButton() {
  if (ImGui::Button("Leave fast mode")) {
    lSwitch = true;
  }
}

/**
* The main function.
*/
int main(int argc, const char **argv) {

  //glWatcomPrecisionTimer(GL_TRUE);

  if (argc == 1) {
    glEGA(XRES,YRES);
  } else {
    const bool pureVGA = false;
    if (pureVGA) {
      glVGA();
    } else {
      if (!glVesa(XRES,YRES,16)) { // 16 bit is a lot faster than 32 bit on NVidia cards
        printf("No Vesa %dx%dx16 found.\n", XRES, YRES);
        if (!glVGA())
        {
          printf("No VGA device found.\n");
          exit(0);
        }
      }
    }
  }
  installTimerIrqHandler();

  InitImGui(glFrameBufferWidth, glFrameBufferHeight);

  ImGui::GetStyle().Colors[ImGuiCol_TitleBg] = ImVec4(0.3*0.5,0.6*0.5,0.9*0.5,1);
  ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.3,0.6,0.9,1);

  clearSong();

  while(!quit) {
    glClearColor(0.6,0.8,1.0,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    ImGuiNewFrame();
    int currentKey = ImKey();
    bool esc = ImKeyPressed(ImGuiKey_Escape);
    if ((currentKey == 'a' || (esc && !lSwitch)) && !ImGui::GetIO().WantCaptureKeyboard) {lSwitch = !lSwitch; ImGui::Render(); continue;}
    if (currentKey == '=') selectColumn(0);
    if (currentKey == '!') selectColumn(1);
    if (currentKey == '\"') selectColumn(2);
    if (currentKey == ' ' && ImGui::GetIO().KeyShift) {player_isEditMode = !player_isEditMode; ImGui::Render(); continue;}

    if (exportFile) {fullStop();player_mute++;String fileName = fileDialog(false,&exportFileName); if (!fileName.empty()) export_(fileName.c_str()); exportFile = false;player_mute--; ImGui::Render(); continue;}
    if (saveFile) {fullStop();player_mute++;String fileName = fileDialog(false,&saveFileName); if (!fileName.empty()) save(fileName.c_str()); saveFile = false;player_mute--; ImGui::Render(); continue;}
    if (loadFile) {fullStop();player_mute++;String fileName = fileDialog(true,&loadFileName); if (!fileName.empty()) load(fileName.c_str()); loadFile = false;player_mute--; ImGui::Render(); continue;}

    ImGui::SetNextWindowPos(ImVec2(0,0));
    ImGui::SetNextWindowSize(ImVec2(glFrameBufferWidth,glFrameBufferHeight));
    ImGui::Begin("Speaker Tracker v1.0 by Puls4r",0,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);
    L(ImBPMField(); ImSameLine(); ImHorSeparator(); ImSameLine(); ImSongNameField(); ImSameLine(); ImHorSeparator(); ImSameLine(); ImAuthorField(); ImSameLine(); ImHorSeparator(); ImSameLine();) ImTimeString();
    ImSeparator();
    ImPlayButton(); NL(ImSameLine(); ImLeaveFastModeButton();); L(ImSameLine(); ImHorSeparator(); ImSameLine(); ImSongLength();ImSameLine(); ImHorSeparator(); ImSameLine(); ImPatternLength(); ImSameLine(); ImHorSeparator(); ImSameLine(); ImSaveButton(); ImSameLine(); ImLoadButton();  ImSameLine(); ImExportButton(); ImSameLine(); ImExitButton();)
    ImSeparator();
    L(ImPatternEditField(); ImSameLine();) ImTrackEditField(); L(ImSameLine(); ImInfoField();) NL(ImSameLine(); ImInfoField2(););
    L(ImInstrumentField();)
    ImGui::End();

    ImGui::Render();

    glRefresh();
    requestSongUpdate();
  }

  glDone();
  uninstallTimerIrqHandler();
  speaker_tone_off2();
  clearAdlib();
  return 0;
}

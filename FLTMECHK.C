#include <i86.h>
#include <stdio.h>
#include <fcntl.h>

short getFileDate(int file) {
  union REGS r;
  r.h.ah = 0x57;
  r.h.al = 0x00;
  r.w.bx = (short)file;
  int386(0x21,&r,&r);
  return r.w.dx;
}

short getFileTime(int file) {
  union REGS r;
  r.h.ah = 0x57;
  r.h.al = 0x00;
  r.w.bx = (short)file;
  int386(0x21,&r,&r);
  return r.w.cx;
}

int main(int argc, char *argv[]) {
  int handle1;
  int handle2;
  int file1Time;
  int file1Date;
  int file2Time;
  int file2Date;

  handle1 = open( argv[1], O_RDONLY | O_TEXT );
  file1Time = getFileTime(handle1) & 0xffff;
  file1Date = getFileDate(handle1) & 0xffff;
  close(handle1);

  handle2 = open( argv[2], O_RDONLY | O_TEXT );
  file2Time = getFileTime(handle2) & 0xffff;
  file2Date = getFileDate(handle2) & 0xffff;
  close(handle2);

  return file1Date < file2Date ? 255 : (file1Date > file2Date ? 1 : file1Time < file2Time ? 255 : (file1Time == file2Time ? 0 : 1));
}
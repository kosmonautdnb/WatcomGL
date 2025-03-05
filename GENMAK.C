#include <i86.h>
#include <stdio.h>
#include <stdlib.h>

#define MAXFILES 65536
char str[4096];
char *files[MAXFILES];

int currentFile = 0;
int fileCount = 0;

FILE *makeFile;

void getIncludes(const char *fileName, int *firstFile, int *lastFile, unsigned char top) {
  FILE *in;
  FILE *in2;
  char *start;
  char *end;
  char *str2;
  char *c;
  int j;
  if (top) *firstFile=currentFile;
  in = fopen(fileName,"r");
  while(fgets(str,4096,in)) {
    if (sscanf(str,"#include%s",str)) {
      str2 = str;
      while(*str2!=0&&*str2!='\"') str2++;
      if (*str2=='\"') {
        str2++;
        start = str2;
        while(*str2!=0&&*str2!='\"') str2++;
        end = str2;
        for (c = start; c!=end; c++) {if (*c=='/') *c='\\'; if (*c>='A'&&*c<='Z') *c=*c-'A'+'a';}
        for (j=strlen(fileName)-1;j>0;j--) if (fileName[j]=='/'||fileName[j]=='\\') {j++;break;}
        files[currentFile] = malloc(j+end-start+1);
        if(j>0)memcpy(files[currentFile],fileName,j);
        memcpy(files[currentFile]+j,start,end-start);
        files[currentFile][end-start+j]=0x00;
        for(j=*firstFile;j<currentFile;j++) if (strcmp(files[currentFile],files[j])==0) {j=-1;break;}
        if (j!=-1) {
          c = files[currentFile];
          in2 = fopen(c,"r");
          if (in2 != NULL) {
            currentFile++;
            getIncludes(c,firstFile,lastFile,0);
            fclose(in2);
          }
        }
      }
    }
  }
  fclose(in);
  if (top) *lastFile=currentFile;
}


int main(int argc, char *argv[]) {
  FILE *in;
  FILE *mak;
  int i,j;
  int firstFile;
  int lastFile;
  const char *makeFileName;
  const char *listFile;
  //const char *makeFilePrefix;

  if (argc != 3) {
    printf("USAGE: genmak <makeFileName> <listFileWithCAndObjFiles>\n");
    exit(0);
  }

  makeFileName = argv[1];
  listFile = argv[2];
  //makeFilePrefix = argv[3];

  currentFile = 0;
  in = fopen(listFile,"r");
  while(fgets(str,4096,in)) {
    files[currentFile] = malloc(strlen(str)+1);
    memcpy(files[currentFile],str,strlen(str)+1);
    i = 0;
    for(;;i++) if(files[currentFile][i]=='\t'||files[currentFile][i]==' '||files[currentFile][i]=='\n'||files[currentFile][i]==0) {if (files[currentFile][i]!=0) {files[currentFile][i]=0;i++;}break;}
    for(;;i++) if((files[currentFile][i]!='\t'&&files[currentFile][i]!=' '&&files[currentFile][i]!='\n')||files[currentFile][i]==0) {files[currentFile+1]=&files[currentFile][i];break;}
    for(;;i++) if(files[currentFile][i]=='\t'||files[currentFile][i]==' '||files[currentFile][i]=='\n'||files[currentFile][i]==0) {if (files[currentFile][i]!=0) {files[currentFile][i]=0;i++;}break;}
    if(strlen(files[currentFile])>0&&strlen(files[currentFile+1])>0)
      currentFile+=2;

  }
  fclose(in);
  fileCount = currentFile/2;
  printf("processing for files:\n");
  for (i = 0; i < fileCount; i++) {
    printf(".C:%s .OBJ:%s\n",files[i*2],files[i*2+1]);
  }

  printf("generating makefile:\n");
  makeFile = fopen(makeFileName,"w");
  //mak = fopen(makeFilePrefix,"r");
  //while(fgets(str,4096,mak)) fputs(str,makeFile);
  //fclose(mak);

  fprintf(makeFile,"\n\n");
  fprintf(makeFile,"all:");
  for (i = 0; i < fileCount; i++) {
    fprintf(makeFile," _BUILD\\OBJ\\%s", files[i*2+1]);
  }
  fprintf(makeFile," .SYMBOLIC\n");
  for (i = 0; i < fileCount; i++) {
    fprintf(makeFile," @echo FILE _BUILD\\OBJ\\%s >> _BUILD\\LINK.LNK\n", files[i*2+1]);
  }
  fprintf(makeFile," @call _BUILD\\AL.BAT _BUILD\\LINK.LNK GLTEST.EXE\n", files[i*2+1]);
  fprintf(makeFile," @%%null\n");
  fprintf(makeFile,"\n\n");
  fprintf(makeFile,"clean: .SYMBOLIC\n");
  fprintf(makeFile,"    @del _BUILD\\OBJ\\*.obj\n");
  fprintf(makeFile,"    @%%null\n");
  fprintf(makeFile,"\n\n");
  for (i = 0; i < fileCount; i++) {
    printf(".");fflush(NULL);
    //printf("%s: ",files[i*2+1]);
    fprintf(makeFile,"_BUILD\\OBJ\\%s:",files[i*2+1]);
    //printf("%s ",files[i*2+0]);
    fprintf(makeFile," %s",files[i*2+0]);
    getIncludes(files[i*2+0],&firstFile,&lastFile,1);
    for (j = firstFile; j < lastFile; j++) {
      //printf("%s ",files[j]);
      fprintf(makeFile," %s",files[j]);
    }
    fprintf(makeFile,"\n    @call _build\\cc.bat %s %s\n",files[i*2+0],files[i*2+1]);
    fprintf(makeFile,"\n\n");
    //printf("\n\n");
    fprintf(makeFile,"\n\n");
    fflush(makeFile);
  }
  printf("\n");
  fclose(makeFile);
  return 0;
}
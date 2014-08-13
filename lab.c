/* lab.c
 *
 * An audio control panel.
 *
 *   (C) Copyright 1996-2003 Fred Gleason <fredg@paravelsystems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 2 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wait.h>
#include "conflib.h"
#include "wavlib.h"
#include "lab.h"

/*
 * Global Variables
 */
struct buttontext btButtons[MAX_PALETTES][25];
char sAudiodev[STRING_SIZE];
int hAudiodev;
char sAudioroot[STRING_SIZE];
int dDeckStop=0;
int dDeckNum=-1;
int dMilitaryTime=0;
int dPalette=0,dPaletteQuan=1;
int dDeckStat=LAB_STATUS_READY;
int dCurPalette=0;

int main(int argc,char *argv[])
{
  int c,e;
  fd_set fdSet,fdRef;
  struct timeval tvTime,tvRef;
  time_t tDeckEnd=0,tCurrentTime;
  pid_t pPid=0;
  int dScanCounter=0;
  char sAccum[STRING_SIZE];

  if(LoadIni(INI_PATH)<0) {
    exit(1);
  }

  /*
   * Initialize Audio Devices
   */
  hAudiodev=open(sAudiodev,O_WRONLY);
  if(hAudiodev<0) {
    printf("lab: unable to open audio device\n");
    exit(1);
  }

  (void) signal(SIGINT,Finish);
  (void) signal(SIGCHLD,ClearDeck);
  StartScreen(LAB_BLUE);

  /*
   * Initialize values for select()
   */
  FD_ZERO(&fdRef);
  FD_SET(0,&fdRef);
  tvRef.tv_sec=0;
  tvRef.tv_usec=SCAN_INTERVAL;

  ScanArchive();

  /*
   * Main Loop
   */
  for(;;) {
    fdSet=fdRef;
    tvTime=tvRef;
    e=select(1,&fdSet,NULL,NULL,&tvTime);
    if(e>0) {
      c=GetKey();
      /*      c=CaseDown(getchar());  */
      if(c>0) {
	if((c>='a')&&(c<='w')) {   
	  if(btButtons[dPalette][c-'a'].dStatus!=LAB_STATUS_DISABLED) {
	    switch(dDeckStat) {
	    case LAB_STATUS_READY:      /* Start Cut */  
	      dDeckStat=LAB_STATUS_PLAYING;
	      dDeckNum=c-'a';
	      dCurPalette=dPalette;
	      tCurrentTime=time(&tCurrentTime);
	      tDeckEnd=tCurrentTime+GetWavLength(dDeckNum);
	      SetColor(dDeckNum,LAB_WHITE);
	      pPid=fork();
	      if(pPid==0) {
		PlayWavDesc(btButtons[dPalette][dDeckNum].sPath,
			    hAudiodev,WAVLIB_PAUSEABLE);
		exit(0);
	      }
	      break;
	    case LAB_STATUS_PLAYING:    /* Pause Cut */
	      if(dDeckNum==(c-'a')) {
		dDeckStat=LAB_STATUS_PAUSED;
		kill(pPid,SIGUSR1);
		strcat(btButtons[dPalette][c-'a'].line0," **PAUSED**");
		SetText(c-'a',&btButtons[dPalette][c-'a']);
	      }
	      break;
	    case LAB_STATUS_PAUSED:    /* Resume Cut */
	      if(dDeckNum==(c-'a')) {
		dDeckStat=LAB_STATUS_PLAYING;
		kill(pPid,SIGUSR2);
		SetColor(dDeckNum,LAB_WHITE);
		btButtons[dPalette][c-'a'].line0[1]=0;
		SetText(c-'a',&btButtons[dPalette][c-'a']);
		break;
	      }
	    }
	  }
	}
	if((c>=1)&&(c<=25)) {   /* Stop Audio */
	  if(dDeckNum==c-1) {
	    if((dDeckStat==LAB_STATUS_PLAYING)||(dDeckStat==LAB_STATUS_PAUSED)) {
	      dDeckStat=LAB_STATUS_READY;
	      kill(pPid,SIGTERM);
	      btButtons[dPalette][c-1].line0[1]=0;
	      StampLength(c-1);
	    }
	  }
	}
	if(c==KEY_F(12)) {   /* Exit the program */
	  if(CloseScreen()==0) {
	    if((dDeckStat==LAB_STATUS_PLAYING)||(dDeckStat==LAB_STATUS_PAUSED)) {
	      kill(pPid,SIGTERM);
	    }
	    exit(0);
	  }
	}
	if(c==KEY_PPAGE) {   /* Advance to next palette */
	  dPalette++;
	  if(dPalette==dPaletteQuan) {
	    dPalette=0;
	  }
	  ScanArchive();
	}
      }
    }
    if(dDeckStop==1) {
      SetColor(dDeckNum,LAB_BLUE);
      switch(dDeckStat) {
      case LAB_STATUS_PLAYING:
	dDeckStat=LAB_STATUS_READY;
	dDeckNum=-1;  
	break;
      }
      dDeckStop=0;
    }
    
    /*
     * Update Counter
     */
    if(dDeckStat==LAB_STATUS_PLAYING) {
      tCurrentTime=time(&tCurrentTime);
      BreakDownInterval(tDeckEnd-tCurrentTime,sAccum);
      sprintf(btButtons[dPalette][dDeckNum].line3,"        %s",sAccum);
      SetText(dDeckNum,&btButtons[dPalette][dDeckNum]); 
    }
    if(dDeckStat==LAB_STATUS_PAUSED) {
      tDeckEnd++;
    }
    
    TickClock();
    
    if(dScanCounter++>ARCHIVE_SCAN) {
      ScanArchive();
    }
  }
  exit(0);
}


void ClearDeck(int sig)
{
  pid_t pLocalPid;

  pLocalPid=waitpid(-1,NULL,WNOHANG);
  dDeckStop=1;
  signal(SIGCHLD,ClearDeck);
}


void Finish(int sig)
{
  CloseScreen();
  exit(0);
}



int LoadIni(char *sInipath)
{
  int i,j;
  char sSection[STRING_SIZE];
  char sLabel[STRING_SIZE];
  char sDefault[STRING_SIZE];
  char sAccum[PATH_LEN];

  /*
   * The Global Section
   */
  if(GetPrivateProfileString(sInipath,"Global","AudioRoot",sAudioroot,"",
			     STRING_SIZE)<0) {
    printf("lab: missing audio archive directory\n");
    return -1;
  }
  GetPrivateProfileString(sInipath,"Devices","PlayDevice",sAudiodev,
			  "/dev/dsp",STRING_SIZE);
  dPaletteQuan=GetPrivateProfileInt(sInipath,"Global","Palettes",1);

  /*
   * The Palette Section
   */
  for(j=0;j<dPaletteQuan;j++) {
    sprintf(sSection,"Palette%d",j);
    strcpy(btButtons[j][23].line0,"PgUp");
    GetPrivateProfileString(sInipath,sSection,"Label1",btButtons[j][23].line1,
			    sSection,STRING_SIZE);
    GetPrivateProfileString(sInipath,sSection,"Label2",btButtons[j][23].line2,
			    "",STRING_SIZE);
    strcpy(btButtons[j][23].line3,"");
    
    strcpy(sDefault,"<UNASSIGNED>");
    for(i=0;i<23;i++) {
      btButtons[j][i].line0[0]='A'+i;
      btButtons[j][i].line0[1]=0;
      sprintf(sLabel,"Button%dLine1",i);
      GetPrivateProfileString(sInipath,sSection,sLabel,btButtons[j][i].line1,
			      sDefault,STRING_SIZE);
      sprintf(sLabel,"Button%dLine2",i);
      GetPrivateProfileString(sInipath,sSection,sLabel,btButtons[j][i].line2,
			      sDefault,STRING_SIZE);
      strcpy(btButtons[j][i].line3,"");
      strcpy(btButtons[j][i].sPath,sAudioroot);
      strcat(btButtons[j][i].sPath,"/");
      sprintf(sLabel,"Button%dPath",i);
      GetPrivateProfileString(sInipath,sSection,sLabel,sAccum,"",STRING_SIZE);
      if(strcmp(sAccum,"")==0) {
	btButtons[j][i].sPath[0]=0;
      }
      else {
	strcat(btButtons[j][i].sPath,sAccum);
	sprintf(sLabel,"Button%dLife",i);
      }
      btButtons[j][i].dShelflife=GetPrivateProfileInt(sInipath,
						      sSection,sLabel,0);
      sprintf(sLabel,"SkipWeekend%d",i);
      btButtons[j][i].dEndwrap=GetPrivateProfileInt(sInipath,
						    sSection,sLabel,0);
    }
  }
  return 0;
}


int CaseDown(int dKey)
{
  if((dKey>='A')&&(dKey<='Z')) {
    return dKey-'A'+'a';
  }
  return dKey;
}


void ScanArchive(void)
{
  int i;
  int dFudge=0;
  struct stat statWave;
  time_t tCurrentTime,tMidnight;
  struct tm tmModtime,tmCurrentTime;

  /*
   * Calculate current time and the weekend fudge factor (if needed)
   */
  tCurrentTime=time(&tCurrentTime);
  tmCurrentTime=*localtime(&tCurrentTime);
  switch(tmCurrentTime.tm_wday) {
  case 6:   /* Saturday */
    tmCurrentTime.tm_hour=0;
    tmCurrentTime.tm_min=0;
    tmCurrentTime.tm_sec=0;
    tMidnight=time(&tMidnight);
    dFudge=tCurrentTime-tMidnight;
    break;
  case 0:   /* Sunday */
    tmCurrentTime.tm_hour=0;
    tmCurrentTime.tm_min=0;
    tmCurrentTime.tm_sec=0;
    tMidnight=time(&tMidnight);
    dFudge=86400+tCurrentTime-tMidnight;
    break;
  case 1:   /* Monday */
    dFudge=172800;
    break;
  default:
    dFudge=0;
    break;
  }

  /*
   * Update the page button
   */
  SetText(23,&btButtons[dPalette][23]);

  /*
   * Update the audio buttons
   */
  for(i=0;i<23;i++) {
    if(strcmp(btButtons[dPalette][i].sPath,"")==0) {  /* Button Disabled */
      strcpy(btButtons[dPalette][i].line0,"");
      strcpy(btButtons[dPalette][i].line1,"");
      strcpy(btButtons[dPalette][i].line2,"");
      strcpy(btButtons[dPalette][i].line3,"");
      SetText(i,&btButtons[dPalette][i]);
      SetColor(i,LAB_BLACK);
      btButtons[dPalette][i].dStatus=LAB_STATUS_DISABLED;
    }
    else {
      if(stat(btButtons[dPalette][i].sPath,&statWave)<0) {  
	SetColor(i,LAB_RED);  /* Can't find the audio! */
	StampLength(i);
	btButtons[dPalette][i].dStatus=LAB_STATUS_MISSING;
      }
      else {
	if(btButtons[dPalette][i].dEndwrap==1) {   /* Weekend fudge factor */
	  tmModtime=*localtime(&statWave.st_mtime);
	  if(tmModtime.tm_wday==5) {
	    statWave.st_mtime+=dFudge;
	  }
	}
	if((btButtons[dPalette][i].dShelflife!=0)&&
	   (statWave.st_mtime+btButtons[dPalette][i].dShelflife)<tCurrentTime) {
	  SetColor(i,LAB_YELLOW);   /* Expired! */
	  StampLength(i);
	  btButtons[dPalette][i].dStatus=LAB_STATUS_EXPIRED;
	}
	else {
	  if((dDeckNum==i)&&(dCurPalette==dPalette)) {
	    if((dDeckStat==LAB_STATUS_PLAYING)||
	       (dDeckStat==LAB_STATUS_PAUSED)) {
	      SetColor(i,LAB_WHITE);
	    }
	  }
	  else {
	    if(statWave.st_nlink>1) {
	      SetColor(i,LAB_GREEN);    /* Updating! */
	      StampLength(i);
	      btButtons[dPalette][i].dStatus=LAB_STATUS_UPDATING;
	    }
	    else {
	      SetColor(i,LAB_BLUE);       /* All OK! */
	      StampLength(i);
	      btButtons[dPalette][i].dStatus=LAB_STATUS_READY;
	    }
	  }
	}
      }
    }
    SetText(i,&btButtons[dPalette][i]);
  }
}



void StampLength(int dButton)
{
  char sInterval[STRING_SIZE];
  time_t tLength;
  
  tLength=GetWavLength(dButton);
  BreakDownInterval(tLength,sInterval);
  sprintf(btButtons[dPalette][dButton].line3,"        %s",sInterval);
  SetText(dButton,&btButtons[dPalette][dButton]);
  return;
}



void BreakDownInterval(int dInterval,char *sInterval)
{
  int dMin,dSec;

  dMin=dInterval/60;
  dSec=dInterval-(dMin*60);
  sprintf(sInterval,"%02d:%02d",dMin,dSec);
  return;
}



time_t GetWavLength(int dButton)
{
  struct wavHeader wavData;
  int hFilename;

  hFilename=OpenWav(btButtons[dPalette][dButton].sPath,&wavData);
  if(hFilename<0) {
    return 0;
  }
  close(hFilename);
  return wavData.tWavLength;
}



void TickClock(void)
{
  time_t tCurrentTime;
  struct tm tmCurrentTime;
  int dPm=0;
  char sTime[15];

  tCurrentTime=time(&tCurrentTime);
  tmCurrentTime=*localtime(&tCurrentTime);

  if(dMilitaryTime==0) { 
    if(tmCurrentTime.tm_hour==0) {
      tmCurrentTime.tm_hour+=12;
    }
    if(tmCurrentTime.tm_hour>12) {
      tmCurrentTime.tm_hour-=12;
      dPm=1;
    }
    sprintf(sTime,"%2d:%02d:%02d",tmCurrentTime.tm_hour,tmCurrentTime.tm_min,
	    tmCurrentTime.tm_sec);
    if(dPm==0) {
      sprintf(sTime+8," AM");
    }
    else {
      sprintf(sTime+8," PM");
    }
  }
  else {
    sprintf(sTime,"%02d:%02d:%02d",tmCurrentTime.tm_hour,tmCurrentTime.tm_min,
	    tmCurrentTime.tm_sec);
  }

  WriteClock(sTime);

  return;
}




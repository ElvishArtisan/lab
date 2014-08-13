/* error.c
 *
 * Error handlers for the Linux Audio Backstop.
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "wavlib.h"
#include "labd.h"

void EventError(int dEventError,int dEventRecord)
{
  struct tm tmCurrentTime;
  time_t tCurrentTime;
  char sError[255];
  char sAccum[255];
  int i;

  /* Build the time-stamp */
  /* Get current time */
  time(&tCurrentTime);
  tmCurrentTime=*localtime(&tCurrentTime);

  /* Month */
  if(tmCurrentTime.tm_mon<10) {
    sprintf(sError,"0%d-",tmCurrentTime.tm_mon);
  }
  else {
    sprintf(sError,"%d-",tmCurrentTime.tm_mon);
  }

  /* Day of month */
  if(tmCurrentTime.tm_mday<10) {
    sprintf(sAccum,"0%d-",tmCurrentTime.tm_mday);
  }
  else {
    sprintf(sAccum,"%d-",tmCurrentTime.tm_mday);
  }
  strcat(sError,sAccum);

  /* Year */
  sprintf(sAccum,"%d ",tmCurrentTime.tm_year);
  strcat(sError,sAccum);

  /* Hour */
  if(tmCurrentTime.tm_hour<10) {
    sprintf(sAccum,"0%d:",tmCurrentTime.tm_hour);
  }
  else {
    sprintf(sAccum,"%d:",tmCurrentTime.tm_hour);
  }
  strcat(sError,sAccum);

  /* Minute */
  if(tmCurrentTime.tm_min<10) {
    sprintf(sAccum,"0%d:",tmCurrentTime.tm_min);
  }
  else {
    sprintf(sAccum,"%d:",tmCurrentTime.tm_min);
  }
  strcat(sError,sAccum);

  /* Second */
  if(tmCurrentTime.tm_sec<10) {
    sprintf(sAccum,"0%d",tmCurrentTime.tm_sec);
  }
  else {
    sprintf(sAccum,"%d  ",tmCurrentTime.tm_sec);
  }
  strcat(sError,sAccum);

  /* Add the error message */
  switch(dEventError) {
  case EVENT_ERROR_NOACCESS:
    switch(scdTraffic[dEventRecord].dEventType) {
    case EVENT_TIMED_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_THRESHOLD_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d not accessible",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    }
    strcat(sError,sAccum);
    break;
  case EVENT_ERROR_NODECK:
    switch(scdTraffic[dEventRecord].dEventType) {
    case EVENT_TIMED_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_THRESHOLD_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d doesn't exist",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    }
    strcat(sError,sAccum);
    break;
  case EVENT_ERROR_DECKBUSY:
    switch(scdTraffic[dEventRecord].dEventType) {
    case EVENT_TIMED_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_THRESHOLD_RECORD:
      sprintf(sAccum,"Couldn't record %s: audio device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: audio device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_TIMED_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    case EVENT_CLOSURE_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: switcher device %d busy",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dDevice);
      break;
    }
    strcat(sError,sAccum);
    break;
  case EVENT_ERROR_NOCLOSURE:
    switch(scdTraffic[dEventRecord].dEventType) {
    case EVENT_CLOSURE_PLAYBACK:
      sprintf(sAccum,"Couldn't play %s: no closure on pin %d",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dPin);
      break;
    case EVENT_CLOSURE_RECORD:
      sprintf(sAccum,"Couldn't record %s: no closure on pin %d",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dPin);
      break;
    case EVENT_CLOSURE_SWITCHER:
      sprintf(sAccum,"Couldn't do %s: no closure on pin %d",
	      scdTraffic[dEventRecord].sCutName,
	      scdTraffic[dEventRecord].dPin);
      break;
    }
    strcat(sError,sAccum);
    break;
  }

  /* Dispatch the error message */
  /* Via system console */
  if(dConsoleAlarm==1) {
    ToConsole(sError);
  }

  /* Via e-mail */
  if(dMailAlarm==1) {
    for(i=0;i<4;i++) {
      if(sPageTo[i][0]!=0) {
	if(fork()==0) {   /* Send the mail */
	  exit(0);
	}
      }
    }
  }

  /* Via pager */
  if(dPageAlarm==1) {
  }

  /* Via external alarm */
  if(dExternalAlarm==1) {
  }
}



void ToConsole(char *sMessage)
{
  FILE *hConsole;

  hConsole=fopen("/dev/console","w");
  if(hConsole==NULL) {
    return;
  }
  fprintf(hConsole,"%s",sMessage);
  fclose(hConsole);
}



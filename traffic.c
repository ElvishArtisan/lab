/* traffic.c
 *
 * The traffic module for the Linux Audio Backstop
 *
 *   (C) Copyright 1996-2004 Fred Gleason <fredg@paravelsystems.com>
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
#include <string.h>
#include <math.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <strings.h>
#include "wavlib.h"
#include "conflib.h"
#include "labd.h"

int LoadTraffic(char *sLocalFilename,int dMaxEvents)
{
  char cBuffer[BUFFER_SIZE];
  int i,j,k,l,m;
  int i1,j1;
  int dField,dRecord,dLine;
  FILE *hFilename;
  char sAccum[256];

  /* Open the traffic file */
  hFilename=fopen(sLocalFilename,"r");
  if(hFilename==NULL) {
    return -1;
  }

  /* Read a line at a time and parse it */
  dRecord=0;
  dLine=1;
  while(GetIniLine(hFilename,cBuffer)==0) {
    /* Is this a comment? */
    if(IsComment(cBuffer)==0) {
      i=0;
      dField=0;
      memset(&scdTraffic[dRecord],0,sizeof(struct scdTraffic));
      for(m=0;m<MAX_GPIOS;m++) {
	scdTraffic[dRecord].gpioMap[m]=-1;
      }
      while(cBuffer[i]!=0) {
	j=0;
	while(cBuffer[i]!=',' && cBuffer[i]!=0) {
	  sAccum[j++]=cBuffer[i++];
	}
	sAccum[j]=0;
	switch(dField) {
	case 0:     /* The MONTH Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dOpenMonth=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenMonth)!=1) {
	      TrafficError("MONTH",dLine);
	    }
	    if(scdTraffic[dRecord].dOpenMonth<1 || 
	       scdTraffic[dRecord].dOpenMonth>12) {
	      TrafficError("MONTH",dLine);
	    }
	  }
	  break;
	case 1:     /* The DAY Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dOpenDay=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenDay)!=1) {
	      TrafficError("DAY",dLine);
	    }
	    if(scdTraffic[dRecord].dOpenDay<1 || 
	       scdTraffic[dRecord].dOpenDay>31) {
	      TrafficError("DAY",dLine);
	    }
	  }
	  break;
	case 2:     /* The YEAR Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dOpenYear=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenYear)!=1) {
	      TrafficError("YEAR",dLine);
	    }
	    if(scdTraffic[dRecord].dOpenYear<1 || 
	       scdTraffic[dRecord].dOpenYear>9999) {
	      TrafficError("YEAR",dLine);
	    }
	  }
	  break;
	case 3:     /* The DAY-OF-WEEK Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dDayOfWeek=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dDayOfWeek)!=1) {
	      TrafficError("DAY-OF-WEEK",dLine);
	    }
	    if(scdTraffic[dRecord].dDayOfWeek<0 || 
	       scdTraffic[dRecord].dDayOfWeek>127) {
	      TrafficError("DAY-OF-WEEK",dLine);
	    }
	  }
	  break;
	case 4:     /* The OPEN-HOUR Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dOpenHour=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenHour)!=1) {
	      TrafficError("OPEN-HOUR",dLine);
	    }
	    if(scdTraffic[dRecord].dOpenHour<0 || 
	       scdTraffic[dRecord].dOpenHour>23) {
	      TrafficError("OPEN-HOUR",dLine);
	    }
	  }
	  break;
	case 5:     /* The OPEN-MINUTE Field */
	  if(strcmp(sAccum,"*")==0) {   /* Wildcard */
	    scdTraffic[dRecord].dOpenMinute=WILDCARD;
	  }
	  else {
	    if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenMinute)!=1) {
	      TrafficError("OPEN-MINUTE",dLine);
	    }
	    if(scdTraffic[dRecord].dOpenMinute<0 || 
	       scdTraffic[dRecord].dOpenMinute>59) {
	      TrafficError("OPEN-MINUTE",dLine);
	    }
	  }
	  break;
	case 6:     /* The OPEN-SECOND Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dOpenSecond)!=1) {
	    TrafficError("OPEN-SECOND",dLine);
	  }
	  if(scdTraffic[dRecord].dOpenSecond<0 || 
	     scdTraffic[dRecord].dOpenSecond>59) {
	    TrafficError("OPEN-SECOND",dLine);
	  }
	  break;
	case 7:     // The POST-COMMAND Field
	  l=strlen(sAccum);
	  if(l<256) {
	    j1=0;
	    for(i1=0;i1<l;i1++) { 
	      scdTraffic[dRecord].sCommand[j1++]=sAccum[i1];
	    }
	    if(!strcmp(scdTraffic[dRecord].sCommand,"0")) {
	      scdTraffic[dRecord].sCommand[0]=0;
	    }
	  }
	  else {
	    TrafficError("POST-COMMAND",dLine);
	  }
	  break;
	    case 8:     // The DELETE-SOURCE Field
	      if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dDeleteSource)!=1) {
		TrafficError("DELETE-SOURCE",dLine);
	      }
	      break;
/*
	    case 9:     // The CLOSE-SECOND Field
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dCloseSecond)!=1) {
	    TrafficError("CLOSE-SECOND",dLine);
	  }
	  if(scdTraffic[dRecord].dCloseSecond<0 || 
	     scdTraffic[dRecord].dCloseSecond>59) {
	    TrafficError("CLOSE-SECOND",dLine);
	  }
	  break;
*/
	case 10:     /* The LENGTH Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dCutLength)!=1) {
	    TrafficError("LENGTH",dLine);
	  }
	  break;
	case 11:    /* The EVENT-TYPE Field */
	  scdTraffic[dRecord].dEventType=1000;
	  for(k=0;k<EVENT_QUAN;k++) {
	    if(strcmp(sAccum,sEventType[k])==0) {
	      scdTraffic[dRecord].dEventType=k;
	    }
	  }
	  if(scdTraffic[dRecord].dEventType==1000) {
	    TrafficError("EVENT-TYPE",dLine);
	  }
	  if(((scdTraffic[dRecord].dCutLength<1)|| 
	     (scdTraffic[dRecord].dCutLength>86400))&&
	     (scdTraffic[dRecord].dEventType==EVENT_TIMED_RECORD)) {
	    TrafficError("LENGTH",dLine);
	  }
	  break;
	case 12:    /* The FILENAME Field */
	  strcpy(scdTraffic[dRecord].sFilename,sAccum);
	  Prepend(sAudioroot,scdTraffic[dRecord].sFilename);
	  break;
	case 13:    /* The DEVICE Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dDevice)!=1) {
	    TrafficError("DEVICE",dLine);
	  }
	  if(scdTraffic[dRecord].dDevice<0 || 
	     scdTraffic[dRecord].dDevice>3) {
	    TrafficError("DEVICE",dLine);
	  }
	  break;
	case 14:    /* The SAMPLE-RATE Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dSampleRate)!=1) {
	    TrafficError("SAMPLE-RATE",dLine);
	  }
	  if(scdTraffic[dRecord].dSampleRate<8000 || 
	     scdTraffic[dRecord].dSampleRate>48000) {
	    TrafficError("SAMPLE-RATE",dLine);
	  }
	  break;
	case 15:    /* The SAMPLE-SIZE Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dSampleSize)!=1) {
	    TrafficError("SAMPLE-SIZE",dLine);
	  }
	  if(scdTraffic[dRecord].dSampleSize!=8 && 
	     scdTraffic[dRecord].dSampleSize!=16) {
	    TrafficError("SAMPLE-SIZE",dLine);
	  }
	  break;
	case 16:    /* The CHANNELS Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dChannels)!=1) {
	    TrafficError("CHANNELS",dLine);
	  }
	  if(scdTraffic[dRecord].dChannels<1 || 
	     scdTraffic[dRecord].dChannels>2) {
	    TrafficError("CHANNELS",dLine);
	  }
	  break;
	case 17:    /* The PIN Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dPin)!=1) {
	    TrafficError("PIN",dLine);
	  }
	  if(scdTraffic[dRecord].dPin<0 || 
	     scdTraffic[dRecord].dPin>7) {
	    TrafficError("PIN",dLine);
	  }
	  break;
	case 18:    /* The SILENCE-SENSE Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dSilence)!=1) {
	    TrafficError("SILENCE-SENSE",dLine);
	  }
	  if(scdTraffic[dRecord].dSilence<0 || 
	     scdTraffic[dRecord].dSilence>3600) {
	    TrafficError("SILENCE-SENSE",dLine);
	  }
	  break;
	case 19:    /* The THRESHOLD Field */
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].dThreshold)!=1) {
	    TrafficError("THRESHOLD",dLine);
	  }
	  if(scdTraffic[dRecord].dThreshold<0 || 
	     scdTraffic[dRecord].dThreshold>32768) {
	    TrafficError("THRESHOLD",dLine);
	  }
	  break;
	case 20:    /* The NAME Field */
	  l=strlen(sAccum);
	  if(l<256) {
	    j1=0;
	    for(i1=0;i1<l;i1++) { 
	      if(sAccum[i1]!='\\') {
		scdTraffic[dRecord].sCutName[j1++]=sAccum[i1];
	      }
	      else {
		if(sAccum[i1+1]=='r') {
		  scdTraffic[dRecord].sCutName[j1++]=10;
		}
		if(sAccum[i1+1]=='n') {
		  scdTraffic[dRecord].sCutName[j1++]=13;
		  scdTraffic[dRecord].sCutName[j1++]=10;
		}
		i1++;
	      }
	    }
	    /*
	    scdTraffic[dRecord].sCutName[j1++]=13;
	    scdTraffic[dRecord].sCutName[j1]=0;
	    */
	  }
	  else {
	    TrafficError("NAME",dLine);
	  }
	  break;
	case 21:    /* The Relays Fields */
	case 22:
	case 23:
	case 24:
	case 25:
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
	case 31:
	case 32:
	case 33:
	case 34:
	case 35:
	case 36:
	case 37:
	case 38:
	case 39:
	case 40:
	case 41:
	case 42:
	case 43:
	case 44:
	  if(sscanf(sAccum,"%d",&scdTraffic[dRecord].gpioMap[dField-21])!=1) {
	    TrafficError("RELAY MAP",dLine);
	  }
	  if(scdTraffic[dRecord].gpioMap[dField-21]<-1 || 
	     scdTraffic[dRecord].gpioMap[dField-21]>MAX_GPIOS) {
	    TrafficError("RELAY MAP",dLine);
	  }
	  break;
	default:    /* Unimplemented fields */
	  break;
	}
	i++;
	dField++;
      }
      
      
      dRecord++;
      dLine++;
      if(dRecord>dMaxEvents) {
	fclose(hFilename);
	printf("labd: too many events\n");
	exit(1);
      }
    }
    else {
      dLine++;
    }
  }

  /* ******* DEBUGGING SECTION ******* */
  /*
  for(i=0;i<dRecord;i++) {
    printf("--- TRAFFIC RECORD %d ---\n",i);
    printf("        MONTH: %d\n",scdTraffic[i].dOpenMonth);
    printf("          DAY: %d\n",scdTraffic[i].dOpenDay);
    printf("         YEAR: %d\n",scdTraffic[i].dOpenYear);
    printf("  DAY-OF-WEEK: %d\n",scdTraffic[i].dDayOfWeek);
    printf("    OPEN-HOUR: %d\n",scdTraffic[i].dOpenHour);
    printf("  OPEN-MINUTE: %d\n",scdTraffic[i].dOpenMinute);
    printf("  OPEN-SECOND: %d\n",scdTraffic[i].dOpenSecond);
    printf(" POST-COMMAND: %s\n",scdTraffic[i].sCommand);
    if(scdTraffic[i].dDeleteSource==0) {
      printf("DELETE-SOURCE: False\n");
    }
    else {
      printf("DELETE-SOURCE: True\n");
    }
    printf(" CLOSE-SECOND: %d\n",scdTraffic[i].dCloseSecond);
    printf("       LENGTH: %d\n",scdTraffic[i].dCutLength);
    printf("   EVENT-TYPE: %d\n",scdTraffic[i].dEventType);
    printf("     FILENAME: %s\n",scdTraffic[i].sFilename);
    printf("       DEVICE: %d\n",scdTraffic[i].dDevice);
    printf("  SAMPLE-RATE: %d\n",scdTraffic[i].dSampleRate);
    printf("  SAMPLE-SIZE: %d\n",scdTraffic[i].dSampleSize);
    printf("     CHANNELS: %d\n",scdTraffic[i].dChannels);
    printf("          PIN: %d\n",scdTraffic[i].dPin);
    printf("SILENCE-SENSE: %d\n",scdTraffic[i].dSilence);
    printf("    THRESHOLD: %d\n",scdTraffic[i].dThreshold);
    printf("         NAME: %s\n",scdTraffic[i].sCutName);
    printf("-- more --\n");
    getchar();
  }
  */
  /* ******* End of Debug Section ******** */

  return dRecord;
}


int WriteTraffic(char *sFilename,int dMaxEvents)
{
  int i;
  int j;
  FILE *hFilename;
  char sAccum[3];
  int dAccum;
  int dIndex[MAX_EVENTS];
  int dRank[MAX_EVENTS];
  int dChanged=1;

  /*
   * Initialize the sort structures
   */
  for(i=0;i<dMaxEvents;i++) {
    dIndex[i]=i;
    dRank[i]=3600*scdTraffic[i].dOpenHour+60*scdTraffic[i].dOpenMinute+
      scdTraffic[i].dOpenSecond;
  }

  /*
   * Sort the records
   */
  while(dChanged!=0) {
    dChanged=0;
    for(i=0;i<dMaxEvents-1;i++) {
      if(dRank[dIndex[i]]>dRank[dIndex[i+1]]) {
	dAccum=dIndex[i];
	dIndex[i]=dIndex[i+1];
	dIndex[i+1]=dAccum;
	dChanged++;
      }
    }
  }
	
  /*
   * Open the file for writing
   */
  hFilename=fopen(sFilename,"w");
  if(hFilename==NULL) {
    return -1;
  }

  /*
   * Write the comment header
   */
  fprintf(hFilename,"# This is the lab.traffic file.  It contains the feed schedule information\n");
  fprintf(hFilename,"# for the Linux Audio Backstop.\n");
  fprintf(hFilename,"#\n");
  fprintf(hFilename,"# This file has been machine generated and is not intended to be edited\n");
  fprintf(hFilename,"# manually.  For further information, see the 'lab(1)' manpage.\n");
  fprintf(hFilename,"#\n");
  fprintf(hFilename,"# by Fred Gleason %s\n",PACKAGE_BUGREPORT);

  for(i=0;i<dMaxEvents;i++) {
    if(scdTraffic[dIndex[i]].dOpenMonth==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenMonth);
    }
    if(scdTraffic[dIndex[i]].dOpenDay==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenDay);
    }
    if(scdTraffic[dIndex[i]].dOpenYear==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenYear);
    }
    if(scdTraffic[dIndex[i]].dDayOfWeek==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dDayOfWeek);
    }
    if(scdTraffic[dIndex[i]].dOpenHour==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenHour);
    }
    if(scdTraffic[dIndex[i]].dOpenMinute==WILDCARD) {
      fprintf(hFilename,"*,");
    }
    else {
      fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenMinute);
    }
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dOpenSecond);

    fprintf(hFilename,"%s,",scdTraffic[dIndex[i]].sCommand);
    fprintf(hFilename,"%d,",scdTraffic[dIndex[i]].dDeleteSource);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dCloseSecond);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dCutLength);
    GetTypeCode(scdTraffic[dIndex[i]].dEventType,sAccum);
    fprintf(hFilename,"%s,",sAccum);
    if((scdTraffic[dIndex[i]].dEventType==EVENT_RELAY_PULSE)||
       (scdTraffic[dIndex[i]].dEventType==EVENT_TIMED_SWITCHER)) {
      fprintf(hFilename,"n/a,");
    }
    else {
      fprintf(hFilename,"%s,",
	      scdTraffic[dIndex[i]].sFilename+strlen(sAudioroot));
    }
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dDevice);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dSampleRate);
    /* fprintf(hFilename,"%u,",strlen(sAudioroot));  */
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dSampleSize);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dChannels);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dPin);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dSilence);
    fprintf(hFilename,"%u,",scdTraffic[dIndex[i]].dThreshold);
    fprintf(hFilename,"%s,",scdTraffic[dIndex[i]].sCutName);
    for(j=0;j<MAX_GPIOS;j++) {
      fprintf(hFilename,"%d,",scdTraffic[dIndex[i]].gpioMap[j]);
    }
    fprintf(hFilename,"\n");
  }
  fprintf(hFilename,"\n");
  fprintf(hFilename,"# End of lab.traffic\n");
  fclose(hFilename);

  return 0;
}




void TrafficError(char *sFieldName,int dRecordNum)
{
  printf("labd: syntax error in %s field on line %d of traffic file\n",
	 sFieldName,dRecordNum);
  exit(1);
}



int IsComment(char *cBuffer)
{
  int i,l;

  l=strlen(cBuffer);
  for(i=0;i<l;i++) {
    if(cBuffer[i]=='#') {
      return 1;
    }
    if(cBuffer[i]!=' ' && cBuffer[i]!=9) {
      return 0;
    }
  }
  return 1;
}


int GetTypeCode(unsigned dEventType,char *sTypeCode)
{
  switch(dEventType) {
  case EVENT_TIMED_RECORD:
    strcpy(sTypeCode,"TR");
    return 0;
  case EVENT_TIMED_PLAYBACK:
    strcpy(sTypeCode,"TP");
    return 0;
  case EVENT_TIMED_SWITCHER:
    strcpy(sTypeCode,"TS");
    return 0;
  case EVENT_CLOSURE_RECORD:
    strcpy(sTypeCode,"CR");
    return 0;
  case EVENT_CLOSURE_PLAYBACK:
    strcpy(sTypeCode,"CP");
    return 0;
  case EVENT_CLOSURE_SWITCHER:
    strcpy(sTypeCode,"CS");
    return 0;
  case EVENT_THRESHOLD_RECORD:
    strcpy(sTypeCode,"SR");
    return 0;
  case EVENT_ROTATE_PLAYBACK:
    strcpy(sTypeCode,"RP");
    return 0;
  case EVENT_RELAY_PULSE:
    strcpy(sTypeCode,"PR");
    return 0;
  }
  return -1;
}





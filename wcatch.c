/* wcatch.c
 *
 * This is the WebCatcher interface for the Linux Audio Backstop
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

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include "cgilib.h"
#include "conflib.h"
#include "labd.h"


/*
 * Global Variables
 */
char sWavename[MAX_DECKS][256];
char sFeedname[MAX_DECKS][256];
int dWaveLength[MAX_DECKS];
char sLabels[MAX_SOURCES][256],sCommands[MAX_SOURCES][256];
volatile int dChildDone=0;
char sPost[POST_SIZE];
char sAccum[256];


int main(int argc,char *argv[])
{
  int dAccum;
  int dCommand;
  int dPid;

  /*
   * Load the context
   */
  LoadConfig(CONF_LOCATION);
  ReadPost(sPost,POST_SIZE);
  if(GetPostInt(sPost,"COMMAND",&dCommand)<0) {
    dCommand=0;;
  }
  signal(SIGCHLD,SigHandler);

  /*
   * Command-specific stuff
   */
  /*
  printf("Content-type: text/html\n\n");
  printf("COMMAND: %d\n",dCommand);
  exit(0);
  */
  switch(dCommand) {
  case WCOMMAND_NONE:
    ServeTop();
    ServeBasePage("");
    break;
    
  case WCOMMAND_RESTART:
    ServeTop();
    dPid=GetPid(PID_FILE);
    kill(dPid,SIGHUP);
    ServeBasePage("");
    break;

  case WCOMMAND_START:
    ServeTop();
    if(vfork()!=0) {
      execlp("labd",NULL);
      exit(0);
    }
    /*
    while(dChildDone==0) {
      sleep(1);
    }
    */
    ServeBasePage("");
    break;

  case WCOMMAND_STOP:
    ServeTop();
    dPid=GetPid(PID_FILE);
    kill(dPid,SIGTERM);
    ServeBasePage("");
    break;

  case WCOMMAND_CHANGE:
    ServeTop();
    if(GetPostInt(sPost,"LINE",&dAccum)<0) {
      Error("Missing/Invalid Line Number Argument!");
    }
    printf("<tr align=\"center\"><td>\n");
    ServeHeaderTable("Change Existing Record");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeChangePage(dAccum);
    printf("</td></tr>\n");
    break;

  case WCOMMAND_COMMIT_CHANGE:
    ServeTop();
    if(GetPostInt(sPost,"LINE",&dAccum)<0) {
      Error("Missing/Invalid Line Number Argument!");
    }
    if(CommitChange(dAccum)==0) {
      dPid=GetPid(PID_FILE);
      if(dPid>0) {
	kill(dPid,SIGHUP);
      }
      ServeBasePage("");
    }
    break;

  case WCOMMAND_DELETE:
    ServeTop();
    if(GetPostInt(sPost,"LINE",&dAccum)<0) {
      Error("Missing/Invalid Line Number Argument!");
    }
    printf("<tr align=\"center\"><td>\n");
    ServeHeaderTable("Delete Existing Record");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    printf("<big><big>Are you sure you want to delete this record?</big></big>");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>&nbsp;\n");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeDeletePage(dAccum);
    printf("</td></tr>\n");
    break;

  case WCOMMAND_COMMIT_DELETE:
    ServeTop();
    if(GetPostInt(sPost,"LINE",&dAccum)<0) {
      Error("Missing/Invalid Line Number Argument!");
    }
    if(CommitDelete(dAccum)==0) {
      dPid=GetPid(PID_FILE);
      if(dPid>0) {
	kill(dPid,SIGHUP);
      }
      ServeBasePage("");
    }
    break;

  case WCOMMAND_ADDNEW:
    ServeTop();
    printf("<tr align=\"center\"><td>\n");
    ServeHeaderTable("Add New Record");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeAddPage(dAccum);
    printf("</td></tr>\n");
    break;
   
  case WCOMMAND_SECONDADDNEW:
    ServeTop();
    printf("<tr align=\"center\"><td>\n");
    ServeHeaderTable("Add New Record");
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeSecondAddPage(dAccum); 
    printf("</td></tr>\n");
    break;

  case WCOMMAND_COMMIT_ADDNEW:
    ServeTop();
    if(CommitAdd()==0) {
      dPid=GetPid(PID_FILE);
      if(dPid>0) {
	kill(dPid,SIGHUP);
      }
      ServeBasePage("");
    }
    break;

  case WCOMMAND_HELP:
    printf("Location: %s\n\n",sHelpname);
    exit(0);
    break;
  }

  /*
   * The foot of the overall page
   */
  printf("</table>\n");
  printf("</body>\n");
  printf("</html>\n");
  
  exit(0);
}


void ServeTop(void)
{
  /*
   * The head of the overall page
   */
  printf("Content-type: text/html\n\n");
  printf("<html>\n");
  printf("<head>\n");
  printf("<title>The Linux Audio Backstop</title>\n");
  printf("</head>\n");
  printf("<body bgcolor=\"%s\">\n",WEB_BACK_COLOR);
  printf("<table border=\"0\" cellpadding=\"0\" cellspacing=\"0\" width=\"760\">\n");
}




void ServeBasePage(char *sSubHead)
{
    printf("<tr align=\"center\"><td>\n");
    ServeHeaderTable(sSubHead);
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeStatusTable();
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeSpacerTable();
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeDeckTable();
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeSpacerTable();
    printf("</td></tr>\n");
    printf("<tr align=\"center\"><td>\n");
    ServeTrafficTable();
    printf("</td></tr>\n");
}


void ServeHeaderTable(char *sSubHead)
{
  time_t tTime;
  struct tm tmTime;

  printf("<table width=\"760\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n"); 
  printf("<tr align=\"center\">\n");
  printf("<td width=\"760\" colspan=\"2\"><h1>The Linux Audio Backstop</h1></td>\n");
  printf("</tr>\n");
  printf("<tr align=\"center\">\n");
  printf("<td width=\"760\" colspan=\"2\"><h2>%s</h2></td>\n",sSubHead);
  printf("</tr>\n");
  printf("<tr>\n");
  tTime=time(&tTime);
  tmTime=*localtime(&tTime);
  printf("<td width=\"380\" align=\"left\"><h4>Hostname: %s</h4></td>",getenv("HTTP_HOST"));
  printf("<td width=\"380\" align=\"right\"><h4>%s</h4></td>\n",ctime(&tTime));
  printf("</tr>\n");
  printf("</table>\n");
}



void ServeStatusTable(void)
{
  int dPid;

  printf("<table width=\"760\" border=\"2\" cellspacing=\"0\" cellpadding=\"0\">\n");
  printf("<tr><td>\n");
  printf("<table width=\"760\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n");
  printf("<tr align=\"center\" valign=\"center\">\n");
  printf("<td width=\"10\">&nbsp;</td>\n");
  printf("<td width=\"30\">\n");
  dPid=GetPid(PID_FILE);
  if(dPid<0) {
    printf("<img src=\"../red.png\"></td>\n");
    printf("<td width=\"100\">\n");
    printf("Current Status<br><strong>NOT RUNNING</strong></td>\n");
    printf("<td width=\"10\">&nbsp;</td>\n");
    printf("<td width=\"100\" valign=\"center\">\n");

    /*
     * Disabled for now
     * We may want to enable these buttons some day
     *
     printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
     printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
     WCOMMAND_START);
     printf("<input type=\"submit\" value=\"Start\">\n");
     printf("</form>\n");
    */
    printf("&nbsp;");
    
    printf("</td>");
    printf("<td width=\"100\">\n");
    printf("&nbsp;\n");
    printf("</td>\n");
    printf("<td width=\"410\">&nbsp;</td>\n");
  }
  else {
    printf("<img src=\"../green.png\"></td>\n");
    printf("<td width=\"100\">\n");
    printf("Current Status<br><strong>RUNNING</strong></td>\n");
    printf("<td width=\"10\">&nbsp;</td>\n");
    printf("<td width=\"100\" valign=\"center\">\n");

    /*
     * Disabled for now
     * We may want to enable these buttons some day
     *
     printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
     printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
     WCOMMAND_RESTART);
     printf("<input type=\"submit\" value=\"Restart\">\n");
     printf("</form>");
     printf("</td>\n");
     printf("<td width=\"100\">\n");
     printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
     printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
     WCOMMAND_STOP);
     printf("<input type=\"submit\" value=\"Stop\">\n");
     printf("</form>\n");
    */
    printf("&nbsp;");

    printf("</td>\n");
    printf("<td width=\"410\">&nbsp;</td>\n");
  }
  printf("<td>");
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_NONE);
  printf("<input type=\"submit\" value=\"Refresh\">\n");
  printf("</form>\n");
  printf("</td>\n");
  printf("</tr>\n");
  printf("</table>\n");
  printf("</td></tr>\n");
  printf("</table>\n");
}



void ServeDeckTable(void)
{
  int i;
  struct tm tmTime;
  time_t tEnd;

  for(i=0;i<MAX_DECKS;i++) {
    ReadScoreBoard(SCOREBOARD_FILE,i);
  }
  printf("<table border=\"2\" cellpadding=\"0\" cellspacing=\"0\" width=\"760\" \n");

  printf("<tr><td align=\"center\" colspan=\"6\"><strong>DECK STATUS</strong></td></tr>\n");
  printf("<tr>\n");
  printf("<th>DECK</th>\n");
  printf("<th>STATUS</th>\n");
  printf("<th>FILE</th>\n");
  printf("<th>SOURCE</th>\n");
  printf("<th>STARTED</th>\n");
  printf("<th>ENDING</th>\n");
  printf("</tr>\n");    
  for(i=0;i<MAX_DECKS;i++) {
    printf("<tr align=\"center\"");
    if(IsOdd(i)) {
      printf(" bgcolor=\"%s\"",WEB_BAR_COLOR);
    }
    printf(">\n");
    printf("<td>%d</td>\n",i);
    printf("<td>");
    PrintDeckStatus(dDeckEvent[i]);
    printf("</td>\n");
    if((dDeckEvent[i]==DECK_RECORDING)||(dDeckEvent[i]==DECK_PLAYING)) {
      if(sWavename[i][0]!=0) {
	printf("<td>%s</td>\n",sWavename[i]);
      }
      else {
	printf("<td>&nbsp;</td>\n");
      }
      if(sFeedname[i][0]!=0) {
	printf("<td>%s</td>\n",sFeedname[i]);
      }
      else {
	printf("<td>&nbsp;</td>\n");
      }
      tmTime=*localtime(&tmDeckStart[i]);
      printf("<td>%02d:%02d:%02d</td>\n",tmTime.tm_hour,tmTime.tm_min,
	     tmTime.tm_sec);
      tEnd=tmDeckStart[i]+dWaveLength[i];
      tmTime=*localtime(&tEnd);
      if(dDeckEvent[i]==EVENT_TIMED_RECORD) {
	printf("<td>%02d:%02d:%02d</td>\n",tmTime.tm_hour,tmTime.tm_min,
	     tmTime.tm_sec);
      }
      else {
	printf("<td>------</td>\n");
      }
    }
    else {
      printf("<td>------</td>\n");
      printf("<td>------</td>\n");
      printf("<td>------</td>\n");
      printf("<td>------</td>\n");
    }
    printf("</tr>\n");
  }  
  printf("</table>\n");
}



void ServeTrafficTable(void)
{
  int i;
  int dMin,dSec;

  printf("<table border=\"2\" cellpadding=\"0\" cellspacing=\"0\" width=\"760\" \n");
  printf("<tr><td align=\"center\" colspan=\"10\"><strong>TRAFFIC FEEDS</strong></td></tr>\n");

  /*
   * Table Headings
   */
  printf("<tr>\n");
  printf("<th>DECK</th>\n");
  printf("<th>TIME</th>\n");
  printf("<th>DAY OF WEEK</th>\n");
  printf("<th>PIN</th>\n");
  printf("<th>LENGTH</th>\n");
  printf("<th>TYPE</th>\n");
  printf("<th>FILE</th>\n");
  printf("<th>SOURCE</th>\n");
  printf("<th>&nbsp;</th>\n");
  printf("<th>&nbsp;</th>\n");
  printf("</tr>\n");    

  /*
   * Traffic Data
   */
  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS);
  for(i=0;i<dTrafficRecords;i++) {
    printf("<tr align=\"center\"");
    if(IsOdd(i)) {
      printf(" bgcolor=\"%s\"",WEB_BAR_COLOR);
    }
    printf(">\n");
    /*
     * Deck Number
     */
    if(scdTraffic[i].dEventType==EVENT_RELAY_PULSE) {
      printf("<td>--</td>\n");
    }
    else {
      printf("<td>%d</td>\n",scdTraffic[i].dDevice);
    }

    /*
     * Start Time
     */
    printf("<td>");
    if(scdTraffic[i].dOpenHour!=WILDCARD) {
      printf("%02d:",scdTraffic[i].dOpenHour);
    }
    else {
      printf("*:");
    }
    if(scdTraffic[i].dOpenMinute!=WILDCARD) {
      printf("%02d:",scdTraffic[i].dOpenMinute);
    }
    else {
      printf("*:");
    }
    printf("%02d</td>\n",scdTraffic[i].dOpenSecond);
    /*
     * Day of the Week
     */
    printf("<td>\n");
    printf("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n");
    printf("<tr>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&1)!=0) {  /* Sunday */
      printf("Su");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&2)!=0) {  /* Monday */
      printf("Mo");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&4)!=0) {  /* Tuesday */
      printf("Tu");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&8)!=0) {  /* Wednesday */
      printf("We");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&16)!=0) {  /* Thursday */
      printf("Th");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&32)!=0) {  /* Friday */
      printf("Fr");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("<td width=\"20\">");
    if((scdTraffic[i].dDayOfWeek&64)!=0) {  /* Saturday */
      printf("Sa");
    }
    else {
      printf("&nbsp;");
    }
    printf("</td>\n");
    printf("</tr>\n");
    printf("</table>\n");
    printf("</td>\n");
    /*
     * Pin Number
     */
    if((scdTraffic[i].dEventType==EVENT_CLOSURE_RECORD)||
       (scdTraffic[i].dEventType==EVENT_CLOSURE_SWITCHER)||
       (scdTraffic[i].dEventType==EVENT_RELAY_PULSE)||
       (scdTraffic[i].dEventType==EVENT_CLOSURE_PLAYBACK)) {
      printf("<td>%d</td>\n",scdTraffic[i].dPin);
    }
    else {
      printf("<td>---</td>\n");
    }
    /*
     * Cut Length
     */
    if((scdTraffic[i].dEventType==EVENT_RELAY_PULSE)||
       (scdTraffic[i].dEventType==EVENT_TIMED_SWITCHER)) {
      printf("<td>-----</td>\n");
    }
    else {
      dMin=scdTraffic[i].dCutLength/60;
      dSec=scdTraffic[i].dCutLength-60*dMin;
      printf("<td>%02d:%02d</td>\n",dMin,dSec);
    }

    /*
     * Event Type
     */
    printf("<td>");
    PrintEventType(scdTraffic[i].dEventType);
    printf("</td>\n");
    /*
     * File Name
     */
    if((scdTraffic[i].dEventType==EVENT_RELAY_PULSE)||
       (scdTraffic[i].dEventType==EVENT_TIMED_SWITCHER)) {
      printf("<td>-----</td>\n");
    }
    else {
      printf("<td>%s</td>\n",scdTraffic[i].sFilename+strlen(sAudioroot));
    }
    /*
     * Audio Source
     */
    if((scdTraffic[i].dEventType==EVENT_TIMED_RECORD)||
       (scdTraffic[i].dEventType==EVENT_CLOSURE_RECORD)||
       (scdTraffic[i].dEventType==EVENT_TIMED_PLAYBACK)||
       (scdTraffic[i].dEventType==EVENT_TIMED_SWITCHER)) {
      printf("<td>%s</td>\n",scdTraffic[i].sCutName);
    }
    else {
      printf("<td>------</td>\n");
    }
    /*
     * The CHANGE Button
     */
    printf("<td><form action=\"wcatch.cgi\" method=\"post\">\n");
    printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	   WCOMMAND_CHANGE);
    printf("<input type=\"hidden\" name=\"LINE\" value=\"%d\">\n",i);
    printf("<input type=\"submit\" value=\"Change\">\n");
    printf("</form></td>\n");
    /*
     * The DELETE Button
     */
    printf("<td><form action=\"wcatch.cgi\" method=\"post\">\n");
    printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	   WCOMMAND_DELETE);
    printf("<input type=\"hidden\" name=\"LINE\" value=\"%d\">\n",i);
    printf("<input type=\"submit\" value=\"Delete\">\n");
    printf("</form></td>\n");

  }
  printf("</tr>\n");
  printf("<tr align=\"center\"><td colspan=\"10\">\n");
  /*
   * The ADD NEW FEED Button
   */
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_ADDNEW);
  printf("<input type=\"submit\" value=\"Add New Feed\">\n");
  printf("</form></td>\n");
  printf("</tr>\n");

  printf("</table>\n");
}



void ServeSpacerTable(void)
{
  printf("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"760\"\n");
  printf("<tr><td align=\"center\">&nbsp;</td></tr>\n");
  printf("</table>\n");
}



void ServeChangePage(int dLine)
{
  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS);
  ServeTrafficRecord(dLine,WCOMMAND_COMMIT_CHANGE,0);
}




void ServeDeletePage(int dLine)
{
  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS);
  ServeTrafficRecord(dLine,WCOMMAND_COMMIT_DELETE,0);
}




void ServeAddPage(int dLine)
{
  int i;

  printf("<table width=\"760\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\n");
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"100\" align=\"right\">\n");
  printf("</strong>Deck to use: </strong>\n");
  printf("</td>\n");
  printf("<td width=\"75\" align=\"left\" valign=\"top\">\n");
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_SECONDADDNEW);
  printf("<select name=\"DEVICE\">\n");
  for(i=0;i<MAX_DECKS;i++) {
    if(dAudioDevice[i]==1) {
      printf("<option> %d\n",i);
    }
  }
  printf("</select>\n");
  printf("</td>\n");
  printf("<td width=\"75\" align=\"center\" valign=\"top\">\n");
  printf("<input type=\"submit\" value=\"OK\">\n"); 
  printf("</form></td>\n");
  printf("<td width=\"75\" align=\"center\" valign=\"top\">\n");
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_NONE);
  printf("<input type=\"submit\" value=\"Cancel\">\n"); 
  printf("</form></td>\n");
  printf("<td width=\"435\">&nbsp;</td>\n");
  printf("</tr>\n");

  printf("</table>\n");
}




void ServeSecondAddPage(int dLine)
{
  int i;

  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS)+1;

  /*
   * Get the deck number
   */
  GetPostInt(sPost,"DEVICE",&scdTraffic[dTrafficRecords].dDevice);

  /*
   * Initialize with some sane defaults
   */
  scdTraffic[dTrafficRecords].dOpenMonth=WILDCARD;
  scdTraffic[dTrafficRecords].dOpenDay=WILDCARD;
  scdTraffic[dTrafficRecords].dOpenYear=WILDCARD;
  scdTraffic[dTrafficRecords].dDayOfWeek=62;   /* Weekdays */
  scdTraffic[dTrafficRecords].dOpenHour=0;
  scdTraffic[dTrafficRecords].dOpenMinute=0;
  scdTraffic[dTrafficRecords].dOpenSecond=0;
  scdTraffic[dTrafficRecords].dCloseHour=0;
  scdTraffic[dTrafficRecords].dCloseMinute=0;
  scdTraffic[dTrafficRecords].dCloseSecond=0;
  scdTraffic[dTrafficRecords].dCutLength=0;
  scdTraffic[dTrafficRecords].dEventType=EVENT_TIMED_RECORD;
  scdTraffic[dTrafficRecords].sFilename[0]=0;
  scdTraffic[dTrafficRecords].dSampleRate=44100;
  scdTraffic[dTrafficRecords].dSampleSize=16;
  scdTraffic[dTrafficRecords].dChannels=2;
  scdTraffic[dTrafficRecords].dPin=0;
  scdTraffic[dTrafficRecords].dSilence=0;
  scdTraffic[dTrafficRecords].dThreshold=0;
  scdTraffic[dTrafficRecords].sCutName[0]=0;
  scdTraffic[dTrafficRecords].sCommand[0]=0;
  scdTraffic[dTrafficRecords].dDeleteSource=0;
  for(i=0;i<MAX_GPIOS;i++) {
    scdTraffic[dTrafficRecords].gpioMap[i]=-1;
  }

  ServeTrafficRecord(dTrafficRecords,WCOMMAND_COMMIT_ADDNEW,0);
}




void ServeTrafficRecord(int dLine,int dCommand,unsigned dError)
{
  int dMin,dSec;
  int dSources;
  int i;
  int j;
  int k;
  int gpios=0;

  switch(scdTraffic[dLine].dEventType) {
      case EVENT_TIMED_RECORD:
	gpios=GetGpioSize(0);
	break;

      case EVENT_TIMED_PLAYBACK:
	gpios=GetGpioSize(1);
	break;
  }
  printf("<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" width=\"760\">\n");

  /*
   * Error Warning
   */
  if(dError!=0) {
    printf("<tr><td width=\"760\" colspan=\"6\" align=\"center\"><font color=\"%s\">\n",
	   WEB_WARNING_COLOR);
    printf("WARNING: Errors have been found in your submission, and marked ");
    printf("with a \'***\'.  Please correct and try again!</font></td></tr>\n");
    printf("<tr><td colspan=\"6\">&nbsp;</td></tr>\n");
  }

  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");

  /*
   * Basic Navigation Stuff
   */
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",dCommand);
  printf("<input type=\"hidden\" name=\"LINE\" value=\"%d\">\n",dLine);

  /*
   * Deck Number
   */
  printf("<tr bgcolor=\"%s\"><td width=\"760\" align=\"center\" colspan=\"6\">\n",
	 WEB_GROUP_COLOR);
  printf("<big><strong>AUDIO SETTINGS</strong></big></td></tr>\n");
  printf("<tr bgcolor=\"%s\"><td width=\"100\" align=\"right\"><strong>",
	 WEB_GROUP_COLOR);
  if((dError&WERROR_DEVICE)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#deck\" target=\"docs\">DECK</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"153\" align=\"left\">&nbsp;%d</td>\n",scdTraffic[dLine].dDevice);
  printf("<td width=\"100\">&nbsp;</td><td width=\"153\">&nbsp;</td>\n");
  printf("<td width=\"100\">&nbsp;</td><td width=\"153\">&nbsp;</td></tr>\n");
  printf("<input type=\"hidden\" name=\"DEVICE\" value=\"%d\">\n",
	 scdTraffic[dLine].dDevice);

  /*
   * File Name
   */
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_FILENAME)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#filename\" target=\"docs\">FILENAME</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"90\" align=\"left\" bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<input type=\"text\" name=\"FILENAME\" value=\"%s\" ",
	 scdTraffic[dLine].sFilename+strlen(sAudioroot));
  printf("length=\"10\" maxlength=\"20\"></td>\n");
  
  /*
   * Length
   */
  dMin=scdTraffic[dLine].dCutLength/60;
  dSec=scdTraffic[dLine].dCutLength-60*dMin;
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_CUTLENGTH)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#length\" target=\"docs\">LENGTH</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<input type=\"text\" name=\"LENGTH_MIN\" value=\"%02d\" ",dMin);
  printf("size=\"2\" maxlength=\"2\">:\n");
  printf("<input type=\"text\" name=\"LENGTH_SEC\" value=\"%02d\" ",dSec);
  printf("size=\"2\" maxlength=\"2\">\n");
  printf("</td>\n");

  /*
   * Source
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_CUTNAME)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#source\" target=\"docs\">SOURCE</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"90\" align=\"left\"><select name=\"SOURCE\" length=\"15\">\n");
  dSources=LoadSwitchCommands(scdTraffic[dLine].dDevice);
  if(dSources>0) {
    for(i=0;i<dSources;i++) {
      printf("<option");
      if(strcmp(scdTraffic[dLine].sCutName,sLabels[i])==0) {
	printf(" selected");
      }
      printf("> %s\n",sLabels[i]);
    }
  }
  else {
    printf("<option> FIXED\n"); 
  }
  printf("</select></td></tr>\n");

  /*
   * Event Type
   */
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_EVENTTYPE)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#type\" target=\"docs\">TYPE</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"90\" align=\"left\"><select name=\"TYPE\" length=\"15\">\n");
  printf("  <option");
  if(scdTraffic[dLine].dEventType==EVENT_TIMED_RECORD) {
    printf(" selected");
  }
  printf("> Timed Record\n");
  printf("  <option");
  if(scdTraffic[dLine].dEventType==EVENT_TIMED_PLAYBACK) {
    printf(" selected");
  }
  printf("> Timed Playback\n");
  printf("  <option");
  if(scdTraffic[dLine].dEventType==EVENT_TIMED_SWITCHER) {
    printf(" selected");
  }
  printf("> Timed Switch\n");
  printf("  <option");
  if(scdTraffic[dLine].dEventType==EVENT_RELAY_PULSE) {
    printf(" selected");
  }
  printf("> Pulse Relay\n");
  printf("</select></td>\n");

  /*
   * Pin Number
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_PIN)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#pin\" target=\"docs\">PIN</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<input type=\"text\" name=\"PIN\" value=\"%d\" ",
	 scdTraffic[dLine].dPin);
  printf("size=\"2\" maxlength=\"2\"></td>\n");

  /*
   * Channels
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_CHANNELS)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#channels\" target=\"docs\">CHANNELS</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<select name=\"CHANNELS\">\n");
  printf("<option");
  if( scdTraffic[dLine].dChannels==1) {
    printf(" selected");
  }
  printf("> Mono\n");
  printf("<option");
  if( scdTraffic[dLine].dChannels==2) {
    printf(" selected");
  }
  printf("> Stereo\n");
  printf("</select></td>\n");
  printf("</tr>\n");

  /*
   * Spacer
   */
  printf("<tr><td colspan=\"6\">&nbsp;</td></tr>\n");

  /*
   * Date
   */
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"760\" colspan=\"6\" align=\"center\">\n");
  printf("<big><strong>DATE/TIME SETTINGS</strong></big></td></tr>\n");
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"100\" align=\"right\"><strong>");
  if(((dError&WERROR_OPENMONTH)!=0)||
     ((dError&WERROR_OPENDAY)!=0)||
     ((dError&WERROR_OPENYEAR)!=0)) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#date\" target=\"docs\">DATE</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"153\" align=\"left\">\n");
  if(scdTraffic[dLine].dOpenMonth!=WILDCARD) {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENMONTH\" value=\"%02d\">-\n",scdTraffic[dLine].dOpenMonth);
  }
  else {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENMONTH\" value=\"*\">-");
  }    
  if(scdTraffic[dLine].dOpenDay!=WILDCARD) {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENDAY\" value=\"%02d\">-\n",scdTraffic[dLine].dOpenDay);
  }
  else {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENDAY\" value=\"*\">-");
  }    
  if(scdTraffic[dLine].dOpenYear!=WILDCARD) {
    printf("<input type=\"text\" size=\"4\" maxlength=\"4\" name=\"OPENYEAR\" value=\"%04d\">\n",scdTraffic[dLine].dOpenYear);
  }
  else {
    printf("<input type=\"text\" size=\"4\" maxlength=\"4\" name=\"OPENYEAR\" value=\"*\">\n");
  }
  printf("</td>\n");
  
  /*
   * Time
   */
  printf("<td width=\"100\" align=\"right\"><strong>");
  if(((dError&WERROR_OPENHOUR)!=0)||
     ((dError&WERROR_OPENMINUTE)!=0)||
     ((dError&WERROR_OPENSECOND)!=0)) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#time\" target=\"docs\">TIME</a>:</strong></td>\n",
	 sHelpname);
  printf("<td width=\"153\" align=\"left\">\n");
  if(scdTraffic[dLine].dOpenHour!=WILDCARD) {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENHOUR\" value=\"%02d\">:\n",scdTraffic[dLine].dOpenHour);
  }
  else {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENHOUR\" value=\"*\">:");
  }    
  if(scdTraffic[dLine].dOpenMinute!=WILDCARD) {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENMINUTE\" value=\"%02d\">:\n",scdTraffic[dLine].dOpenMinute);
  }
  else {
    printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENMINUTE\" value=\"*\">:");
  }    
  printf("<input type=\"text\" size=\"2\" maxlength=\"2\" name=\"OPENSECOND\" value=\"%02d\">\n",scdTraffic[dLine].dOpenSecond);
  printf("</td>\n");
  
  /*
   * Placeholder
   */
  printf("<td width=\"100\" align=\"right\">&nbsp;</td>\n");
  printf("<td width=\"153\" align=\"left\">&nbsp;</td></tr>\n");

  /*
   * Spacer
   */
  printf("<tr bgcolor=\"%s\"><td colspan=\"6\">&nbsp;</td></tr>\n",
	 WEB_GROUP_COLOR);

  /*
   * Day of the Week
   */
  printf("<tr bgcolor=\"%s\"><td width=\"100\" align=\"right\"><strong>",
	 WEB_GROUP_COLOR);
  if((dError&WERROR_DAYOFWEEK)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#dayofweek\" target=\"docs\">DAY OF THE WEEK</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"660\" align=\"left\" colspan=\"5\">\n");
  printf("<table width=\"630\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n");
  printf("<tr bgcolor=\"%s\" align=\"center\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY0\" value=\"1\"");
  if((scdTraffic[dLine].dDayOfWeek&1)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Sun</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY1\" value=\"2\"");
  if((scdTraffic[dLine].dDayOfWeek&2)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Mon</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY2\" value=\"4\"");
  if((scdTraffic[dLine].dDayOfWeek&4)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Tue</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY3\" value=\"8\"");
  if((scdTraffic[dLine].dDayOfWeek&8)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Wed</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY4\" value=\"16\"");
  if((scdTraffic[dLine].dDayOfWeek&16)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Thu</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY5\" value=\"32\"");
  if((scdTraffic[dLine].dDayOfWeek&32)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Fri</td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DAY6\" value=\"64\"");
  if((scdTraffic[dLine].dDayOfWeek&64)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("Sat</td>\n");
  printf("</tr>\n");
  printf("</table>\n");
  printf("</td></tr>\n");

  /*
   * Spacer
   */
  printf("<tr><td colspan=\"6\">&nbsp;</td></tr>\n");

  /*
   * Sample Rate
   */
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"760\" colspan=\"6\" align=\"center\">\n");
  printf("<big><strong>ADVANCED SETTINGS</strong></big></td></tr>\n");
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);  
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_SAMPLERATE)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#samplerate\" target=\"docs\">SAMPLE RATE</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<select name=\"SAMPLERATE\">\n");
  printf("<option");
  if( scdTraffic[dLine].dSampleRate==32000) {
    printf(" selected");
  }
  printf("> 32 kHz\n");
  printf("<option");
  if( scdTraffic[dLine].dSampleRate==44100) {
    printf(" selected");
  }
  printf("> 44.1 kHz\n");
  printf("<option");
  if( scdTraffic[dLine].dSampleRate==48000) {
    printf(" selected");
  }
  printf("> 48 kHz\n");
  printf("</select></td>\n");

  /*
   * Sample Size
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_SAMPLESIZE)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#samplesize\" target=\"docs\">SAMPLE SIZE</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<select name=\"SAMPLESIZE\">\n");
  printf("<option");
  if( scdTraffic[dLine].dSampleSize==8) {
    printf(" selected");
  }
  printf("> 8 bits/sample\n");
  printf("<option");
  if( scdTraffic[dLine].dSampleSize==16) {
    printf(" selected");
  }
  printf("> 16 bits/sample\n");
  printf("</select></td>\n");

  /*
   * Post Command
   */
  EscapeQuotes(scdTraffic[dLine].sCommand,scdTraffic[dLine].sResolvedCmd,1023);
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_POSTCOMMAND)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#postcommand\" target=\"docs\">COMMAND</a>:",sHelpname);
  printf("</strong></td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"text\" size=\"15\" maxlength=\"254\" name=\"POSTCOMMAND\" value=\"%s\">\n",scdTraffic[dLine].sResolvedCmd);
  printf("</td>\n");
  printf("</tr>\n");

  /*
   * Auto-Trim Holdoff
   */
  printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_SILENCE)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#silence\" target=\"docs\">AUTOTRIM HOLDOFF</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("<input type=\"text\" name=\"SILENCESENSE\" value=\"%d\" ",
	 scdTraffic[dLine].dSilence);
  printf("size=\"2\" maxlength=\"2\">sec</td>\n");

  /*
   * Auto-Trim Threshold
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  if((dError&WERROR_THRESHOLD)!=0) {
    printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
  }
  printf("<a href=\"%s#silence\" target=\"docs\">AUTOTRIM THRESHOLD</a>:</strong></td>\n",sHelpname);
  printf("<td width=\"90\" align=\"left\">\n");
  printf("-<input type=\"text\" name=\"THRESHOLD\" value=\"%d\" ",
	 scdTraffic[dLine].dThreshold);
  printf("size=\"2\" maxlength=\"2\">dBu</td>\n");

  /*
   * Delete Source
   */
  printf("<td width=\"90\" align=\"right\"><strong>");
  printf("<a href=\"%s#deletesource\" target=\"docs\">DELETE SOURCE</a>:",
	 sHelpname);

  printf("</strong></td>\n");
  printf("<td width=\"90\">\n");
  printf("<input type=\"checkbox\" name=\"DELETESOURCE\" value=\"1\"");
  if((scdTraffic[dLine].dDeleteSource)!=0) {
    printf("checked");
  }
  printf(">\n");
  printf("</td>\n");
  printf("</tr>\n");

  /*
   * GPIO Widgets
   */
  if(gpios>0) {
    /*
     * Spacer
     */
    printf("<tr><td colspan=\"6\">&nbsp;</td></tr>\n");
    
    /*
     * Relay Map
     */
    printf("<tr><td colspan=\"6\">\n");
    printf("<table width=\"780\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\">\n");
    printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);
    printf("<td width=\"780\" colspan=\"16\" align=\"center\">\n");
    printf("<big><strong><a href=\"%s#relaymap\" target=\"docs\">RELAY MAP</a></strong></big></td>\n",sHelpname);
    for(i=0;i<gpios;i+=8) {
      printf("<tr bgcolor=\"%s\">\n",WEB_GROUP_COLOR);  
      for(j=0;j<8;j++) {
	printf("<td align=\"right\"><strong>");
	if((dError&WERROR_SAMPLERATE)!=0) {
	  printf("<font color=\"%s\">*** </font>",WEB_WARNING_COLOR);
	}
	printf("%d:</strong></td>\n",i+j);
	printf("<td align=\"left\">\n");
	printf("<select name=\"RELAY%02d\">\n",i+j);
	printf("<option");
	if( scdTraffic[dLine].gpioMap[i+j]==-1) {
	  printf(" selected");
	}
	printf("> Off\n");
	for(k=0;k<gpios;k++) {
	  printf("<option");
	  if( scdTraffic[dLine].gpioMap[i+j]==k) {
	    printf(" selected");
	  }
	  printf("> %d\n",k);
	}
	printf("</select></td>\n");
      }
    }
    printf("</tr>\n");
    printf("</table>\n");
    printf("</td></tr>\n");
  }

  /*
   * Action Buttons
   */
  printf("<tr><td width=\"90\" align=\"right\">\n");
  /*
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_HELP);
  printf("<input type=\"submit\" value=\"Help\">\n");
  printf("</form></td>\n");
  */
  printf("</td>\n");
  printf("<td width=\"270\" colspan=\"3\">&nbsp;</td>\n");
  printf("<td width=\"90\" align=\"right\" valign=\"top\">\n");
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",dCommand);
  printf("<input type=\"submit\" value=\"OK\">\n");
  printf("</form>\n");
  printf("</td>\n");
  printf("<td width=\"90\" align=\"left\" valign=\"top\">\n");
  printf("<form action=\"wcatch.cgi\" method=\"post\">\n");
  printf("<input type=\"hidden\" name=\"COMMAND\" value=\"%d\">\n",
	 WCOMMAND_NONE);
  printf("<input type=\"submit\" value=\"Cancel\">\n");
  printf("</form></td></tr>\n");

  printf("</table>\n");
}




int CommitChange(int dLine)
{
  struct scdTraffic scdRecord;
  unsigned dError;

  dError=ReadTrafficPost(&scdRecord);
  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS);
  scdTraffic[dLine]=scdRecord;
  if(dError==0) {   /* All well, go for it! */
    WriteTraffic(sTrafficname,dTrafficRecords);
    return 0;
  }
  ServeHeaderTable("Change Existing Record");
  ServeTrafficRecord(dLine,WCOMMAND_COMMIT_CHANGE,dError);

  return -1;
}




int CommitDelete(int dLine)
{
  int i;

  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS)-1;
  for(i=dLine;i<dTrafficRecords;i++) {
    scdTraffic[i]=scdTraffic[i+1];
  }
  WriteTraffic(sTrafficname,dTrafficRecords);

  return 0;
}




int CommitAdd()
{
  struct scdTraffic scdRecord;
  unsigned dError;

  dError=ReadTrafficPost(&scdRecord);
  dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS)+1;
  scdTraffic[dTrafficRecords-1]=scdRecord;
  if(dError==0) {   /* All well, go for it! */
    WriteTraffic(sTrafficname,dTrafficRecords);
    return 0;
  }
  ServeHeaderTable("Add Existing Record");
  ServeTrafficRecord(dTrafficRecords-1,WCOMMAND_COMMIT_ADDNEW,dError);

  return -1;
}




pid_t GetPid(char *sFilename)
{
  FILE *hFilename;
  long ldPid;

  hFilename=fopen(sFilename,"r");
  if(hFilename==NULL) {
    return -1;
  }
  if(fscanf(hFilename,"%ld",&ldPid)==1) {
    return (pid_t)ldPid;
  }
  return -1;
}



int ReadScoreBoard(char *sFilename,int deck)
{
  FILE *hFilename;
  char name[PATH_LEN];

  sprintf(name,"%s%d",sFilename,deck);
  hFilename=fopen(name,"r");
  if(hFilename==NULL) {
    dDeckEvent[deck]=DECK_UNAVAILABLE;
    return -1;
  }
  fgets(sAccum,255,hFilename);
  sscanf(sAccum,"%d",&dDeckEvent[deck]);
  fgets(sAccum,255,hFilename);
  sscanf(sAccum,"%s",sWavename[deck]);
  fgets(sAccum,255,hFilename);
  sscanf(sAccum,"%s",sFeedname[deck]);
  fgets(sAccum,255,hFilename);
  sscanf(sAccum,"%lu",&tmDeckStart[deck]);
  fgets(sAccum,255,hFilename);
  sscanf(sAccum,"%d",&dWaveLength[deck]);
  return 0;
}



void PrintDeckStatus(int dDeckStat)
{
  switch(dDeckStat) {
  case DECK_UNAVAILABLE:
    printf("Not Present");
    break;
  case DECK_IDLE:
    printf("Idle");
    break;
  case DECK_RECORDING:
    printf("Recording");
    break;
  case DECK_PLAYING:
    printf("Playing");
    break;
  case DECK_WAITFORCLOSE_RECORD:
    printf("Waiting for record closure");
    break;
  case DECK_WAITFORCLOSE_PLAY:
    printf("Waiting for play closure");
    break;
  case DECK_WAITFORCLOSE_SWITCH:
    printf("Waiting for switcher closure");
    break;
  case DECK_WAITFORSIGNAL_RECORD:
    printf("Waiting for record audio");
    break;
  default:
    printf("Unknown/undefined");
    break;
  }
}



void PrintEventType(int dEventType)
{
  switch(dEventType) {
  case EVENT_TIMED_RECORD:
    printf("Timed Record");
    break;
  case EVENT_TIMED_PLAYBACK:
    printf("Timed Playback");
    break;
  case EVENT_TIMED_SWITCHER:
    printf("Timed Switch");
    break;
  case EVENT_CLOSURE_RECORD:
    printf("Closure Record");
    break;
  case EVENT_CLOSURE_PLAYBACK:
    printf("Closure Playback");
    break;
  case EVENT_CLOSURE_SWITCHER:
    printf("Closure Switcher");
    break;
  case EVENT_THRESHOLD_RECORD:
    printf("Threshold Record");
    break;
  case EVENT_ROTATE_PLAYBACK:
    printf("Rotating Playback");
    break;
  case EVENT_RELAY_PULSE:
    printf("Pulse Relay");
    break;
  default:
    printf("Unknown");
    break;
  }
}




unsigned ReadTrafficPost(struct scdTraffic *scdRecord)
{
  int i;
  int dAccum=0;
  int dMin=0,dSec=0;
  char sAccum[256];
  unsigned dError=0;
  char temp[255];

  /*
   * Initialize the record
   */
  memset(scdRecord,0,sizeof(struct scdTraffic));
  for(i=0;i<MAX_GPIOS;i++) {
    scdRecord->gpioMap[i]=-1;
  }
  GetPostString(sPost,"OPENMONTH",sAccum,254);
  if(strcmp(sAccum,"*")==0) {
    scdRecord->dOpenMonth=WILDCARD;
  }
  else {
    if(GetPostInt(sPost,"OPENMONTH",&scdRecord->dOpenMonth)<0) {
      dError|=WERROR_OPENMONTH;
      scdRecord->dOpenMonth=1;
    }
  }
  GetPostString(sPost,"OPENDAY",sAccum,254);
  if(strcmp(sAccum,"*")==0) {
    scdRecord->dOpenDay=WILDCARD;
  }
  else {
    if(GetPostInt(sPost,"OPENDAY",&scdRecord->dOpenDay)<0) {
      dError|=WERROR_OPENDAY;
      scdRecord->dOpenDay=1;
    }
  }
  GetPostString(sPost,"OPENYEAR",sAccum,254);
  if(strcmp(sAccum,"*")==0) {
    scdRecord->dOpenYear=WILDCARD;
  }
  else {
    if(GetPostInt(sPost,"OPENYEAR",&scdRecord->dOpenYear)<0) {
      dError|=WERROR_OPENYEAR;
      scdRecord->dOpenYear=2000;
    }
  }
  for(i=0;i<7;i++) {
    sprintf(sAccum,"DAY%d",i);
    if(GetPostInt(sPost,sAccum,&dAccum)>=0) {
      scdRecord->dDayOfWeek|=dAccum;
    }
  }
  GetPostString(sPost,"OPENHOUR",sAccum,254);
  if(strcmp(sAccum,"*")==0) {
    scdRecord->dOpenHour=WILDCARD;
  }
  else {
    if(GetPostInt(sPost,"OPENHOUR",&scdRecord->dOpenHour)<0) {
      dError|=WERROR_OPENHOUR;
      scdRecord->dOpenHour=0;
    }
  }
  GetPostString(sPost,"OPENMINUTE",sAccum,254);
  if(strcmp(sAccum,"*")==0) {
    scdRecord->dOpenMinute=WILDCARD;
  }
  else {
    if(GetPostInt(sPost,"OPENMINUTE",&scdRecord->dOpenMinute)<0) {
      dError|=WERROR_OPENMINUTE;
      scdRecord->dOpenMinute=0;
    }
  }
  if(GetPostInt(sPost,"OPENSECOND",&scdRecord->dOpenSecond)<0) {
    dError|=WERROR_OPENSECOND;
    scdRecord->dOpenSecond=0;
  }
  if(GetPostString(sPost,"TYPE",sAccum,254)<0) {
    dError|=WERROR_EVENTTYPE;
    scdRecord->dEventType=EVENT_TIMED_RECORD;
  }
  if(!strcasecmp(sAccum,"Timed Record")) {
    scdRecord->dEventType=EVENT_TIMED_RECORD;
  }
  else {
    if(!strcasecmp(sAccum,"Timed Playback")) {
      scdRecord->dEventType=EVENT_TIMED_PLAYBACK;
    }
    else {
      if(!strcasecmp(sAccum,"Pulse Relay")) {
	scdRecord->dEventType=EVENT_RELAY_PULSE;
      }
      else {
	if(!strcasecmp(sAccum,"Timed Switch")) {
	  scdRecord->dEventType=EVENT_TIMED_SWITCHER;
	}
	else {
	  dError|=WERROR_EVENTTYPE;
	  scdRecord->dEventType=EVENT_TIMED_RECORD;
	}
      }
    }
  }
  if(scdRecord->dEventType==EVENT_TIMED_RECORD) {
    if(GetPostInt(sPost,"LENGTH_MIN",&dMin)<0) {
      dError|=WERROR_CUTLENGTH;
      scdRecord->dCutLength=0;
    }
    if(GetPostInt(sPost,"LENGTH_SEC",&dSec)<0) {
      dError|=WERROR_CUTLENGTH;
      scdRecord->dCutLength=0;
    }
    if((dError&WERROR_CUTLENGTH)==0) {
      scdRecord->dCutLength=60*dMin+dSec;
    }
  }
  else {
    scdRecord->dCutLength=0;
  }
  if((scdRecord->dEventType==EVENT_RELAY_PULSE)||
    (scdRecord->dEventType==EVENT_TIMED_SWITCHER)) {
    strcpy(scdRecord->sFilename,"n/a");
  }
  else {
    if(GetPostString(sPost,"FILENAME",sAccum,254)<0) {
      dError|=WERROR_FILENAME;
      scdRecord->sFilename[0]=0;
    }
    strcpy(scdRecord->sFilename,sAudioroot);
    strcat(scdRecord->sFilename,sAccum);
  }
  if(GetPostInt(sPost,"DEVICE",&scdRecord->dDevice)<0) {
    dError|=WERROR_DEVICE;
    scdRecord->dDevice=0;
  }
  if(GetPostString(sPost,"SAMPLERATE",sAccum,254)<0) {
    dError|=WERROR_SAMPLERATE;
    scdRecord->dSampleRate=44100;
  }
  scdRecord->dSampleRate=0;
  if(strcasecmp(sAccum,"32 kHz")==0) {
    scdRecord->dSampleRate=32000;
  }
  if(strcasecmp(sAccum,"44.1 kHz")==0) {
    scdRecord->dSampleRate=44100;
  }
  if(strcasecmp(sAccum,"48 kHz")==0) {
    scdRecord->dSampleRate=48000;
  }
  if(scdRecord->dSampleRate==0) {
    dError|=WERROR_SAMPLERATE;
    scdRecord->dSampleRate=44100;
  }
  if(GetPostInt(sPost,"SAMPLESIZE",&scdRecord->dSampleSize)<0) {
    dError|=WERROR_SAMPLESIZE;
    scdRecord->dSampleSize=16;
  }
  if(GetPostString(sPost,"CHANNELS",sAccum,254)<0) {
    dError|=WERROR_CHANNELS;
    scdRecord->dChannels=2;
  }
  if(strcasecmp(sAccum,"Mono")==0) {
    scdRecord->dChannels=1;
  }
  if(strcasecmp(sAccum,"Stereo")==0) {
    scdRecord->dChannels=2;
  }
  if(GetPostInt(sPost,"PIN",&scdRecord->dPin)<0) {
    dError|=WERROR_PIN;
    scdRecord->dPin=0;
  }
  if(GetPostInt(sPost,"SILENCESENSE",&scdRecord->dSilence)<0) {
    dError|=WERROR_SILENCE;
    scdRecord->dSilence=0;
  }
  if(GetPostInt(sPost,"THRESHOLD",&scdRecord->dThreshold)<0) {
    dError|=WERROR_THRESHOLD;
    scdRecord->dThreshold=0;
  }
  if(GetPostString(sPost,"SOURCE",scdRecord->sCutName,254)<0) {
    dError|=WERROR_CUTNAME;
    scdRecord->sCutName[0]=0;
  }
  if(GetPostString(sPost,"POSTCOMMAND",scdRecord->sCommand,254)<0) {
    dError|=WERROR_POSTCOMMAND;
    scdRecord->sCommand[0]=0;
  }
  if(GetPostString(sPost,"DELETESOURCE",sAccum,254)<0) {
    scdRecord->dDeleteSource=0;
  }
  else {
    scdRecord->dDeleteSource=1;
  }

  for(i=0;i<MAX_GPIOS;i++) {
    sprintf(temp,"RELAY%02d",i);
    GetPostInt(sPost,temp,&(scdRecord->gpioMap[i]));
  }

  /*
   * This is a hack to ensure sane values for the CLOSE-time fields
   * We may want to use these fields in future
   */
  scdRecord->dCloseHour=0;
  scdRecord->dCloseMinute=0;
  scdRecord->dCloseSecond=0;

  dError|=CheckTrafficRecord(scdRecord);
  return dError;
}




unsigned CheckTrafficRecord(struct scdTraffic *scdRecord)
{
  int dError=0;
  int i;

  if(((scdRecord->dOpenMonth<1)||(scdRecord->dOpenMonth>12))&&
     (scdRecord->dOpenMonth!=WILDCARD)) {
    dError|=WERROR_OPENMONTH;
  }
  if(((scdRecord->dOpenDay<1)||(scdRecord->dOpenDay>31))&&
     (scdRecord->dOpenDay!=WILDCARD)) {
    dError|=WERROR_OPENDAY;
  }
  if((scdRecord->dOpenYear<2000)&&(scdRecord->dOpenYear!=WILDCARD)) {
    dError|=WERROR_OPENYEAR;
  }
  if(((scdRecord->dOpenHour<0)||(scdRecord->dOpenHour>23))&&
     (scdRecord->dOpenHour!=WILDCARD)) {
    dError|=WERROR_OPENHOUR;
  }
  if(((scdRecord->dOpenMinute<0)||(scdRecord->dOpenMinute>59))&&
     (scdRecord->dOpenMinute!=WILDCARD)) {
    dError|=WERROR_OPENMINUTE;
  }
  if((scdRecord->dOpenSecond<0)||(scdRecord->dOpenSecond>60)) {
    dError|=WERROR_OPENSECOND;
  }
  if((scdRecord->dDayOfWeek<0)||(scdRecord->dDayOfWeek>127)) {
    dError|=WERROR_DAYOFWEEK;
  }

  /*
   * Commented out for now -- we may use this in future
   *
  if(((scdRecord->dCloseHour<0)||(scdRecord->dCloseHour>23))&&
     (scdRecord->dCloseHour!=WILDCARD)) {
    dError|=WERROR_CLOSEHOUR;
  }
  if(((scdRecord->dCloseMinute<0)||(scdRecord->dCloseMinute>59))&&
     (scdRecord->dCloseMinute!=WILDCARD)) {
    dError|=WERROR_CLOSEMINUTE;
  }
  if((scdRecord->dCloseSecond<0)||(scdRecord->dCloseSecond>60)) {
    dError|=WERROR_CLOSESECOND;
  }
  */

  if((scdRecord->dCutLength<=0)&&(scdRecord->dEventType==EVENT_TIMED_RECORD)) {
    dError|=WERROR_CUTLENGTH;
  }
  if((scdRecord->sFilename[0]==0)&&
     (scdRecord->dEventType!=EVENT_RELAY_PULSE)&&
     (scdRecord->dEventType!=EVENT_TIMED_SWITCHER)) {
    dError|=WERROR_FILENAME;
  }
  if((scdRecord->dEventType<0)||(scdRecord->dEventType>EVENT_LASTEVENT)) {
    dError|=WERROR_EVENTTYPE;
  }
  if((scdRecord->dSampleRate<0)||(scdRecord->dSampleRate>48000)) {
    dError|=WERROR_SAMPLERATE;
  }
  if((scdRecord->dSampleSize!=8)&&(scdRecord->dSampleSize!=16)) {
    dError|=WERROR_SAMPLESIZE;
  }
  if((scdRecord->dChannels!=1)&&(scdRecord->dChannels!=2)) {
    dError|=WERROR_CHANNELS;
  }
  if((scdRecord->dPin<0)||(scdRecord->dPin>24)) {
    dError|=WERROR_PIN;
  }
  if((scdRecord->dSilence<0)||(scdRecord->dSilence>60)) {
    dError|=WERROR_SILENCE;
  }
  if((scdRecord->dThreshold<0)||(scdRecord->dThreshold>100)) {
    dError|=WERROR_THRESHOLD;
  }
  for(i=0;i<MAX_GPIOS;i++) {
    if((scdRecord->gpioMap[i]<-1)||(scdRecord->gpioMap[i]>MAX_GPIOS)) {
      dError|=WERROR_RELAYS;
    }
  }
  return dError;
}



void SigHandler(int signo)
{
  while(waitpid(-1,NULL,WNOHANG)>0);
  dChildDone=1;

  return;
}




int LoadSwitchCommands(int dDevice)
{
  int i;
  char sSection[256];
  char sName[256];
  char sCommand[256];

  sprintf(sSection,"Deck%d",dDevice);
  for(i=0;i<MAX_SOURCES;i++) {
    sprintf(sName,"SourceName%d",i);
    sprintf(sCommand,"SourceCmd%d",i);
    GetPrivateProfileString(CONF_LOCATION,sSection,sCommand,sCommands[i],"",
			    256);
    if(GetPrivateProfileString(CONF_LOCATION,sSection,sName,sLabels[i],"",256)<0) {
      return i;
    }
  }
  return i;
}


int GetGpioSize(int direction)
{
#ifdef HAVE_GPIO
  struct gpio_info info;
  int size;
  int mode;
  int fd;

  if(direction==0) {
    fd=open(sGpoDevice,O_RDWR);
    if(fd<0) {
      return 0;
    }
    mode=GPIO_MODE_INPUT;
    ioctl(fd,GPIO_SETMODE,&mode);
    ioctl(fd,GPIO_GETINFO,&info);
    size=info.inputs;
  }
  else {
    fd=open(sGpoDevice,O_RDWR);
    if(fd<0) {
      return 0;
    }
    mode=GPIO_MODE_OUTPUT;
    ioctl(fd,GPIO_SETMODE,&mode);
    ioctl(fd,GPIO_GETINFO,&info);
    size=info.outputs;
  }
  close(fd);
  return size;
#endif  // HAVE_GPIO
  return 0;
}

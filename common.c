/* common.c
 * 
 * Common components for the Linux Audio Backstop
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
#include <pwd.h>
#include <grp.h>
#include <signal.h>
#include <sys/stat.h>
#include <netdb.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/resource.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <setjmp.h>
#include <string.h>
#include "wavlib.h"
#include "conflib.h"
#include "labd.h"



int LoadConfig(char *sFilename) 
{
  int i=0,j=0;
  char sSection[255],sLabel[255],sAccum[255];
  char temp[255];
  char sAddr[255];
  struct hostent *hostent;
  int port;
  struct passwd *passwd;
  struct group *group;

  /*
   * Global Values
   */
  GetPrivateProfileString(sFilename,"Global","TrafficFile",sTrafficname,
			  "/etc/lab.traffic",255);
  GetPrivateProfileString(sFilename,"Global","AudioRoot",sAudioroot,
			  "/usr/snd/",255);
  if(GetPrivateProfileString(sFilename,"Global","Owner",temp,"root",254)==0) {
    if((passwd=getpwnam(temp))==NULL) {
      fprintf(stderr,"no such user\n");
      exit(1);
    }
    dOwner=passwd->pw_uid;
  }
  if(GetPrivateProfileString(sFilename,"Global","Group",temp,"root",254)==0) {
    if((group=getgrnam(temp))==NULL) {
      fprintf(stderr,"labd: no such group\n");
      exit(1);
    }
    dGroup=group->gr_gid;
  }
  if(sAudioroot[strlen(sAudioroot)-1]!='/') {
    strcat(sAudioroot,"/");
  }
  GetPrivateProfileString(sFilename,"Global","HelpFile",sHelpname,
			  "/lab.html",255);
  GetPrivateProfileString(sFilename,"Global","GpiDevice",sGpiDevice,
			  "/dev/gpio0",255);
  GetPrivateProfileString(sFilename,"Global","GpoDevice",sGpoDevice,
			  "/dev/gpio0",255);
  GetPrivateProfileString(sFilename,"Global","InvertGpi",temp,"No",254);
  if(!strcasecmp(temp,"yes")) {
    dInvertGpi=1;
  }
  else {
    dInvertGpi=0;
  }
  GetPrivateProfileString(sFilename,"Global","UseRealtime",temp,"Yes",254);
  if(!strcasecmp(temp,"yes")) {
    dUseRealtime=1;
  }
  else {
    dUseRealtime=0;
  }

  /* 
   * Decks
   */
  sprintf(sSection,"Deck%d",i);
  while(GetPrivateProfileInt(sFilename,sSection,"DeckActive",0)==1) {
    /*
     * First, the audio devices
     */
    if(GetPrivateProfileString(sFilename,sSection,"RecDevice",
			       sAudioDevice[i],"",255)==0) {
      dAudioDevice[i]=1;
      if(dDeckEvent[i]==DECK_UNAVAILABLE) {  /* So we don't overwrite */
	dDeckEvent[i]=DECK_IDLE;             /* scoreboard data!      */
      }
    }
    /*
     * Then, the switcher devices
     */
    GetPrivateProfileString(sFilename,sSection,"SwitchType",temp,"serial",7);
    if(!strcasecmp(temp,"serial")) {
      dSwitcherType[i]=SWITCHER_TYPE_SERIAL;
    }
    if(!strcasecmp(temp,"udp")) {
      dSwitcherType[i]=SWITCHER_TYPE_UDP;
    }
    if(GetPrivateProfileString(sFilename,sSection,"SwitchDevice",
			       sSwitcherDevice[i],"",255)==0) {
      dSwitcherDevice[i]=1;
    }
    switch(dSwitcherType[i]) {
	case SWITCHER_TYPE_UDP:
	  j=0;
	  while((sSwitcherDevice[i][j]!=':')&&
		(j<strlen(sSwitcherDevice[i]))) {
	    sAddr[j]=sSwitcherDevice[i][j];
	    j++;
	  }
	  sAddr[j]=0;
	  if((hostent=gethostbyname(sAddr))==NULL) {
	    perror("labd");
	    return -1;
	  }
	  if(dDebug==1) {
	    printf("Resolved %s to %d.%d.%d.%d\n",sAddr,
		   hostent->h_addr[0]&255,
		   hostent->h_addr[1]&255,
		   hostent->h_addr[2]&255,
		   hostent->h_addr[3]&255);
	  }
	  if(sscanf(sSwitcherDevice[i]+j+1,"%d",&port)!=1) {
	    fprintf(stderr,"labd: invalid port number\n");
	    return -1;
	  }
	  memset(&inSwitcherAddress[i],0,sizeof(struct sockaddr_in));
	  inSwitcherAddress[i].sin_family=AF_INET;
	  inSwitcherAddress[i].sin_port=htons((uint16_t)port);
	  inSwitcherAddress[i].sin_addr.s_addr=
	    *((uint32_t *)hostent->h_addr);
	  break;
    }
    dSwitcherBaud[i]=GetTtySpeed(GetPrivateProfileInt(sFilename,
						      sSection,"SwitchBaud",
						      9600));
    /*
     * The switcher command terminator
     */
    GetPrivateProfileString(sFilename,sSection,"CmdTerm",sAccum,"none",5);
    dCmdTerm[i]=-1;
    if(strcasecmp(sAccum,"none")==0) {
      dCmdTerm[i]=TERM_NONE;
      dCmdSize[i]=0;
      if(dDebug>0) {
	printf("Serial command terminator: none\n");
      }
    }
    if(strcasecmp(sAccum,"lf")==0) {
      dCmdTerm[i]=TERM_LF;
      dCmdSize[i]=1;
      if(dDebug>0) {
	printf("Serial command terminator: LF\n");
      }
    }
    if(strcasecmp(sAccum,"crlf")==0) {
      dCmdTerm[i]=TERM_CRLF;
      dCmdSize[i]=2;
      if(dDebug>0) {
	printf("Serial command terminator: CR/LF\n");
      }
    }
    if(strcasecmp(sAccum,"cr")==0) {
      dCmdTerm[i]=TERM_CR;
      dCmdSize[i]=1;
      if(dDebug>0) {
	printf("Serial command terminator: CR\n");
      }
    }
    if(dCmdTerm[i]==-1) {
      printf("labd: invalid switcher command terminator\n");
      return -1;
    }

    /*
     * Finally, the switcher source table 
     */
    j=0;
    sprintf(sLabel,"SourceName%d",j);
    while(GetPrivateProfileString(sFilename,sSection,sLabel,
				  ssSource[i][j].sName,"",254)==0) {
      sprintf(sLabel,"SourceCmd%d",j);
      GetPrivateProfileString(sFilename,sSection,sLabel,
			      ssSource[i][j].sCommand,"",254);
      sprintf(sLabel,"SourceName%d",++j);
    }
    dSourceQuan[i]=j;
    sprintf(sSection,"Deck%d",++i);
  }
  return 0;
}


void DisplayAddress(const char *label,struct sockaddr_in *sockaddr_in)
{
  char *addr_ptr=(char *)&sockaddr_in->sin_addr.s_addr;
  char *port_ptr=(char *)&sockaddr_in->sin_port;
  printf("%s: %d.%d.%d.%d:%d\n",label,
	 (int)(addr_ptr[0]&255),
	 (int)(addr_ptr[1]&255),
	 (int)(addr_ptr[2]&255),
	 (int)(addr_ptr[3]&255),
	 (int)(256*(port_ptr[0]&255)+(port_ptr[1]&255)));
}


void ResolveMacros(time_t tTime,const char *filename,const char *template,
		   char *output,int maxsize)
{
  struct tm *tm;
  int i=0;
  int j=0;

  tm=localtime(&tTime);
  for(i=0;i<strlen(template);i++) {
    if(template[i]=='%') {
      switch(template[++i]) {
	  case 'a':  // Abbreviated day of the week (sun - sat)
	    if((j+4)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    switch(tm->tm_wday) {
		case 0:  // Sunday
		  strcat(output,"sun");
		  break;

		case 1:  // Monday
		  strcat(output,"mon");
		  break;

		case 2:  // Tuesday
		  strcat(output,"tue");
		  break;

		case 3:  // Wednesday
		  strcat(output,"wed");
		  break;

		case 4:  // Thursday
		  strcat(output,"thu");
		  break;

		case 5:  // Friday
		  strcat(output,"fri");
		  break;

		case 6:  // Saturday
		  strcat(output,"sat");
		  break;
	    }
	    j+=3;
	    break;

	  case 'b':  // Abbreviated month name (jan - dec)
	    if((j+4)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    switch(tm->tm_mon) {
		case 0:  // January
		  strcat(output,"jan");
		  break;

		case 1:  // February
		  strcat(output,"feb");
		  break;

		case 2:  // March
		  strcat(output,"mar");
		  break;

		case 3:  // April
		  strcat(output,"apr");
		  break;

		case 4:  // May
		  strcat(output,"may");
		  break;

		case 5:  // June
		  strcat(output,"jun");
		  break;

		case 6:  // July
		  strcat(output,"jul");
		  break;

		case 7:  // August
		  strcat(output,"aug");
		  break;

		case 8:  // September
		  strcat(output,"sep");
		  break;

		case 9:  // October
		  strcat(output,"oct");
		  break;

		case 10:  // November
		  strcat(output,"nov");
		  break;

		case 11:  // December
		  strcat(output,"dec");
		  break;
	    }
	    j+=3;
	    break;

	  case 'd':  // Day of the month
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_mday);
	    j+=2;
	    break;

	  case 'f':  // Filename
	    if((j+strlen(filename)+1)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%s",filename);
	    j+=strlen(filename);
	    break;

	  case 'k':  // Hour
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_hour);
	    j+=2;
	    break;

	  case 'm':  // Month of the year
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_mon+1);
	    j+=2;
	    break;

	  case 'M':  // Minute
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_min);
	    j+=2;
	    break;

	  case 'S':  // Second
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_sec);
	    j+=2;
	    break;

	  case 'w':  // Day of the week (0 = Sunday)
	    if((j+3)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%02d",tm->tm_wday);
	    j+=2;
	    break;

	  case 'Y':  // Four digit year
	    if((j+5)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%04d",tm->tm_year+1900);
	    j+=4;
	    break;

	  case '%':  // Literal '%'
	    if((j+2)>maxsize) {
	      output[j]=0;
	      return;
	    }
	    sprintf(output+j,"%%");
	    j+=1;
	    break;

      }
    }
    else {
      if((j+2)>maxsize) {
	output[j]=0;
	return;
      }
      output[j++]=template[i];
    }
  }
}

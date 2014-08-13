/* conflib.c
 *
 * A small library for handling common configuration file tasks
 * Version 1.3.0
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include "conflib.h"

#define BUFFER_SIZE 1024

int GetPrivateProfileString(char *sFilename,char *cHeader,char *cLabel,
			    char *cValue,char *cDefault,int dValueLength)
{
  int i;
  
  i=GetIni(sFilename,cHeader,cLabel,cValue,dValueLength);
  if(i==0) {
    return 0;
  }
  else {
    strcpy(cValue,cDefault);
    return -1;
  }
}


int GetPrivateProfileInt(char *sFilename,char *cHeader,char *cLabel,
			 int dDefault)
{
  int c;
  char sNum[12];

  if(GetIni(sFilename,cHeader,cLabel,sNum,11)==0) {
    if(sscanf(sNum,"%d",&c)==1) {
      return c;
    }
    else {
      return dDefault;
    }
  }
  else {
    return dDefault;
  }
}



int GetIni(char *sFileName,char *cHeader,char *cLabel,char *cValue,int dValueLength)	/* get a value from the ini file */
     
{
  FILE *cIniName;
  char cIniBuffer[BUFFER_SIZE],cIniHeader[80],cIniLabel[80];
  int i=0,j=0;
  int iFileStat;

  cIniName=fopen(sFileName,"r");
  if(cIniName==NULL) {
    return 2;	/* ini file doesn't exist! */
  }
  while(GetIniLine(cIniName,cIniBuffer)!=EOF) {
    if(cIniBuffer[0]=='[') {	/* start of section */
      i=1;
      while(cIniBuffer[i]!=']' && cIniBuffer!=0) {
	cIniHeader[i-1]=cIniBuffer[i];
	i++;
      }
      cIniHeader[i-1]=0;
      if(strcmp(cIniHeader,cHeader)==0) {		/* this is the right section! */
	iFileStat=EOF+1;   /* Make this anything other than EOF! */
	while(iFileStat!=EOF) {
	  iFileStat=GetIniLine(cIniName,cIniBuffer);
	  if(cIniBuffer[0]=='[') return 1;
	  i=0;
	  while(cIniBuffer[i]!='=' && cIniBuffer[i]!=0) {
	    cIniLabel[i]=cIniBuffer[i];
	    i++;
	  }
	  cIniLabel[i++]=0;
	  if(strcmp(cIniLabel,cLabel)==0) {	/* this is the right label! */
	    j=0;
	    while(j<dValueLength && cIniBuffer[i]!=0) {
	      cValue[j++]=cIniBuffer[i++];
	    }
	    cValue[j]=0;
	    fclose(cIniName);
	    return 0;		/* value found! */
	  }
	}
      }
    }
  }
  fclose(cIniName);
  return 1;		/* section or label not found! */
}




int GetIniLine(FILE *cIniName,char *cIniBuffer)		/* read a line from the ini file */
     
{
  int i;
  
  for(i=0;i<BUFFER_SIZE-1;i++) {
    cIniBuffer[i]=getc(cIniName);
    switch(cIniBuffer[i]) {
      
    case EOF:
      cIniBuffer[i]=0;
      return EOF;
      
    case 10:
      cIniBuffer[i]=0;
      return 0;
    }
  }
  return 0;
}


void Prepend(char *sPathname,char *sFilename)
{
  char sTemp[256];

  if(sPathname[strlen(sPathname)-1]!='/' && sFilename[0]!='/') {
    strcat(sPathname,"/");
  }
  strcpy(sTemp,sPathname);
  strcat(sTemp,sFilename);
  strcpy(sFilename,sTemp);
}

  
int IncrementIndex(char *sPathname,int dMaxIndex)
{
  int dLockname=-1;
  FILE *hPathname;
  int i;
  char sLockname[256];
  char sAccum[256];
  int dIndex,dNewIndex;

  /* Lock the index */
  strcpy(sLockname,sPathname);
  strcat(sLockname,".LCK");
  i=0;
  while(dLockname<0 && i<MAX_RETRIES) {
    dLockname=open(sLockname,O_WRONLY|O_EXCL|O_CREAT,
		   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    i++;
  }
  if(dLockname<0) {
    return -1;
  }
  sprintf(sAccum,"%d",getpid());
  write(dLockname,sAccum,strlen(sAccum));
  close(dLockname);

  /* We've got the lock, so read the index */
  hPathname=fopen(sPathname,"r");
  if(hPathname==NULL) {
    unlink(sLockname);
    return -1;
  }
  if(fscanf(hPathname,"%d",&dIndex)!=1) {
    fclose(hPathname);
    unlink(sLockname);
    return -1;
  }
  fclose(hPathname);

  /* Update the index */
  if((dIndex<dMaxIndex) || (dMaxIndex==0)) {
    dNewIndex=dIndex+1;
  }
  else {
    dNewIndex=1;
  }

  /* Write it back */
  hPathname=fopen(sPathname,"w");
  if(hPathname==NULL) {
    unlink(sLockname);
    return -1;
  }
  fprintf(hPathname,"%d",dNewIndex);
  fclose(hPathname);

  /* Release the lock */
  unlink(sLockname);

  /* Ensure a sane value to return and then exit */
  if((dIndex>dMaxIndex)&&(dMaxIndex!=0)) {
    dIndex=1;
  }

  return dIndex;
}


/*
 * int StripLevel(char *sString)
 *
 * This strips the lower-most level from the pathname pointed to by 'sString'
 */

void StripLevel(char *sString)
{
  int i;                              /* General Purpose Pointer */
  int dString;                        /* Initial Length of 'sString' */

  dString=strlen(sString)-1;
  for(i=dString;i>=0;i--) {
    if(sString[i]=='/') {
      sString[i]=0;
      return;
    }
  }
  sString[0]=0;
}




int GetLock(char *sLockname)
{
  int dLockname=0,i=0;
  char sAccum[256];

  while((dLockname<=0) && (i<MAX_RETRIES)) {
    dLockname=open(sLockname,O_WRONLY|O_EXCL|O_CREAT,
		   S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH);
    if(dLockname<=0) {
      sleep(1);
    }
    i++;
  }
  if(dLockname<0) {
    return -1;
  }
  sprintf(sAccum,"%d",getpid());
  write(dLockname,sAccum,strlen(sAccum));
  close(dLockname);
  return 0;
}




void ClearLock(char *sLockname)
{
  unlink(sLockname);
}



speed_t GetTtySpeed(int dSpeed)
{
  switch(dSpeed) {
  case 50:
    return B50;
    break;
  case 75:
    return B75;
    break;
  case 110:
    return B110;
    break;
  case 134:
    return B134;
    break;
  case 150:
    return B150;
    break;
  case 200:
    return B200;
    break;
  case 300:
    return B300;
    break;
  case 600:
    return B600;
    break;
  case 1200:
    return B1200;
    break;
  case 1800:
    return B1800;
    break;
  case 2400:
    return B2400;
    break;
  case 4800:
    return B4800;
    break;
  case 9600:
    return B9600;
    break;
  case 19200:
    return B19200;
    break;
  case 38400:
    return B38400;
    break;
  case 57600:
    return B57600;
    break;
  case 115200:
    return B115200;
    break;
  case 230400:
    return B230400;
    break;
  default:
    return -1;
  }
  return -1;
}


int GetRealBaudRate(speed_t spSpeed)
{
  switch(spSpeed) {
  case B0:
    return 0;
  case B50:
    return 50;
  case B75:
    return 75;
  case B110:
    return 110;
  case B134:
    return 134;
  case B150:
    return 150;
  case B200:
    return 200;
  case B300:
    return 300;
  case B600:
    return 600;
  case B1200:
    return 1200;
  case B1800:
    return 1800;
  case B2400:
    return 2400;
  case B4800:
    return 4800;
  case B9600:
    return 9600;
  case B19200:
    return 19200;
  case B38400:
    return 38400;
  case B57600:
    return 57600;
  case B115200:
    return 115200;
  case B230400:
    return 230400;
  default:
    return -1;
  }
  return -1;
}



int IsOdd(int dNumber)
{
  if((2*(dNumber/2))==dNumber) {
    return 1;
  }
  return 0;
}

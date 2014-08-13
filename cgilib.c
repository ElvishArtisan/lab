/* cgilib.c
 *
 * A small library for interfacing with HTML forms via the Common Gateway
 * Interface (CGI) Standard 
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
#include <string.h>
#include "cgilib.h"


/* ReadPost(char *cBuffer,int dSize) */

/* This function reads POST data (such as that submitted by an HTML form) into
the buffer pointed to by cBuffer.  The size of the buffer is indicated by 
dSize.

RETURNS: Number of bytes read if the function is successful
         -1 if an error is encountered. */

int ReadPost(char *cBuffer,int dSize)

{
  int dPostSize=0;

  if(strcasecmp(getenv("REQUEST_METHOD"),"POST")!=0) {   /* No post data to receive! */
    return -1;
  }
  sscanf(getenv("CONTENT_LENGTH"),"%d",&dPostSize);
  if(dPostSize>=dSize) {  /* Data block too large! */
    return -1;
  }
  dPostSize++;
  fgets(cBuffer,dPostSize,stdin);
  return dPostSize;  
}



/*
 * int PutPostString(char *sPost,char *sArg,char *sValue,int dMaxSize)
 *
 * This function changes the contents of the POST buffer pointed to by
 * 'sPost'.  If the entry pointed to by 'sArg' exists, it's value is
 * replaced by the string pointed to by 'sValue'.  If the entry doesn't
 * exist, it is created.  'dMaxSize' is the maximum allowable size of 'sPost'.
 *
 * RETURNS:  If successful, a pointer to the start of the updated value
 *           If unsuccessful, -1
 */
int PutPostString(char *sPost,char *sArg,char *sValue,int dMaxSize)
{
  int dOrigin;         /* Start of insert point */
  int dValue;          /* Length of sValue */
  int i;               /* General purpose counter */
  char sAccum[CGI_ACCUM_SIZE];

  /*
   * Does the argument already exist?
   */
  dOrigin=FindPostString(sPost,sArg,sAccum,CGI_ACCUM_SIZE);
  if(dOrigin<0) {
    /* 
     * Create a new entry
     * Will it fit?
     */
    dOrigin=strlen(sPost);
    if((dOrigin+strlen(sArg)+strlen(sValue)+2)>=dMaxSize) {
      return -1;
    }
    /*
     * Append to the end
     */
    strcat(sPost,"&");
    strcat(sPost,sArg);
    strcat(sPost,"=");
    dOrigin=strlen(sPost);
    strcat(sPost,sValue);
  }
  else {
    /*
     * The argument exists, so update it
     */
    dValue=strlen(sValue);
    if(BufferDiff(sPost,dOrigin,dValue-strlen(sAccum),dMaxSize)<0) {
      return -1;
    }
    for(i=0;i<dValue;i++) {
      sPost[dOrigin+i]=sValue[i];
    }
    sPost[dOrigin+dValue]='&';
  }
  return dOrigin;
} 


/*
 * int FindPostString(char *cBuffer,char *sSearch,char *sReturn,
 * int dReturnSize)
 *
 * This function returns the argument value associated with field name 
 * pointed to by sSearch in the POST buffer cBuffer.  The argument value 
 * is returned in the buffer pointed to by sReturn, of maximum size 
 * dReturnSize.
 *
 * RETURNS:  Pointer to the start of the value, if successful
 *          -1 if the search is unsuccessful
 */

int FindPostString(char *cBuffer,char *sSearch,char *sReturn,int dReturnSize)

{
  int i=0,j=0;
  int dMatch,dOrigin;

  while(cBuffer[i]!=0) {
    j=0;
    dMatch=0;
    while(cBuffer[i]!='=' && cBuffer[i]!=0) {
      if(cBuffer[i++]!=sSearch[j++]) dMatch=1;
    }
    if(dMatch==0 && cBuffer[i]=='=' && sSearch[j]==0) {   /* Found it! */
      j=0;
      i++;
      dOrigin=i;
      while(cBuffer[i]!='&' && cBuffer[i]!=0 && j<dReturnSize-1) {
	sReturn[j++]=cBuffer[i++];
      }
      sReturn[j]=0;
      return dOrigin;
    }
    else {
      while(cBuffer[i]!='&' && cBuffer[i]!=0) i++;
    }
    if(cBuffer[i]==0) {
      sReturn[0]=0;
      return -1;   /* No match found! */
    }
    else {  /* advance to next field */
      i++;
    }
  }
  sReturn[0]=0;
  return -1;
}




/*
 * int GetPostString(char *cBuffer,char *sSearch,char *sReturn,
 * int dReturnSize)
 *
 * This function returns the argument value associated with field name 
 * pointed to by sSearch in the POST buffer cBuffer.  The argument value 
 * is returned in the buffer pointed to by sReturn, of maximum size 
 * dReturnSize.  The argument value is also processed to convert any
 * CGI escape sequences back into normal characters.
 *
 * RETURNS:  0 if successful
 *          -1 if the search is unsuccessful
 */

int GetPostString(char *cBuffer,char *sSearch,char *sReturn,int dReturnSize)
{
  if(FindPostString(cBuffer,sSearch,sReturn,dReturnSize)<0) {
    return -1;
  }
  DecodeString(sReturn);
  return 0;
}



/*
 * int GetPostInt(char *cBuffer,char *sSearch,int *dReturn)
 *
 * This function returns the integer argument value associated with field name 
 * pointed to by sSearch in the POST buffer cBuffer.  The argument value 
 * is returned in the integer variable pointed to by dReturn.
 *
 * RETURNS:  0 if successful
 *          -1 if the search is unsuccessful
 */

int GetPostInt(char *cBuffer,char *sSearch,int *dReturn)
{
  char sAccum[256];

  if(GetPostString(cBuffer,sSearch,sAccum,255)<0) {
    return -1;
  }
  if(sscanf(sAccum,"%d",dReturn)!=1) {
    return -1;
  }
  return 0;
}




/*
 * int PurgePostString(char *sPost,char *sArg)
 *
 * This function removes the argument/value pair pointed to by 'sArg'.
 *
 * RETURNS:  If successful, the new size of 'sPost'
 *           If unsuccessful, -1
 */ 
int PurgePostString(char *sPost,char *sArg,int dMaxSize)
{
  char sAccum[CGI_ACCUM_SIZE];
  int dPointer;

  dPointer=FindPostString(sPost,sArg,sAccum,CGI_ACCUM_SIZE);
  if(dPointer<0) {
    return -1;
  }
  dPointer-=(strlen(sArg)+1);
  BufferDiff(sPost,dPointer,-(strlen(sArg)+strlen(sAccum)+2),dMaxSize);
  return strlen(sPost);
}



/*
 * int EncodeString(char *sString,int dMaxSize)
 *
 * This function processes the string pointed to by 'sString', replacing
 * any spaces with + and escaping most punctuation characters in accordance
 * with the Common Gateway Interface (CGI) standard
 *
 * RETURNS: If successful, the new size of 'sString'
 *          If unsuccessful, -1
 */
int EncodeString(char *sString,int dMaxSize)
{
  int i;                  /* General Purpose Counter */
  char sAccum[4];          /* General String Buffer */

  i=0;
  while(sString[i]!=0) {
    if(((sString[i]!=' ') && (sString[i]!='*') && (sString[i]!='-') &&
	(sString[i]!='_') && (sString[i]!='.')) && 
       ((sString[i]<'0') ||
       ((sString[i]>'9') && (sString[i]<'A')) ||
       ((sString[i]>'Z') && (sString[i]<'a')) ||
       (sString[i]>'z'))) {
      if(BufferDiff(sString,i,2,dMaxSize)<0) {
	return -1;
      }
      sprintf(sAccum,"%%%2x",sString[i]);
      sString[i++]=sAccum[0];
      sString[i++]=sAccum[1];
      sString[i]=sAccum[2];
    }
    if(sString[i]==' ') {
      sString[i]='+';
    }
    i++;
  }
  return strlen(sString);
}


/*
 * int EncodeSQLString(char *sString,int dMaxSize)
 *
 * This function processes the string pointed to by 'sString', 
 * escaping the ' \ and " characters.
 *
 * RETURNS: If successful, the new size of 'sString'
 *          If unsuccessful, -1
 */
int EncodeSQLString(char *sString,int dMaxSize)
{
  int i;                  /* General Purpose Counter */
  char sAccum[4];          /* General String Buffer */

  i=0;
  while(sString[i]!=0) {
    if((sString[i]=='%')||(sString[i]==34)||(sString[i]==39)) {
      if(BufferDiff(sString,i,2,dMaxSize)<0) {
	return -1;
      }
      sprintf(sAccum,"%%%2x",sString[i]);
      sString[i++]=sAccum[0];
      sString[i++]=sAccum[1];
      sString[i]=sAccum[2];
    }
    i++;
  }
  return strlen(sString);
}




int DecodeString(char *sString)

{
  int i=0,j=0,k;
  char sAccum[4];

  while(sString[i]!=0) {
    switch(sString[i]) {
      
      case '+':
      sString[j]=' ';
      break;

      case '%':   /* escape sequence */
      sAccum[0]=sString[++i];
      sAccum[1]=sString[++i];
      sAccum[2]=0;
      sscanf(sAccum,"%x",&k);
      sString[j]=(char)k;
      break;

      default:
      sString[j]=sString[i];
      break;
    }
    i++;
    j++;
  }
  sString[j]=0;
  return --j;
}



/*
 * PutPlaintext(char *sPost,int dMaxSize)
 *
 * This function appends a block of text consisting of the *decoded* values
 * of all the POST values found in the buffer pointed to by 'sPost' into
 * the buffer pointed to by 'sPost'.  The block is enclosed by the HTML
 * start and end comment sequence (<! ... -->).  'sPost' is of maximum size
 * 'dMaxSize'.
 *
 * RETURNS: If successful, the new size of 'sPost'.
 *          If unsuccessful, -1.  
 */
int PutPlaintext(char *sPost,int dMaxSize)
{
  int dOriginalsize=0,dPostsize=0;   /* Current post buffer length */
  int i,j=0;                           /* General purpose counter */
  int iState=0;                      /* State Counter */
  char sAccum[CGI_ACCUM_SIZE];       /* General String Buffer */
  int dAccum;                        /* Length of sAccum */

  /*
   * Initialize some data structures
   */
  dOriginalsize=strlen(sPost);
  dPostsize=dOriginalsize;

  /*
   * Append the start of comment sequence
   */
  if((dPostsize+3)>=dMaxSize) {
    return -1;
  }
  strcat(sPost,"&< ");
  dPostsize+=3;

  /* 
   * Scan for value strings
   */
  for(i=0;i<dOriginalsize+1;i++) {
    switch(iState) {

    case 0:    /* Looking for a start of value or comment */
      switch(sPost[i]) {

      case '=':   /* Start of value */
	j=0;
	sAccum[j]=0;
	iState=1;
	break;

      case '<':   /* Start of comment sequence */
	iState=10;
	break;
      }
      break;

    case 1:
      switch(sPost[i]) {

      case '&':   /* End of value string */
	sAccum[j++]=' ';
	sAccum[j]=0;
	DecodeString(sAccum);
	dAccum=strlen(sAccum);
	if(dAccum>=dMaxSize) {
	  return -1;
	}
	strcat(sPost,sAccum);
	dPostsize+=dAccum;
	iState=0;
	break;

      default:    /* Another character in value string */
	if((sPost[i]!='<') && (sPost[i]!='>')) {
	  sAccum[j++]=sPost[i];
	}
	break;
      }
    case 10:    /* Middle of a comment */
      switch(sPost[i]) {
      case '>':   /* End of comment */
	iState=0;
	break;
      }
      break;
    default:    /* Parser error! */
      return -1;
      break;
    }
  }

  /*
   * Append the end of comment sequence
   */
  if((dPostsize+1)>=dMaxSize) {
    return -1;
  }
  strcat(sPost,">");
  dPostsize+=1;

  return dPostsize;
}




/*
 * int PurgePlaintext(char *sPost,int dMaxSize)
 *
 * This function removes one or more plaintext blocks enclosed by HTML comment
 * sequences (<! ... -->) from the buffer pointed to by 'sPost', of
 * maximum size 'dMaxSize'.  
 *
 * RETURNS: If successful, the new size of 'sPost'. 
 *          If unsuccessful, -1
 */
int PurgePlaintext(char *sPost,int dMaxSize)
{
  int i=0;                    /* General Purpose Counters */
  int dComments=0;            /* Comment State Switch */
  int dStart=0;               /* Comment Startpoint Pointer */

  /*
   * Scan for comment sequences
   */
  while(sPost[i]!=0) {
    if((sPost[i]=='<') && (dComments==0)) {   /* Start of comment */
      dStart=i;
      dComments=1;
    }
    if((sPost[i]=='>') && (dComments==1)) {   /* End of comment */
      if(BufferDiff(sPost,dStart,dStart-i-1,dMaxSize)<0) {
	return -1;
      }
      if(sPost[i]==0) {   /* Ensure a proper exit if at end of string */
	i--;
      }
    }
    i++;
  }

  /*
   * Clean up and exit nicely
   */
  PruneAmp(sPost);
  return strlen(sPost);
}




void Error(char *sError)
{
  /* The cgi header */
  printf("Content-type: text/html\n");
  printf("\n");

  /* The html header */
  printf("<html>\n");
  printf("<head>\n");
  printf("<title>");
  printf("CGI Internal Error");
  printf("</title>\n");
  printf("</head>\n");

  /* The body of the message */
  printf("<h1>Oops!</h1><br>\n");
  printf("We seem to have encountered a problem!  The system says: <br>\n");
  printf("<pre>%s</pre><br>\n",sError);

  /* The html footer */
  printf("</body>\n");
  exit(1);
}



/*
 * int BufferDiff(char sString,int dOrigin,int dDiff,int dMaxSize)
 *
 * This function adds (+ value) or deletes (- value) 'dDiff' characters 
 * from the string pointed to by 'sString' at the offset location pointed 
 * to by 'dOrigin'.  'dMaxSize' is the maximum allowable size of 'sString'.
 *
 * RETURNS:  If successful, the new size of 'sString'
 *           If unsuccessful, -1
 */
int BufferDiff(char *sString,int dOrigin,int dDiff,int dMaxSize)
{
  int dOldSize,dNewSize;
  int i;

  /*
   * Will it fit?
   */
  dOldSize=strlen(sString);
  if((dOldSize+dDiff)>=dMaxSize) {
    return -1;
  }
  dNewSize=dOldSize+dDiff;

  /* 
   * Adding characters 
   */
  if(dDiff>0) {
    for(i=dOldSize;i>dOrigin;i--) {
      sString[i+dDiff]=sString[i];
    }
    return dNewSize;
  }

  /* 
   * No Change
   */
  if(dDiff==0) {
    return dNewSize;
  }

  /*
   * Deleting Characters
   */
  if(dDiff<0) {
    for(i=dOrigin;i<dOldSize;i++) {
      sString[i]=sString[i-dDiff];
    }
    return dNewSize;
  }
  return -1; 
}




void PruneAmp(char *sPost)
{
  if(sPost[strlen(sPost)-1]=='&') {
    sPost[strlen(sPost)-1]=0;
  }
}


int EscapeQuotes(const char *src,char *dest,int maxlen)
{
  int i=0;
  int j=0;
  while(src[i]!=0) {
    if(src[i]==34) {  // Double Quotes
      if((j+7)>maxlen) {
	dest[j]=0;
	return j;
      }
      dest[j]=0;
      strcat(dest,"&quot;");
      i++;
      j+=6;
    }      
    else {
      if((j+2)>maxlen) {
	dest[j]=0;
	return j;
      }
      dest[j++]=src[i++];
    }
  }
  dest[j]=0;
  return j;
}

/* cgilib.h
 *
 * A small library for interfacing with HTML forms via the 
 * Common Gateway Interface (CGI) Standard 
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

/*
 * Data Structure Sizes
 */
#define CGI_ACCUM_SIZE 1024

/* Function Prototypes */
extern int ReadPost(char *,int);
extern int PutPostString(char *,char *,char *,int);
extern int FindPostString(char *,char *,char *,int);
extern int GetPostString(char *,char *,char *,int);
extern int GetPostInt(char *,char *,int *);
extern int PurgePostString(char *,char *,int);
extern int EncodeString(char *,int);
extern int EncodeSQLString(char *,int);
extern int DecodeString(char *);
extern int PutPlaintext(char *,int);
extern int PurgePlaintext(char *,int);
extern void Error(char *);
extern int BufferDiff(char *,int,int,int);
extern void PruneAmp(char *);
extern int EscapeQuotes(const char *src,char *dest,int maxlen);

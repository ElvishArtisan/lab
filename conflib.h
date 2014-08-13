/* conflib.h
 *
 * A small library for reading configuration files.
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

#include <termios.h>
#define MAX_RETRIES 10

/* Function Prototypes */
int GetPrivateProfileString(char *,char *,char *,char *,char *,int);
int GetPrivateProfileInt(char *,char *,char *,int);
int GetIni(char *,char *,char *,char *,int);
int GetIniLine(FILE *,char *);
void Prepend(char *,char *);
int IncrementIndex(char *,int);
void StripLevel(char *); 
int GetLock(char *);
void ClearLock(char *);
speed_t GetTtySpeed(int);
int GetRealBaudRate(speed_t);
int IsOdd(int);

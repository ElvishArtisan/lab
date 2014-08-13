/* globals.c
 * 
 * Global variables for labd
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

#include <sys/time.h>
#include <setjmp.h>
#include <netinet/in.h>
#include "labd.h"

/*
 * Debug Flags 
 */
int dDebug=0;

char sAudioroot[256];
uid_t dOwner=0;
gid_t dGroup=0;
int hAudioDevice[4]={0,0,0,0};
int hSwitcherDevice[4]={0,0,0,0};
jmp_buf jmpEnv;

/*
 * Configuration Values
 */
char sPidname[255];
char sScoreBoard[255];
char sAudioDevice[4][256];
int dAudioDevice[4]={0,0,0,0};
char sSwitcherDevice[4][256];
int dSwitcherType[4]={0,0,0,0};
int dSwitcherDevice[4]={0,0,0,0};
struct sockaddr_in inSwitcherAddress[4];
int dSwitcherBaud[4]={9600,9600,9600,9600};
char sTrafficname[256];
char sHelpname[256];
char sGpiDevice[256];
int hGpiDevice=-1;
char sGpoDevice[256];
int hGpoDevice=-1;
int dInvertGpi=0;
#ifdef HAVE_GPIO
struct gpio_info gpi_info;
struct gpio_info gpo_info;
#endif  // HAVE_GPIO
int dUseRealtime=0;

/* 
 * Alarm Switches 
 */
int dConsoleAlarm;
int dMailAlarm;
char sMailTo[4][256];
int dPageAlarm;
char sPagePort[256];
char sPageTo[4][256];
int dExternalAlarm;

/* 
 * The event code table 
 */
char sEventType[EVENT_QUAN][3] = {"TR",  /* Timed Record */
				  "TP",  /* Timed Playback */
				  "TS",  /* Timed Switcher */
				  "CR",  /* Closure Record */
				  "CP",  /* Closure Playback */
				  "CS",  /* Closure Switcher */
				  "SR",  /* Threshold Record */
				  "RP",  /* Rotate Playback */
				  "PR"}; /* Pulse Relay */

/* 
 * The event status table
 */
char sDeckStatus[8][39] = {"Unavailable                           ",
			   "Idle                                  ",
			   "Recording                             ",
			   "Playing                               ", 
			   "Waiting for Closure to Start Recording",
			   "Waiting for Closure to Start Playback ",
			   "Waiting for Closure to Switch Channel ",
			   "Waiting for Audio to Start Recording  "};
                       
/* 
 * The Deck Task Structures
 */
unsigned dDeckEvent[MAX_DECKS]={DECK_UNAVAILABLE,   /* Current event */
				DECK_UNAVAILABLE,
				DECK_UNAVAILABLE,
				DECK_UNAVAILABLE};
unsigned dDeckRecord[MAX_DECKS];  /* Current traffic record */
time_t tmDeckStart[MAX_DECKS];    /* Time current event started */
long pPid[MAX_DECKS]={0,0,0,0};  /* Process ID of child process */

/* 
 * The Traffic File
 */
struct scdTraffic scdTraffic[MAX_EVENTS];
int dTrafficRecords;                /* TOTAL TRAFFIC RECORDS */

/* 
 * The rotator table
 */
char sRotators[MAX_ROTATOR_DIRS][MAX_ROTATOR_FILES][255];
int dRotatorQuan[MAX_ROTATOR_DIRS]; /* NUMBER OF ROTATOR FILES */
int dRotatorRecords;
char sRandomState[STATE_SIZE];
char sNextRotator[MAX_ROTATOR_DIRS][256]; /* Next rotator audio */

/*
 * Execution Control
 */
volatile int dRestart=0;

/*
 * The switcher source tables
 */
struct ssSource ssSource[MAX_DECKS][MAX_SOURCES];
int dSourceQuan[MAX_DECKS];
int dCmdTerm[MAX_DECKS];    /* 0=none, 1=CR, 2=CR/LF 3=CR */
int dCmdSize[MAX_DECKS];

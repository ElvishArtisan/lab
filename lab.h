/* lab.h
 *
 * The header file for The Linux Audio Backstop
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
 *
 */

#ifndef _TIME_H
#include <sys/time.h>
#endif

/*
 * Maximum Number of Palettes
 */
#define MAX_PALETTES 25

/*
 * File paths
 */
#define INI_PATH "/etc/lab.conf"

/*
 * The default status line text
 */
#define STATUS_MESSAGE "The Linux Audio Backstop"

/*
 * The Input Scan Interval (microseconds)
 */
#define SCAN_INTERVAL 1000000

/*
 * The Archive Scan Interval (seconds)
 */
#define ARCHIVE_SCAN 10

/*
 * The length of time to wait for a 'y' to confirm an exit (1/10th sec)
 */
#define EXIT_WAIT_TIME 100

/*
 * The margin size for ncurses buttons
 */
#define CURS_MARGIN 13

/*
 * Standard Button Color Schemes
 */
#define LAB_BLUE 1
#define LAB_GREEN 2
#define LAB_RED 3
#define LAB_WHITE 4
#define LAB_YELLOW 5
#define LAB_BLACK 6

/*
 * Button Status States
 */
#define LAB_STATUS_READY 0
#define LAB_STATUS_PLAYING 1
#define LAB_STATUS_PAUSED 2
#define LAB_STATUS_EXPIRED 3
#define LAB_STATUS_MISSING 4
#define LAB_STATUS_UPDATING 5
#define LAB_STATUS_DISABLED 6

/*
 * Data Structures
 */
#define STRING_SIZE 80
#define PATH_LEN 256
struct buttontext {
  char line0[STRING_SIZE];
  char line1[STRING_SIZE];
  char line2[STRING_SIZE];
  char line3[STRING_SIZE];
  char sPath[PATH_LEN];
  unsigned dShelflife;
  unsigned dEndwrap; 
  int dStatus;
};

/*
 * Global Variables
 */
extern struct buttontext btButtons[MAX_PALETTES][25];

/*
 * Function Prototypes
 */
int LoadIni(char *);
int CaseDown(int);
void Finish(int);
void ClearDeck(int);
void ScanArchive(void);
void StampLength(int);
void BreakDownInterval(int,char *);
time_t GetWavLength(int);
void TickClock(void);
extern int dPalette;

/*
 * Prototypes for the screen interface
 */
void StartScreen(int);
void SetColor(int,int);
void SetText(int,struct buttontext *);
void WriteClock(char *);
int GetKey(void);
int CloseScreen(void);






/* curs_screen.c
 *
 * This is the NCURSES-based screen interface for The Linux Audio Backstop
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

#include <string.h>
#include <curses.h>
#include <lab.h>

/*
 * Global Variables
 */
WINDOW *wButtons[40];
WINDOW *wStatus;
WINDOW *wClock;
int i,j,k;
short dColorTable[7];

void StartScreen(int dColor)
{
  /*
   * Setup some basic behavior
   */
  initscr();
  nonl();
  raw();
  noecho();

  /*
   * Colors
   */
  if(has_colors()) {
    start_color();
    init_pair(COLOR_BLUE,COLOR_WHITE,COLOR_BLUE);
    init_pair(COLOR_RED,COLOR_WHITE,COLOR_RED);
    init_pair(COLOR_WHITE,COLOR_RED,COLOR_WHITE);
    init_pair(COLOR_GREEN,COLOR_BLACK,COLOR_GREEN);
    init_pair(COLOR_YELLOW,COLOR_WHITE,COLOR_YELLOW);
    init_pair(COLOR_BLACK,COLOR_BLACK,COLOR_BLACK);
    dColorTable[LAB_BLUE]=COLOR_PAIR(COLOR_BLUE);
    dColorTable[LAB_RED]=COLOR_PAIR(COLOR_RED);
    dColorTable[LAB_WHITE]=COLOR_PAIR(COLOR_WHITE);
    dColorTable[LAB_GREEN]=COLOR_PAIR(COLOR_GREEN);
    dColorTable[LAB_YELLOW]=COLOR_PAIR(COLOR_YELLOW);
    dColorTable[LAB_BLACK]=COLOR_PAIR(COLOR_BLACK);
  }

  /*
   * Create the status line,page and clock areas
   */
  wStatus=newwin(1,64,24,0);
  leaveok(wStatus,TRUE);
  keypad(wStatus,TRUE);
  wmove(wStatus,0,1);
  wprintw(wStatus,STATUS_MESSAGE);
  wrefresh(wStatus);
  wClock=newwin(1,15,24,65);
  leaveok(wClock,TRUE);
  wButtons[23]=newwin(4,31,20,48);
  leaveok(wButtons[23],TRUE);
  wbkgdset(wButtons[23],dColorTable[LAB_BLUE]);
  wbkgd(wButtons[23],dColorTable[LAB_BLUE]);
  wrefresh(wButtons[23]);


  /*
   * Create the button fields
   */
  k=0;
  for(j=0;j<24;j+=5) {
    for(i=0;i<80;i+=16) {
      if(!((j>19)&&(i>47))) {
	wButtons[k]=newwin(4,15,j,i);
	leaveok(wButtons[k],TRUE);
	wbkgdset(wButtons[k],dColorTable[dColor]);
	wbkgd(wButtons[k],dColorTable[dColor]);
	SetText(k,&btButtons[dPalette][k]);
	k++;
      }
    }
  }
}


void SetColor(int dButton,int dColor)
{
  wbkgdset(wButtons[dButton],dColorTable[dColor]);
  wbkgd(wButtons[dButton],dColorTable[dColor]);
  wrefresh(wButtons[dButton]);
}


void SetText(int dButton,struct buttontext *btText)
{
  char sAccum[STRING_SIZE];
  int dMargin=CURS_MARGIN;

  if(dButton==23) {   /* Page Button */
    dMargin=CURS_MARGIN*2+3;
  }
  werase(wButtons[dButton]);
  strcpy(sAccum,btText->line0);
  if(strlen(sAccum)>dMargin) {
    sAccum[dMargin]=0;
  }
  wmove(wButtons[dButton],0,1);
  wprintw(wButtons[dButton],"%s",sAccum);
  strcpy(sAccum,btText->line1);
  if(strlen(sAccum)>dMargin) {
    sAccum[dMargin]=0;
  }
  wmove(wButtons[dButton],1,1);
  wprintw(wButtons[dButton],"%s",sAccum);
  strcpy(sAccum,btText->line2);
  if(strlen(sAccum)>dMargin) {
    sAccum[dMargin]=0;
  }
  wmove(wButtons[dButton],2,1);
  wprintw(wButtons[dButton],"%s",sAccum);
  strcpy(sAccum,btText->line3);
  if(strlen(sAccum)>dMargin) {
    sAccum[dMargin]=0;
  }
  wmove(wButtons[dButton],3,1);
  wprintw(wButtons[dButton],"%s",sAccum);
  wrefresh(wButtons[dButton]);
}



void WriteClock(char *sString)
{
  wmove(wClock,0,2);
  wprintw(wClock,"%s",sString);
  wrefresh(wClock);

  return;
}


int GetKey(void)
{
  int c;

  c=wgetch(wStatus);
  if((c>='A')&&(c<='Z')) {
    c+=32;
  }
  return c;
}



int CloseScreen()
{
  int c=0;

  wclear(wStatus);
  wmove(wStatus,0,1);
  wprintw(wStatus,"Exit? (y/n)");
  wrefresh(wStatus);
  
  c=wgetch(wStatus)&0xFF;
  if((c=='y')||(c=='Y')) {
    clear();
    refresh();
    endwin();
    return 0;
  }
  wclear(wStatus);
  wmove(wStatus,0,1);
  wprintw(wStatus,STATUS_MESSAGE);
  wrefresh(wStatus);
  return -1;
}

/* labd.h
 *
 * The netcatcher component of the Linux Audio Backstop.
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

#include <setjmp.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#ifdef HAVE_GPIO
#include <linux/gpio.h>
#endif  // HAVE_GPIO
#include <netinet/in.h>

#define LAMDUSAGE "labd [-v][-d]"
#define CONF_LOCATION "/etc/lab.conf"
#define BUFFER_SIZE 1024
#define PATH_LEN 256
#define MAX_EVENTS 256
#define MAX_ROTATOR_DIRS 10
#define MAX_ROTATOR_FILES 256
#define STATE_SIZE 64
#define SLEEP_TIME 50000
#define SEEK_LIMIT 3600
#define PRIORITY 0
#define MAX_DECKS 4
#define MAX_SOURCES 255
#define POST_SIZE 5000
#define PID_FILE "/var/run/labd.pid"
#define SCOREBOARD_FILE "/var/run/labd.score"
#define RELAY_PULSE_TIME 333000
#define GPI_SCAN_INTERVAL 50000
#define MAX_GPIOS 24

/*
 * Web Catcher Colors
 */
#define WEB_BACK_COLOR "#FFFFFF"
#define WEB_BAR_COLOR "#80FFFF"
#define WEB_GROUP_COLOR "#80FFFF"
#define WEB_WARNING_COLOR "#FF0000"

/*
 * Time interval before timed event start to wake up (secs)
 */
#define SLEEP_GAP 3

/*
 * The wildcard numeric code
 */
#define WILDCARD -100000

/* 
 * Devices 
 *
 * Mixer Devices
 */
#define MIXERDEV "/dev/mixer"

/* Joystick 1 */
#define STICK0 "/dev/js0"
#define STICK1 "/dev/js1"

/*
 * Data Structures
 */
struct scdTraffic {
  int dOpenMonth;        /* Month */
  int dOpenDay;          /* Day */
  int dOpenYear;         /* Year */
  int dDayOfWeek;        /* Day-of-Week */
  int dOpenHour;         /* Open-Hour */
  int dOpenMinute;       /* Open-Minute */
  int dOpenSecond;       /* Open-Second */
  int dCloseHour;        /* Close-Hour */
  int dCloseMinute;      /* Close-Minute */
  int dCloseSecond;      /* Close-Second */
  unsigned dCutLength;        /* Cut Length (secs) */
  unsigned dEventType;        /* Event Type Code */
  char sFilename[255];        /* Filename */
  unsigned dDevice;           /* Device Number */
  unsigned dSampleRate;       /* Sample Rate (samples/sec) */
  unsigned dSampleSize;       /* Sample Size (bits/sample) */
  unsigned dChannels;         /* Number of channels */
  unsigned dPin;              /* Pin Number */
  unsigned dSilence;          /* Silence-sense wait-time (secs) */
  unsigned dThreshold;        /* Silence-sense threshold */
  char sCutName[255];         /* Comment Field */
  int dRotatorRecord;         /* Pointer to rotator file list */
  int gpioMap[MAX_GPIOS];     /* Redirect for relays */
  char sCommand[255];         /* Post record command */
  int dDeleteSource;          /* Delete Source WAV file */
  char sResolvedName[1024];   /* Resolved Filename */
  char sResolvedCmd[1024];    /* Resolved Post Command */
};

struct ssSource {            /* Audio Switcher Sources */
  char sName[255];
  char sCommand[255];
};

struct gpiTrans {
  struct timeval tv;
#ifdef HAVE_GPIO
  struct gpio_mask mask;
#endif  // HAVE_GPIO
};

/* 
 * Function Prototypes
 *
 * In labd.c 
 */
extern void Detach();
extern void PlayCut(char *,char *,int,int);
extern int IsComment(char *);
extern void SigHandler(int);
extern void KillAudio(void);
extern void Reinit(void);
extern int ScanRotators();
extern void GetRandomFile(int,char *);
extern void MakeMark(char *);
extern void DumpDebug(void);
extern int OpenDevs(void);
extern void CloseDevs(void);
extern int MatchTime(int,struct tm *);
extern int GetSwitchCommand(int,char *,char *);
extern int WriteScoreBoard(char *,int);
extern void *PulseRelay(void *);
extern void *StartRecordEvent(void *);
extern void *StartRecord(void *);
extern void WriteRelays(struct scdTraffic *,struct gpiTrans *,int);
extern void *StartPlayEvent(void *);
extern void *StartPlay(void *);
extern void *PulseRelayNumber(void *);
extern void SendSwitchCommand(int dDevice,char *sCommand);
extern void PostCommand(struct scdTraffic *);
#ifdef HAVE_GPIO
extern void InvertMask(struct gpio_mask *);
#endif  // HAVE_GPIO
extern int dGpoLines;

/* 
 * In traffic.c
 */
extern int LoadTraffic(char *,int);
extern int WriteTraffic(char *,int);
extern int GetTypeCode(unsigned,char *);
extern void TrafficError(char *,int);
/* 
 * In error.c
 */
extern void EventError(int,int);
extern void ToConsole(char *);
/*
 * In common.c
 */
extern int LoadConfig(char *);
extern void DisplayAddress(const char *label,struct sockaddr_in *sockaddr_in);
extern void ResolveMacros(time_t time,const char *filename,
			  const char *template,char *output,int maxsize);
/*
 * In wcatch.c
 */
extern void ServeTop(void);
extern void ServeBasePage(char *);
extern void ServeHeaderTable(char *);
extern void ServeStatusTable(void);
extern void ServeDeckTable(void);
extern void ServeTrafficTable(void);
extern void ServeSpacerTable(void);
extern void ServeChangePage(int);
extern void ServeDeletePage(int);
extern void ServeAddPage(int);
extern void ServeSecondAddPage(int);
extern void ServeTrafficRecord(int,int,unsigned);
extern pid_t GetPid(char *);
extern int ReadScoreBoard(char *,int);
extern void PrintDeckStatus(int);
extern void PrintEventType(int);
extern int LoadSwitchCommands(int);
extern int CommitChange(int);
extern int CommitDelete(int);
extern int CommitAdd();
extern unsigned ReadTrafficPost(struct scdTraffic *);
extern unsigned CheckTrafficRecord(struct scdTraffic *);
extern int GetGpioSize(int);
extern double GetFlatTime(struct timeval *);

/* 
 * Global Variables
 *
 * Debug Flags
 */
extern int dDebug;
extern char sAudioroot[256];

extern int hAudioDevice[4];
extern int hSwitcherDevice[4];
extern jmp_buf jmpEnv;

/* 
 * Alarm Switches
 */
extern int dConsoleAlarm;
extern int dMailAlarm;
extern char sMailTo[4][256];
extern int dPageAlarm;
extern char sPagePort[256];
extern char sPageTo[4][256];
extern int dExternalAlarm;

/*
 * Configuration Values
 */
extern char sAudioDevice[4][256];
extern int dAudioDevice[4];
extern uid_t dOwner;
extern gid_t dGroup;
extern int dSwitcherType[4];
extern char sSwitcherDevice[4][256];
extern struct sockaddr_in inSwitcherAddress[4];
extern int dSwitcherDevice[4];
extern int dSwitcherBaud[4];
extern char sTrafficname[256];
extern char sHelpname[256];
extern char sGpiDevice[256];
extern int hGpiDevice;
extern char sGpoDevice[256];
extern int hGpoDevice;
extern int dInvertGpi;
extern struct gpio_info gpi_info;
extern struct gpio_info gpo_info;
extern int dUseRealtime;

/* 
 * The event code table
 */
#define EVENT_QUAN 9
extern char sEventType[EVENT_QUAN][3];

/* 
 * Event Codes
 */
#define EVENT_TIMED_RECORD 0
#define EVENT_TIMED_PLAYBACK 1
#define EVENT_TIMED_SWITCHER 2
#define EVENT_CLOSURE_RECORD 3
#define EVENT_CLOSURE_PLAYBACK 4
#define EVENT_CLOSURE_SWITCHER 5
#define EVENT_THRESHOLD_RECORD 6
#define EVENT_ROTATE_PLAYBACK 7
#define EVENT_RELAY_PULSE 8
#define EVENT_LASTEVENT 9

/*
 * WebCatcher CGI Command Codes
 */
#define WCOMMAND_NONE 0
#define WCOMMAND_RESTART 1
#define WCOMMAND_STOP 2
#define WCOMMAND_START 3
#define WCOMMAND_CHANGE 4
#define WCOMMAND_COMMIT_CHANGE 5
#define WCOMMAND_DELETE 6
#define WCOMMAND_COMMIT_DELETE 7
#define WCOMMAND_ADDNEW 8
#define WCOMMAND_SECONDADDNEW 9
#define WCOMMAND_COMMIT_ADDNEW 10
#define WCOMMAND_HELP 11

/*
 * Traffic Record Error Flags
 */
#define WERROR_OPENMONTH 0x00000001
#define WERROR_OPENDAY 0x00000002
#define WERROR_OPENYEAR 0x00000004
#define WERROR_OPENHOUR 0x00000008
#define WERROR_OPENMINUTE 0x00000010
#define WERROR_OPENSECOND 0x00000020
#define WERROR_DAYOFWEEK 0x00000040
#define WERROR_CLOSEHOUR 0x00000080
#define WERROR_CLOSEMINUTE 0x00000100
#define WERROR_CLOSESECOND 0x00000200
#define WERROR_CUTLENGTH 0x00000400
#define WERROR_EVENTTYPE 0x00000800
#define WERROR_FILENAME 0x00001000
#define WERROR_DEVICE 0x00002000
#define WERROR_SAMPLERATE 0x00004000
#define WERROR_SAMPLESIZE 0x00008000
#define WERROR_CHANNELS 0x00010000
#define WERROR_PIN 0x00020000
#define WERROR_SILENCE 0x00040000
#define WERROR_THRESHOLD 0x00080000
#define WERROR_CUTNAME 0x00100000
#define WERROR_ROTATOR 0x00200000
#define WERROR_RELAYS 0x00400000
#define WERROR_POSTCOMMAND 0x00800000



/*
 * The switcher command terminator codes
 */
#define TERM_NONE 0
#define TERM_LF 1
#define TERM_CRLF 2
#define TERM_CR 3

/*
 * Switcher Device Types
 */
#define SWITCHER_TYPE_SERIAL 0
#define SWITCHER_TYPE_UDP 1

/* 
 * The event status table
 */
extern char sEventStatus[8][39];

/* 
 * The Deck Task Structures
 */
extern unsigned dDeckEvent[4];   /* Current event */
extern unsigned dDeckRecord[4];  /* Current traffic record */
extern time_t tmDeckStart[4];    /* Time current event started */
extern long pPid[4];            /* Process ID of child process */

/* 
 * The Traffic File
 */
extern struct scdTraffic scdTraffic[MAX_EVENTS];
extern int dTrafficRecords;                /* TOTAL TRAFFIC RECORDS */

/* 
 * The rotator table
 */
extern char sRotators[MAX_ROTATOR_DIRS][MAX_ROTATOR_FILES][255];
extern int dRotatorQuan[MAX_ROTATOR_DIRS]; /* NUMBER OF ROTATOR FILES */
extern int dRotatorRecords;
extern char sRandomState[STATE_SIZE];
extern char sNextRotator[MAX_ROTATOR_DIRS][256]; /* Next rotator audio */
/*
 * Execution Control
 */
extern volatile int dRestart;
/*
 * Switcher Audio Sources
 */
extern struct ssSource ssSource[MAX_DECKS][MAX_SOURCES];
extern int dSourceQuan[MAX_DECKS];
extern int dCmdTerm[MAX_DECKS];  /* 0=none, 1=LF, 2=CR/LF 3=CR */
extern int dCmdSize[MAX_DECKS]; 

/* 
 * Deck Statuses
 */
#define DECK_UNAVAILABLE 0
#define DECK_IDLE 1
#define DECK_RECORDING 2
#define DECK_PLAYING 3
#define DECK_WAITFORCLOSE_RECORD 4
#define DECK_WAITFORCLOSE_PLAY 5
#define DECK_WAITFORCLOSE_SWITCH 6
#define DECK_WAITFORSIGNAL_RECORD 7

/* 
 * Event Errors
 *
 * Deck Busy
 */
#define EVENT_ERROR_DECKBUSY 0
/* 
 * Deck Doesn't Exist
 */
#define EVENT_ERROR_NODECK 1
/* 
 * Can't Access Deck
 */
#define EVENT_ERROR_NOACCESS 2
/* 
 * Closure never arrived
 */
#define EVENT_ERROR_NOCLOSURE 3


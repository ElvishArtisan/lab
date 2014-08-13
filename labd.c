/* labd.c
 * 
 * The netcatcher component for the Linux Audio Backstop
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/resource.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <setjmp.h>
#include <pthread.h>

#include "wavlib.h"
#include "conflib.h"
#include "labd.h"



int main(int argc,char *argv[])
{
  int i;
  int dOptionFound;
  time_t tCurrentTime;
  struct tm tmLocalTime;
  FILE *hPidname;
  time_t t,tSleeptime;
  int dNextActive=0;
  time_t tActivetime=0;
  int dActiveEvent=0;
  int dMatch;
  int dDetached=0;
  char sCommand[256];
  sigset_t sigSigchld;
  pthread_t deck_thread[MAX_DECKS];
  pthread_t gpo_thread[MAX_GPIOS];
  pthread_attr_t deck_attr;

  /* 
   * Get Options 
   */
  if(argc>1) {
    for(i=1;i<argc;i++) {
      dOptionFound=0;
      if(strcmp(argv[i],"-v")==0) {   /* Print Version Number */
	printf("The Linux Audio Backstop v%s\n",VERSION);
	printf("\n");
	exit(0);
	dOptionFound=1;
      }
      if(strcmp(argv[i],"-d")==0) {   /* Debug Mode */
	dDebug=1;
	dOptionFound=1;
	printf("labd: debug mode...\n");
      }
      if(dOptionFound==0) {
	  printf(LAMDUSAGE);
	  printf("\n\n");
	  exit(1);
      }
    }
  }
  /* 
   * Set the process priority 
   */
  if(setpriority(PRIO_PROCESS,0,PRIORITY)!=0) {
    printf("labd: can't set priority\n");
    exit(1);
  }

  /*
   * Initialize the signal mask
   */
  sigemptyset(&sigSigchld);
  sigaddset(&sigSigchld,SIGHUP);
  sigaddset(&sigSigchld,SIGTERM);
  sigaddset(&sigSigchld,SIGCHLD);
  
  /*
   * The master loop
   *
   * Catching a SIGHUP will cause this whole loop to be restarted
   */
  /*
   * Reinitialize 
   */
  sigsetjmp(jmpEnv,1);
  Reinit();       /* Globals */
  dNextActive=0;  /* Locals */
  tActivetime=0;
  dActiveEvent=0;

  for(;;) {
    /*
     * Load the Configuration File
     */
    if(LoadConfig(CONF_LOCATION)<0) {
      printf("labd: can't open configuration file\n");
      exit(1);
    }

    /*
     * Detach 
     */
    if((dDebug==0)&&(dDetached==0)) {
      dDetached=1;
      Detach();
    }

    /*
     * Build the thread attributes
     */
    pthread_attr_init(&deck_attr);
    pthread_attr_setdetachstate(&deck_attr,PTHREAD_CREATE_DETACHED);
    if(dUseRealtime==1) {
      pthread_attr_setschedpolicy(&deck_attr,SCHED_FIFO);
    }
    else {
      pthread_attr_setschedpolicy(&deck_attr,SCHED_OTHER);
    }

    /* 
     * Load the traffic file 
     */
    dTrafficRecords=LoadTraffic(sTrafficname,MAX_EVENTS);
    if(dTrafficRecords<0) {
      printf("labd: can't open traffic file\n");
      exit(1);
    }
    /* 
     * Display debug information 
     */
    if(dDebug==1) {
      DumpDebug();
    }
    
    /* 
     * Open the tty device(s)
     */
    if(OpenDevs()<0) {
      exit(1);
    }

    /* 
     * Setup signal handling 
     */
    signal(SIGHUP,SigHandler);
    signal(SIGTERM,SigHandler);
    signal(SIGINT,SigHandler);
    signal(SIGCHLD,SigHandler);
    
    /* 
     * Log the pid 
     */
    hPidname=fopen(PID_FILE,"w");
    if(hPidname==NULL) {
      printf("labd: can't log pid at: %s\n",PID_FILE);
      exit(1);
    }
    fprintf(hPidname,"%d",getpid());
    fclose(hPidname);
    
    /* 
     * Main Loop 
     */
    while(1==1) {
      sigprocmask(SIG_BLOCK,&sigSigchld,NULL);
      for(i=0;i<MAX_DECKS;i++) {
	WriteScoreBoard(SCOREBOARD_FILE,i);
      }
      sigprocmask(SIG_UNBLOCK,&sigSigchld,NULL);
      time(&tCurrentTime);
      
      /*
       * Seek ahead for SEEK_LIMIT seconds for the next event
       * If we find one, cue it up, then sleep until it's time to run it
       */
      dMatch=0;
      for(t=tCurrentTime;t<tCurrentTime+SEEK_LIMIT;t++) {
	tmLocalTime=*localtime(&t);
	for(i=0;i<dTrafficRecords;i++) {
	  if(MatchTime(i,&tmLocalTime)==0) {
	    
	    /* Event Match! */
	    if((t!=tActivetime) || (dActiveEvent!=i)) {  /* So we don't repeat 
							    the same event */
	      tActivetime=t;
	      dActiveEvent=i;
	      dNextActive=i;
	      t=tCurrentTime+SEEK_LIMIT+2;
	      i=dTrafficRecords+1;
	      dMatch=1;
	    }
	  }
	}
      }
      if(dMatch==0) {  /* just wait */
	tSleeptime=SEEK_LIMIT-SLEEP_GAP;
      }
      else {
	tSleeptime=tActivetime-(tCurrentTime+SLEEP_GAP);
	if(tSleeptime<0) {
	  tSleeptime=0;
	}
      }
      if(dDebug==1) {
	printf("Sleeping for %u seconds!\n",(unsigned)tSleeptime);
	if(dMatch==1) {
	  printf("Using event: %d\n\n",dNextActive);
	}
	else {
	  printf("No event found\n");
	}
      }
      tSleeptime=sleep(tSleeptime);
      if(dRestart==0) {
	/* Restore State */
	i=dNextActive;
	
	/* If we've got an event cued up, wait for the time, then run it */
	if(dMatch==1) {
	  time(&tCurrentTime);
	  while(tCurrentTime<tActivetime) {
	    time(&tCurrentTime);
	  }
	  
	  /* Check the deck status */
	  if((dDeckEvent[scdTraffic[i].dDevice]==DECK_IDLE)|| // Available
	    (scdTraffic[i].dEventType==EVENT_RELAY_PULSE)||
	    (scdTraffic[i].dEventType==EVENT_TIMED_SWITCHER)) {
	    /* Initialize deck task structure */
	    if((scdTraffic[i].dEventType!=EVENT_RELAY_PULSE)&&
	       (scdTraffic[i].dEventType!=EVENT_TIMED_SWITCHER)) {
	      dDeckRecord[scdTraffic[i].dDevice]=i;
	      tmDeckStart[scdTraffic[i].dDevice]=tCurrentTime;
	    }

	    /* Launch the task */
	    switch(scdTraffic[i].dEventType) {
	    case EVENT_TIMED_RECORD:
	      if(dDebug==1) {
		printf("Recording: %s on %s\n",scdTraffic[i].sFilename,
		       sAudioDevice[scdTraffic[i].dDevice]);
	      }
	      dDeckEvent[scdTraffic[i].dDevice]=DECK_RECORDING;
	      /*
	       * Switch the channel
	       */
	      if(GetSwitchCommand(scdTraffic[i].dDevice,
				  scdTraffic[i].sCutName,sCommand)==0) {
		SendSwitchCommand(scdTraffic[i].dDevice,sCommand);
	      }
	      pthread_create(&deck_thread[scdTraffic[i].dDevice],&deck_attr,
			     StartRecordEvent,&scdTraffic[i]);
	      break;
	    case EVENT_TIMED_PLAYBACK:
	      if(dDebug==1) {
		printf("Playing: %s on %s\n",scdTraffic[i].sFilename,
		       sAudioDevice[scdTraffic[i].dDevice]);
	      }
	      dDeckEvent[scdTraffic[i].dDevice]=DECK_PLAYING;
	      /*
	       * Switch the channel
	       */
	      if(GetSwitchCommand(scdTraffic[i].dDevice,
				  scdTraffic[i].sCutName,sCommand)==0) {
		SendSwitchCommand(scdTraffic[i].dDevice,sCommand);
	      }
	      pthread_create(&deck_thread[scdTraffic[i].dDevice],&deck_attr,
			     StartPlayEvent,&scdTraffic[i]);
	      break;
	    case EVENT_TIMED_SWITCHER:
	      /*
	       * Switch the channel
	       */
	      if(GetSwitchCommand(scdTraffic[i].dDevice,
				  scdTraffic[i].sCutName,sCommand)==0) {
		SendSwitchCommand(scdTraffic[i].dDevice,sCommand);
	      }
	      break;
	    case EVENT_CLOSURE_RECORD:
	      dDeckEvent[scdTraffic[i].dDevice]=DECK_WAITFORCLOSE_RECORD;
	      break;
	    case EVENT_CLOSURE_PLAYBACK:
	      dDeckEvent[scdTraffic[i].dDevice]=DECK_WAITFORCLOSE_PLAY;
	      break;
	    case EVENT_CLOSURE_SWITCHER:
	      break;
	    case EVENT_THRESHOLD_RECORD:
	      break;
	    case EVENT_RELAY_PULSE:
	      if(dDebug==1) {
		printf("Pulsing Relay: %d\n",scdTraffic[i].dPin);
	      }
	      if(pthread_create(&gpo_thread[scdTraffic[i].dPin],NULL,
				PulseRelay,&scdTraffic[i])==0) {
		pthread_detach(gpo_thread[scdTraffic[i].dPin]);
	      }
	      break;
	    }
	  }
	}
      }
    }
  }
  exit(0);
}  



void Detach() 
{
  if(daemon(0,0)) {
    perror("labd");
    exit(1);
  }
}


void PlayCut(char *sFilename,char *sAudio,int dMainVolValue,int dAudioVol)
{
  if(dDebug==1) {
    printf("Playing: ");
    printf("%s",sFilename);
    printf("\n");
  }
  /* Play the audio */
  PlayWav(sFilename,sAudio,0);
}



void Reinit(void)
{
  int i;

  for(i=0;i<MAX_DECKS;i++) {
    hAudioDevice[i]=0;
    dAudioDevice[i]=0;
    hSwitcherDevice[i]=0;
    dSwitcherDevice[i]=0;
  }
  dTrafficRecords=0;
  dRestart=0;
}



void SigHandler(int signo)
{
  int i;
  char name[PATH_LEN];
  pid_t pLocalPid;

  switch(signo) {
      case SIGHUP:
	if(dDebug==1) {
	  printf("RESTARTING...\n\n");
	}
	siglongjmp(jmpEnv,1);
	
      case SIGTERM:
      case SIGINT:
	unlink(PID_FILE);
	for(i=0;i<MAX_DECKS;i++) {
	  sprintf(name,"%s%d",SCOREBOARD_FILE,i);
	  unlink(name);
	}
	exit(0);
  
      case SIGCHLD:
	pLocalPid=waitpid(-1,NULL,WNOHANG);
	while(pLocalPid>0) {
	  pLocalPid=waitpid(-1,NULL,WNOHANG);
	}
	signal(SIGCHLD,SigHandler);
	break;
  }
}



void MakeMark(char *sMark)
{
  FILE *hFilename;

  hFilename=fopen("MARK","w");
  if(hFilename==NULL) {
    printf("mark: can't open file\n");
  }
  fprintf(hFilename,"%s\n",sMark);
}




void DumpDebug(void)
{
  int i;
  /* int j; */

  printf("Using config file: ");
  printf(CONF_LOCATION);
  printf("\n");
  printf("Using traffic file: %s\n",sTrafficname);
  if(dUseRealtime==1) {
    printf("Using realtime scheduling\n");
  }
  for(i=0;i<4;i++) {
    if(dAudioDevice[i]==1) {
      printf("Audio Device %d: %s\n",i,sAudioDevice[i]);
    }
  }
  for(i=0;i<4;i++) {
    if(dSwitcherDevice[i]==1) {
      printf("Switcher Device %d: %s at %d bps\n",i,sSwitcherDevice[i],
	     GetRealBaudRate(dSwitcherBaud[i]));
    }
  }
  printf("Traffic Entries: %d\n",dTrafficRecords);
  for(i=0;i<MAX_DECKS;i++) {
    if(dAudioDevice[i]==1) {
      printf("Deck %d Audio Sources:\n",i);
      /*
      for(j=0;j<dSourceQuan[i];j++) {
	printf("  %20s : %s\n",ssSource[i][j].sName,
	      ssSource[i][j].sCommand);
      }
      */
    }
  }
}


int OpenDevs(void)
{
  int i;
  struct termios termTermios;
#ifdef HAVE_GPIO
  unsigned mode;
#endif  // HAVE_GPIO

  for(i=0;i<4;i++) {
    if(dSwitcherDevice[i]==1) {
      switch(dSwitcherType[i]) {
	  case SWITCHER_TYPE_SERIAL:
	    hSwitcherDevice[i]=open(sSwitcherDevice[i],O_RDWR|O_NOCTTY);
	    if(hSwitcherDevice[i]<0) {
	      printf("labd: can't open %s\n",sSwitcherDevice[i]);
	      return -1;;
	    }
	    tcgetattr(hSwitcherDevice[i],&termTermios);
	    cfsetispeed(&termTermios,dSwitcherBaud[i]);
	    cfsetospeed(&termTermios,dSwitcherBaud[i]);
	    cfmakeraw(&termTermios);
	    tcsetattr(hSwitcherDevice[i],TCSANOW,&termTermios);
	    break;
	    
	  case SWITCHER_TYPE_UDP:
	    hSwitcherDevice[i]=socket(PF_INET,SOCK_DGRAM,0);
	    if(hSwitcherDevice[i]<0) {
	      perror("labd");
	      return -1;
	    }
	    break;
      }
    }
  }

  /*
   * GPIO Devices
   */
#ifdef HAVE_GPIO
  memset(&gpo_info,0,sizeof(struct gpio_info));
  if((hGpoDevice=open(sGpoDevice,O_RDWR))>0) {
    mode=GPIO_MODE_OUTPUT;
    ioctl(hGpoDevice,GPIO_SETMODE,&mode);
    ioctl(hGpoDevice,GPIO_GETINFO,&gpo_info);
  }
  else {
    hGpoDevice=-1;
  }
  if(strcasecmp(sGpoDevice,sGpiDevice)) {
    if((hGpiDevice=open(sGpiDevice,O_RDWR))>0) {
      mode=GPIO_MODE_INPUT;
      ioctl(hGpiDevice,GPIO_SETMODE,&mode);
      ioctl(hGpiDevice,GPIO_GETINFO,&gpi_info);
    }
    else {
      hGpoDevice=-1;
    }
  }
  else {
    hGpiDevice=hGpoDevice;
    gpi_info=gpo_info;
  }
#else
  hGpiDevice=-1;
  hGpoDevice=-1;
#endif  // HAVE_GPIO    
  return 0;
}


void CloseDevs(void)
{
  int i;

  for(i=0;i<4;i++) {
    if(dSwitcherDevice[i]==1) {
      close(hSwitcherDevice[i]);
    }
  }
  if(hGpoDevice>0) {
    close(hGpoDevice);
  }
  if((hGpiDevice!=hGpoDevice)&&(hGpiDevice>0)) {
    close(hGpiDevice);
  }
  hGpiDevice=-1;
  hGpoDevice=-1;
}



int MatchTime(int dRec,struct tm *tmTime)
{
  unsigned dWeekMask=0;

  if((scdTraffic[dRec].dOpenHour==tmTime->tm_hour || 
      scdTraffic[dRec].dOpenHour==WILDCARD) && 
     (scdTraffic[dRec].dOpenMinute==tmTime->tm_min || 
      scdTraffic[dRec].dOpenMinute==WILDCARD) && 
     (scdTraffic[dRec].dOpenSecond==tmTime->tm_sec)) {   /* Open Time */

    if((scdTraffic[dRec].dOpenMonth!=WILDCARD)||
       (scdTraffic[dRec].dOpenDay!=WILDCARD)||
       (scdTraffic[dRec].dOpenYear!=WILDCARD)) {
      if((scdTraffic[dRec].dOpenMonth==tmTime->tm_mon || 
	  scdTraffic[dRec].dOpenMonth==WILDCARD) && 
	 (scdTraffic[dRec].dOpenDay==tmTime->tm_mday || 
	  scdTraffic[dRec].dOpenDay==WILDCARD) && 
	 (scdTraffic[dRec].dOpenYear==tmTime->tm_year || 
	  scdTraffic[dRec].dOpenYear==WILDCARD)) {  
	return 0;
      }
      else {
	return -1;
      }
    }
    else {
      dWeekMask=1<<tmTime->tm_wday;
      if(((scdTraffic[dRec].dDayOfWeek&dWeekMask)!=0)||
	 (scdTraffic[dRec].dDayOfWeek==WILDCARD)) {
	return 0;
      }
      else {
	return -1;
      }
    }
  }
  return -1;
}



int GetSwitchCommand(int dDeck,char *sString,char *sCommand)
{
  int i;

  for(i=0;i<dSourceQuan[dDeck];i++) {
    if(strcasecmp(sString,ssSource[dDeck][i].sName)==0) {
      strcpy(sCommand,ssSource[dDeck][i].sCommand);
      return 0;
    }
  }
  return -1;
}



int WriteScoreBoard(char *sFilename,int deck)
{
  FILE *hFilename;
  char name[PATH_LEN];

  sprintf(name,"%s%d",sFilename,deck);
  hFilename=fopen(name,"w");
  if(hFilename==NULL) {
    return -1;
  }
  if((dDeckEvent[deck]==DECK_RECORDING)||(dDeckEvent[deck]==DECK_PLAYING)) {
    fprintf(hFilename,"%d\n%s\n",dDeckEvent[deck],
	    scdTraffic[dDeckRecord[deck]].sFilename);
    fprintf(hFilename,"%s\n",scdTraffic[dDeckRecord[deck]].sCutName);
    fprintf(hFilename,"%lu\n%d\n",(unsigned long)tmDeckStart[deck],
	    scdTraffic[dDeckRecord[deck]].dCutLength);
  }
  else {
    fprintf(hFilename,"%d\nN/A\nN/A\n0\n0\n",dDeckEvent[deck]);
  }
  fclose(hFilename);
  return 0;
}


void *PulseRelay(void *ptr)
{
#ifdef HAVE_GPIO
  struct gpio_line gpio_line;
  struct scdTraffic *traffic=(struct scdTraffic *)ptr;

  if(traffic->dPin>=gpo_info.outputs) {
    return 0;
  }
  memset(&gpio_line,0,sizeof(struct gpio_line));
  gpio_line.line=traffic->dPin;
  gpio_line.state=1;
  ioctl(hGpoDevice,GPIO_SET_OUTPUT,&gpio_line);
  usleep(RELAY_PULSE_TIME);
  gpio_line.state=0;
  ioctl(hGpoDevice,GPIO_SET_OUTPUT,&gpio_line);
#endif  // HAVE_GPIO
  return 0;
}


void *StartRecordEvent(void *ptr)
{
  struct scdTraffic *traffic=(struct scdTraffic *)ptr;
  pthread_t record_thread;
  pthread_attr_t record_attr;
  struct gpiTrans *gpiTrans=NULL;
  int trans_size=0;
  int i;
#ifdef HAVE_GPIO
  struct timezone tz;
#endif  // HAVE_GPIO

  /*
   * Build the thread attributes
   */
  pthread_attr_init(&record_attr);
  pthread_attr_setdetachstate(&record_attr,PTHREAD_CREATE_JOINABLE);
  if(dUseRealtime==1) {
    pthread_attr_setschedpolicy(&record_attr,SCHED_FIFO);
  }
  else {
    pthread_attr_setschedpolicy(&record_attr,SCHED_OTHER);
  }

  /*
   * Resolve the macros
   */
  time_t tTime=time(NULL);
  for(i=0;i<1024;i++) {
    traffic->sResolvedName[i]=0;
    traffic->sResolvedCmd[i]=0;
  }
  ResolveMacros(tTime,"",traffic->sFilename,traffic->sResolvedName,1024);
  ResolveMacros(tTime,traffic->sResolvedName,traffic->sCommand,
		traffic->sResolvedCmd,1024);

  /*
   * Start the record
   */
  pthread_create(&record_thread,&record_attr,StartRecord,traffic);

  /* 
   * Record GPI Transitions During Record
   */
#ifdef HAVE_GPIO
  if(hGpiDevice>0) {
    gpiTrans=malloc(2*sizeof(struct gpiTrans));
    trans_size=2;
    ioctl(hGpiDevice,GPIO_GET_INPUTS,&(gpiTrans[0].mask));
    if(dInvertGpi==1) {
      InvertMask(&(gpiTrans[0].mask));
    }
    gettimeofday(&(gpiTrans[0].tv),&tz);
    while(dDeckEvent[traffic->dDevice]!=DECK_IDLE) {
      ioctl(hGpiDevice,GPIO_GET_INPUTS,&(gpiTrans[trans_size-1].mask));
      if(dInvertGpi==1) {
	InvertMask(&(gpiTrans[0].mask));
      }
      if(gpiTrans[trans_size-1].mask.mask[0]!=
	 gpiTrans[trans_size-2].mask.mask[0]) {
	if(dDebug==1) {
	  printf("recording gpi transition\n");
	}
        gettimeofday(&(gpiTrans[trans_size-1].tv),&tz);
        gpiTrans=realloc(gpiTrans,++trans_size*sizeof(struct gpiTrans));
      }
      usleep(GPI_SCAN_INTERVAL);
    }
  }
#endif  // HAVE_GPIO

  /*
   * We're done with realtime, so revert to normal scheduling
   */
  pthread_setschedparam(pthread_self(),SCHED_OTHER,NULL);
  pthread_join(record_thread,NULL);

  /*
   * Post Processing
   */
  if(traffic->dThreshold!=0) {
    TailTrim(traffic->sFilename,(int)(pow(10,-traffic->dThreshold/20)*32768));
  }

  /*
   * Generate Relay Records
   */
  if(hGpiDevice>0) {
    if(trans_size>2) {
      WriteRelays(traffic,gpiTrans,trans_size);
    }
    free(gpiTrans);
  }

  /*
   * Run the post command
   */
  PostCommand(traffic);

  /*
   * Clean Up and Exit
   */
  return 0;
}


void *StartRecord(void *ptr)
{
  struct wavHeader wavHeader;
  struct wavProcess wavProcess;
  struct scdTraffic *traffic=(struct scdTraffic *)ptr;
  int dAccum;

  /* 
   * Make the recording 
   */
  wavHeader.wFormatTag=WAVE_FORMAT_PCM;
  wavHeader.wChannels=(short)traffic->dChannels;
  wavHeader.dwSamplesPerSec=traffic->dSampleRate;
  wavHeader.wBitsPerSample=traffic->dSampleSize;
  wavProcess.dfThreshold=traffic->dThreshold;
  wavProcess.dSenseTimeout=traffic->dSilence;
  wavProcess.dfNormalLevel=-1;
  dAccum=RecordWavFile(traffic->sResolvedName,      /* Filename */
		       sAudioDevice[traffic->dDevice],
		       traffic->dCutLength,         /* Length */
		       &wavHeader,                  /* Audio Format */
		       &wavProcess,                 /* Process Options */
		       0,                           /* General Options */
		       dOwner,                      /* File Owner */
		       dGroup);                     /* File Group */
  if((dDebug==1)&&(dAccum<0)) {
    printf("RECORD FAULT!\n");
  }
  ioctl(hAudioDevice[traffic->dDevice],SNDCTL_DSP_RESET,
	NULL);
  ioctl(hAudioDevice[traffic->dDevice],SNDCTL_DSP_SYNC,
	NULL);
  dDeckEvent[traffic->dDevice]=DECK_IDLE;
  WriteScoreBoard(SCOREBOARD_FILE,traffic->dDevice);
  pthread_exit((int *)dAccum);
}


void WriteRelays(struct scdTraffic *traffic,struct gpiTrans *gpiTrans,int size)
{
#ifdef HAVE_GPIO
  int i;
  int j;
  struct wavTimer *wavTimer=NULL;
  int timer_size=0;
  unsigned mask;
  unsigned bit;
  double base_time;

  for(i=1;i<(size-1);i++) {
    if((mask=(gpiTrans[i].mask.mask[0]^gpiTrans[i-1].mask.mask[0])&
	gpiTrans[i].mask.mask[0])!=0) {
      base_time=GetFlatTime(&(gpiTrans[0].tv));
      for(j=0;j<gpi_info.inputs;j++) {
	bit=1<<j;
	if(((mask&bit)!=0)&&(traffic->gpioMap[j]!=-1)) {
	  wavTimer=(struct wavTimer *)
	    realloc(wavTimer,++timer_size*sizeof(struct wavTimer));
	  wavTimer[timer_size-1].cLabel=traffic->gpioMap[j];
	  wavTimer[timer_size-1].dTime=
	    (unsigned)(1000.0*(GetFlatTime(&(gpiTrans[i].tv))-base_time));
	}
      }
    }
  }
  if(wavTimer!=NULL) {
    if(dDebug==1) {
      printf("Writing %d Relays to %s\n",timer_size,traffic->sFilename);
    }
    WriteTimerChunk(traffic->sFilename,wavTimer,timer_size);
    free(wavTimer);
  }
#endif  // HAVE_GPIO
}


void *StartPlayEvent(void *ptr)
{
  int i;
  struct scdTraffic *traffic=(struct scdTraffic *)ptr;
  struct wavTimer *wavTimer=NULL;
  int timer_size=0;
  pthread_t play_thread;
  pthread_attr_t play_attr;
  pthread_t gpo_thread;
  pthread_attr_t gpo_attr;
  struct timeval tv;
  struct timezone tz;
  unsigned base_time;
  unsigned current_time;
  int relay;

  /*
   * Read the Relay Data
   */
  wavTimer=ReadTimerChunk(traffic->sFilename,&timer_size);
  if(dDebug==1) {
    printf("found %d relay events\n",timer_size);
    printf("Relay  Time\n");
    printf("-----------\n");
    for(i=0;i<timer_size;i++) {
      printf("  %2d   %4u\n",(int)(0xFF&wavTimer[i].cLabel),wavTimer[i].dTime);
    }
  }

  /*
   * Build the thread attributes
   */
  pthread_attr_init(&play_attr);
  pthread_attr_setdetachstate(&play_attr,PTHREAD_CREATE_JOINABLE);
  if(dUseRealtime==1) {
    pthread_attr_setschedpolicy(&play_attr,SCHED_FIFO);
  }
  else {
    pthread_attr_setschedpolicy(&play_attr,SCHED_OTHER);
  }
  pthread_attr_init(&gpo_attr);
  pthread_attr_setdetachstate(&gpo_attr,PTHREAD_CREATE_DETACHED);
  if(dUseRealtime==1) {
    pthread_attr_setschedpolicy(&gpo_attr,SCHED_FIFO);
  }
  else {
    pthread_attr_setschedpolicy(&gpo_attr,SCHED_OTHER);
  }

  /*
   * Resolve the macros
   */
  time_t tTime=time(NULL);
  for(i=0;i<1024;i++) {
    traffic->sResolvedName[i]=0;
    traffic->sResolvedCmd[i]=0;
  }
  ResolveMacros(tTime,"",traffic->sFilename,traffic->sResolvedName,1024);
  ResolveMacros(tTime,traffic->sResolvedName,traffic->sCommand,
		traffic->sResolvedCmd,1024);

  /*
   * Start the playout
   */
  pthread_create(&play_thread,&play_attr,StartPlay,traffic);

  /*
   * Output Relays
   */
  gettimeofday(&tv,&tz);
  base_time=(unsigned)(1000.0*GetFlatTime(&tv));
  for(i=0;i<timer_size;i++) {
    gettimeofday(&tv,&tz);
    current_time=(unsigned)(1000.0*GetFlatTime(&tv));
    while(current_time<=(base_time+wavTimer[i].dTime)) {
      usleep(1000*(base_time+wavTimer[i].dTime-current_time));
      gettimeofday(&tv,&tz);
      current_time=(unsigned)(1000.0*GetFlatTime(&tv));
    }
    if(wavTimer[i].cLabel<MAX_GPIOS) {
      if((relay=traffic->gpioMap[(unsigned)wavTimer[i].cLabel])!=-1) {
	if(dDebug==1) {
	  printf("pulsing relay %d\n",relay);
	}
	pthread_create(&gpo_thread,&gpo_attr,PulseRelayNumber,&relay);
      }
    }
  }

  /*
   * We're done with realtime, so revert to normal scheduling
   */
  pthread_setschedparam(pthread_self(),SCHED_OTHER,NULL);
  pthread_join(play_thread,NULL);

  /*
   * Run the post command
   */
  PostCommand(traffic);
  return 0;
}


void *StartPlay(void *ptr)
{
  char sAccum[256];
  struct scdTraffic *traffic=(struct scdTraffic *)ptr;

  /* 
   * Play it 
   */
  if(PlayWavFile(traffic->sResolvedName,        /* Filename */
		 sAudioDevice[traffic->dDevice],
		 0)!=0) {                       /* Options */
    sprintf(sAccum,"labd: couldn't access device %s\n",
	    sAudioDevice[traffic->dDevice]);
  }
  dDeckEvent[traffic->dDevice]=DECK_IDLE;
  WriteScoreBoard(SCOREBOARD_FILE,traffic->dDevice);
  return 0;
}


void *PulseRelayNumber(void *ptr)
{
#ifdef HAVE_GPIO
  struct gpio_line gpio_line;
  int *gpo=(int *)ptr;

  if(*gpo>=gpo_info.outputs) {
    return 0;
  }
  memset(&gpio_line,0,sizeof(struct gpio_line));
  gpio_line.line=*gpo;
  gpio_line.state=1;
  ioctl(hGpoDevice,GPIO_SET_OUTPUT,&gpio_line);
  usleep(RELAY_PULSE_TIME);
  gpio_line.state=0;
  ioctl(hGpoDevice,GPIO_SET_OUTPUT,&gpio_line);
#endif  // HAVE_GPIO
  return 0;
}


double GetFlatTime(struct timeval *tv)
{
  return (double)tv->tv_sec+((double)tv->tv_usec)/1000000.0;
}


void SendSwitchCommand(int dDevice,char *sCommand)
{
  char sTermtab[4][3]={{0,0,0},{10,0,0},{13,10,0},{13,0,0}};

  if(dDebug==1) {
    printf("Sending switcher string: %s to %s\n",
	   sCommand,sSwitcherDevice[dDevice]);
  }
  switch(dSwitcherType[dDevice]) {
      case SWITCHER_TYPE_SERIAL:
	write(hSwitcherDevice[dDevice],sCommand,strlen(sCommand));
	write(hSwitcherDevice[dDevice],sTermtab[dCmdTerm[dDevice]],
	      dCmdSize[dDevice]);
	break;

      case SWITCHER_TYPE_UDP:
	sendto(hSwitcherDevice[dDevice],sCommand,strlen(sCommand),0,
	       (const struct sockaddr *)&inSwitcherAddress[dDevice],
	       sizeof(struct sockaddr_in));
	break;
  }
}


void PostCommand(struct scdTraffic *traffic)
{
  if(fork()!=0) {
    return;
  }
  setgid(dGroup);
  setuid(dOwner);
  if(traffic->sResolvedCmd[0]!=0) {
    system(traffic->sResolvedCmd);
  }
  if(traffic->dDeleteSource==1) {
    unlink(traffic->sResolvedName);
  }
  exit(0);
}


#ifdef HAVE_GPIO
void InvertMask(struct gpio_mask *mask)
{
  mask->mask[0]=~(mask->mask[0]);
  mask->mask[1]=~(mask->mask[1]);
  mask->mask[2]=~(mask->mask[2]);
  mask->mask[3]=~(mask->mask[3]);
}
#endif  // HAVE_GPIO

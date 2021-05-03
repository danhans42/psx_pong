#pragma once

//#define CD_DEBUG 1


// ulong
#include <stdlib.h>

#include "config.h"
#include "filetypes.h"
#include "hwregs.h"
#include "littlelibc.h"

#define COM_DELAY 0xBF801020
#define pCOM_DELAY *(volatile ulong*)COM_DELAY

#define CD_CMD_SYNC         0x00
#define CD_CMD_GETSTAT      0x01  // also known as nop
#define CD_CMD_SETLOC       0x02
#define CD_CMD_READN        0x06
#define CD_CMD_PAUSE        0x09
#define CD_CMD_INIT         0x0A
#define CD_CMD_SETMODE      0x0E
#define CD_CMD_SETSESSION   0x12
#define CD_CMD_SEEKL        0x15

#define CD_CMD_TEST         0x19
  #define CD_TESTPARAM_STOP   0x03
#define CD_CMD_READS        0x1B
#define CD_CMD_RESET        0x1C



#define FIFO_STAT_NOINTR_0 0x0
#define FIFO_STAT_DATAREADY_1 0x1
#define FIFO_STAT_ACK_2 0x2
#define FIFO_STAT_COMPLETE_3 0x3
#define FIFO_STAT_DATAEND_4 0x4
#define FIFO_STAT_ERROR_5 0x5

//
// Non-kernel stuff
//

// Rounds a size up to 2K for loading a sector.
ulong Pad2k(ulong inSize);

int CDReadSector( ulong inCount, ulong inSector, char * inBuffer );
int CDOpenAndRead(const char* inFile, char * inBuffer, ulong numSectors);

ulong CDCheckUnlocked();

void CDStop();
void CDWaitReady();

//
// Sending a custom command
//

void CDStartCommand();
void CDWriteParam( unsigned char inParam );
void CDWriteCommand( unsigned char inCommand );

int CDAck();              // called 1-many times after each writecommand (one for each expected int)
ulong CDLastInt();        // read the last interrupt generated from CDAck();
ulong CDLastResponse();   // read the last response generated from CDAck();
int CDReadResponse();     // for e.g. GetLocP which requires multiple responses for one ack

void CDInitForShell();

// for XStation, etc
void DoNotCheckIfDriveUnlocked();

//
// Kernel Stuff
//
void CDLoadAndExecute(const char* fileName, ulong stackAddr, ulong stackOffset);


#if DEBUG_STUFF

void CDTestCode();

#endif

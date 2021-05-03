// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
// Low(ER) level CD functions for Unirom 8
// File loading & handling is over in cdload.c
//

// References:
// http://hitmen.c02.at/files/docs/psx/psx.pdf
// https://github.com/nicolasnoble/pcsx-redux/tree/bios-work/src/mips/openbios/cdrom
// https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/cdrom/helpers.c
// http://psx.rules.org/cdinfo.txt
// http://www.raphnet.net/electronique/psx_adaptor/Playstation.txt
// https://github.com/ChenThread/candyk-psx/blob/master/src/seedy/seedy.c
// ^ I absolutely love this format with StartCommand(), WriteParam()s, WriteCommand()
//   so let's go with this!



#include "cd.h"

#include "config.h"
#include "drawing.h"
#include "filetypes.h"
#include "hwregs.h"
#include "utility.h"
#include "filebrowser.h"
#include "cdunlock.h"

//
// Vars
//

// Last response + int from the HW
ulong lastInt = 0;
ulong lastResponse = 0;

static char didCDInit = 0;
static char checkIfDriveUnlocked = 1;

static char didNocashUnlock = 0;
static ulong waitTimer = 0;

// a shared, working copy
static struct DirEntry defaultDir;

#define CD_IRQ_MASK 0x04

#define REG3_IDX1_RESET_PARAM_FIFO 0x40
#define REG3_IDX0_REQUEST_DATA 0x80

#define CDREG0_DATA_IN_RESPONSEFIFO 0x20
#define CDREG0_DATA_IN_DATAFIFO 0x40
#define CDREG0_DATA_BUSY 0x80

//
// Protos
//
static ulong CDAdd();
static ulong CDRemove();
static int CDClearInts();


#pragma region COMMAND TRIO


        // this trio forms the basis of sending a command to the drive
        /// StartCommand, WriteParams, WriteCommand

        void CDStartCommand(){

            int i;
            
            //CDWaitBusy();

            while( (pCDREG0 & CDREG0_DATA_IN_DATAFIFO) != 0 );    
            while( (pCDREG0 & CDREG0_DATA_BUSY ) != 0 );

            // Select Reg3,Index 1 : 0x1F resets all IRQ bits        
            CDClearInts();

            // Reg2 Index 0 = param fifo
            pCDREG0 = 0;
        }
            

        void CDWriteParam( uchar inParam ){    
            
            //pCDREG0 = 0;                //not required in a loop, but good practice?
            pCDREG2 = inParam;
            
        }

        #pragma GCC push options
        #pragma GCC optimize("-O0")
        void CDWriteCommand( uchar inCommand ){
            // Finish by writing the command
            NewPrintf( "CDWriteCommand(%x)\n", inCommand );
            pCDREG0 = 0;
            pCDREG1 = inCommand;    
            
        }
        #pragma gcc pop options

#pragma endregion COMMAND TRIO
     

#pragma region convenience functions


    // identify the convenience functions by CDSendCommand_?
    static void CDSendCommand_Stop(){
        CDStartCommand();
        CDWriteParam( CD_TESTPARAM_STOP );
        CDWriteCommand( CD_CMD_TEST );
    }

    void CDStop(){
        CDSendCommand_Stop();
        CDAck();
    }

    // Response: Int3, Int2
    static void CDSendCommand_SetSession( int inSession ){

        CDStartCommand();
        CDWriteParam( inSession );
        CDWriteCommand( CD_CMD_SETSESSION );

    }

    // Response: Int3
    static void CDSendCommand_GetStat(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_GETSTAT );
    }

    // Response: Int3, Int2
    static void CDSendCommand_Init(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_INIT );
    }

    // Response: Int3
    static void CDSendCommand_SetLoc( int inMin, int inSec, int inSub ){

        // skipping the itob
        CDStartCommand();
        CDWriteParam( inMin );
        CDWriteParam( inSec );
        CDWriteParam( inSub );
        CDWriteCommand( CD_CMD_SETLOC );

    }

    // Response: Int3, Int 2
    static void CDSendCommand_SeekL(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_SEEKL );
    }

    // Response: Int3, Int1
    static void CDSendCommand_ReadS(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_READS );
    }

    // Response: Int3, Int1
    static void CDSendCommand_ReadN(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_READN );
    }

    /*
    // pseudocode
    static void CDSendCommand_CheckLid(){
        CDStartCommand();
        CDWriteParam( 0x21 );
        CDSendCommand( 0x19 );
        CDAck();
        if ( lastInt == FIFO_STAT_COMPLETE_3 ){
            if ( (lastResponse & 0xwhatever ) != 0 ){

            }
        }else {
            // something fucky
        }

    }
    */

    // Response: Int3, Int1
    static void CDSendCommand_Pause(){
        CDStartCommand();
        CDWriteCommand( CD_CMD_PAUSE );
    }

    // Response: Int3
    static void CDSendCommand_SetLocLBA( int sector ){

        sector += 75 * 2;

        // hmm is this really better than using an i to bcd function :p?

        int mins = sector / 4500;
        sector %= 4500;

        int min = (mins % 10) + (mins / 10) * 0x10;
        int sec = ((sector / 75) % 10) + ((sector / 75) / 10) * 0x10;
        int sub = ((sector % 75) % 10) + ((sector % 75) / 10) * 0x10;

        CDSendCommand_SetLoc( min, sec, sub );

    }

    // Response: Int3
    static void CDSendCommand_SetMode( int inMode ){

        CDStartCommand();
        CDWriteParam( inMode );
        CDWriteCommand( CD_CMD_SETMODE );

    }

#pragma endregion convenience functions


#pragma region CD Logic

    static __attribute__((always_inline)) int CDClearInts(){
        
        pCDREG0 = 1;
        pCDREG3 = 0x1F;

        // put it back
        pCDREG0 = 0;

    }    

    // Wait till there's nothing left in the fifo
    static void EightyWaity(){
        pCDREG0 = 0;    
        pCDREG3 = REG3_IDX0_REQUEST_DATA;
        while ( ( pCDREG0 & CDREG0_DATA_IN_DATAFIFO ) == 0 ){
            volatile int x = 0;
        }
    }
    
    int CDWaitInt(){
        
        // reset all IRQ
        CDClearInts();
        
        // Reg 3 index 1 = Interrupt flags
        // get int 7
        pCDREG0 = 1;
        while( (pCDREG3 & 0x07) == 0 );

        int returnVal = ( pCDREG3 & 0x07 );
        pCDREG3 = REG3_IDX1_RESET_PARAM_FIFO | 0x07;

        // index it back to 0
        pCDREG0 = 0;

        return returnVal;

    }

    int CDReadResponse(){
        // select response Reg1, index1 : Response fifo
        pCDREG0 = 0x01;
        char returnValue = pCDREG1;

        // put it back?
        pCDREG0 = 0;
        return returnValue;
    }

    #pragma GCC options push
    #pragma GCC optimise("-O0")  // required to get the printf in the right order
    int CDAck(){
        lastInt = CDWaitInt();
        lastResponse = CDReadResponse();
        CDClearInts();
        //NewPrintf( "CDAck() int = %x, resp = %x\n", lastInt, lastResponse );
        return lastResponse;
    }
    #pragma GCC options pop


    ulong CDLastInt(){
        return lastInt;
    }

    ulong CDLastResponse(){
        return lastResponse;
    }


    //ulong CDReadSector(ulong inCount, ulong inSector, ulong inBuffer)
    // TODO: char buffer
    // Returns the bytes read
    int CDReadSector( ulong inCount, ulong inSector, char * inBuffer ){

        
        ulong retries = 0;
        ulong maxretries = 10;
        ulong bufferOffset = 0;

        memset( inBuffer, 0, inCount * 2048 );

        // SetLoc
        // single speed, 0x800 sector size        
        CDSendCommand_SetMode( 0x08 );
        CDAck();

        do{
        
            CDSendCommand_SetLocLBA( inSector );
            CDAck();
            
            if ( retries++ > maxretries )
                return -1;

        } while ( lastInt != FIFO_STAT_COMPLETE_3 );
        
        // Don't need to seek!

        // Read

        retries = 0;
        while ( 1 ){

            CDSendCommand_ReadN();

            CDAck();
            if ( lastInt != FIFO_STAT_COMPLETE_3 ){
                if ( retries ++ > maxretries )
                    return -5;
                continue;
            }

            // Will fail here if drive locked
            CDAck();
            if ( lastInt != FIFO_STAT_DATAREADY_1 ){
                if ( retries ++ > maxretries )
                    return -6;
                continue;
            }

            break;

        }

        // Get the data
        
        EightyWaity();
        
        for( int sec = 0; sec < inCount; sec++ ){
            

            
            EightyWaity();
            
            for( int secByte = 0; secByte < 2048; secByte++ ){

                
                EightyWaity();
                
                inBuffer[ bufferOffset++ ] = pCDREG2;
                //*(char*)(inBuffer + bufferOffset) = pCDREG2;

            }
            
            // empty it!
            while( ( pCDREG0  & CDREG0_DATA_IN_DATAFIFO ) != 0 ){
                volatile int x = pCDREG2;
            }

            while( ( pCDREG0  & CDREG0_DATA_IN_RESPONSEFIFO ) != 0 ){
                volatile int x = CDReadResponse();
            }

            if ( sec != inCount -1 ){
                CDAck();
            }

        }
        
        // Stahp feeding it    
        CDSendCommand_Pause();
        CDAck();
        CDAck();
        
        return bufferOffset;

    }


#pragma endregion CD Logic





ulong CDCheckUnlocked() {

    int timeout = 0;
    ulong unlockStatus = 0;

    // western system; it's fine
    if ( DidNocashUnlock() || !IsROM() ) {
        NewPrintf( "Assuming CD unlocked for bootdisc...\n" );
        return 1;
    }

    // japanese system; check...
    while ( unlockStatus != FIFO_STAT_COMPLETE_3 ) {
        
        // just writes 0x01 for all 3 params - 0 doesn't work
        CDSendCommand_SetLoc( 1, 1, 1 );
        CDAck();
        
        CDStartCommand();
        CDWriteCommand( CD_CMD_READS );
        unlockStatus = CDAck();

        // CD not locked
        if (timeout++ > 10) {
            
            ClearTextBuffer();
            
            Blah("\n\n\n\n\n\n");
            Blah("        Waiting for the CD drive!\n");
            if ( IsJAP() && checkIfDriveUnlocked ){
                Blah("        Nocash unlock does not work on NTSC-J systems,\n");
            }
            Blah("\n");

            ClearAndDraw();

            return 0;
        }
    }

    return 1;
}



void DoNotCheckIfDriveUnlocked(){
    checkIfDriveUnlocked = 0;
}

void CDInitForShell(){
    
    if (!didCDInit) {

        didCDInit = 1;

        CDClearInts();

        pCDREG0 = 0;
        pCDREG3 = 0;
        pCOM_DELAY = 4901;

        CDSendCommand_GetStat();
        CDAck();
        CDSendCommand_GetStat();        
        CDAck();
        CDSendCommand_Init();        
        CDAck();

        // Main() may have already called it.
        if ( checkIfDriveUnlocked )
        CDNocash();
        
        // Works even on locked drives
        CDRemove();

        // For -J systems only
        if ( checkIfDriveUnlocked )
            CDCheckUnlocked();

        // We'll lock up here on a locked drive,
        // leaving the NTSC-J message showing.
        CDAdd();

    }
    
}



// LIMITATION: Root directory only
int CDOpenAndRead(const char *inFile, char * inBuffer, ulong numSectors){

    if (!InitBrowser()) {
        //Blah("\n        Error reading disc contents!");
        //HoldMessage();
        return -1;
    }
    
    int sector = GetSector( inFile );

    if ( sector <= 0 )
        return -1;

    if ( numSectors == 0 ){
        numSectors = GetSectorSize( inFile );
    }
    
    return CDReadSector( numSectors, sector, inBuffer );
    
};

// Rounds a size up to 2K for loading a sector.
ulong Pad2k(ulong inSize) {
    return (inSize + 0x7ff) & ~0x7ff;
}

// Can't read sectors till it's done.
void CDWaitReady(){
            
    CDSendCommand_SetSession( 1 );        
    
    CDAck();
    
    if ( lastInt == FIFO_STAT_COMPLETE_3 ){

        CDAck();
        
    }
    
    
}

//
// Borrowed this format from Nicolas Noble's syscalls =)
// https://github.com/grumpycoders/pcsx-redux/blob/master/src/mips/common/syscalls/syscalls.h
// Popped them down here so they dont' scare people off.
//

static ulong CDAdd() {
    register int cmd __asm__("t1") = 0x54;  // 71 seems more fussy
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int (*)(void))0xA0)();
}

static ulong CDRemove() {
    register int cmd __asm__("t1") = 0x56;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int (*)(void))0xA0)();
}

ulong CDFileClose(ulong inHandle) {
    register int cmd __asm__("t1") = 0x36;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int (*)(ulong))0xB0)(inHandle);
}

ulong CDFileOpen(const char *fileName, ulong accessMode) {
    register int cmd __asm__("t1") = 0x32;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int (*)(const char *, ulong))0xB0)(fileName, accessMode);
}

ulong CDFileRead(ulong fileHandle, ulong inDest, ulong inLength) {    
    register int cmd __asm__("t1") = 0x34;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int (*)(ulong, ulong, ulong))0xB0)(fileHandle, inDest, inLength);    
}

void CDLoadAndExecute(const char *fileName, ulong stackAddr, ulong stackOffset) {
    register int cmd __asm__("t1") = 0x51;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((void (*)(const char *, ulong, ulong))0xA0)(fileName, stackAddr, stackOffset);
}

ulong CDGetLbn(const char *fileName) {
    register int cmd __asm__("t1") = 0xA4;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((ulong(*)(const char *))0xA0)(fileName);
}

int x = 3;

ulong CDReadSector_KERNEL(ulong inCount, ulong inSector, ulong inBuffer) {
    
    // Bugged:
    // https://github.com/grumpycoders/pcsx-redux/blob/b5a88276efdd0cd4ae888586e3919515f7f50dea/src/mips/openbios/cdrom/helpers.c#L36

    register int cmd __asm__("t1") = 0xA5;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((ulong(*)(ulong, ulong, ulong))0xA0)(inCount, inSector, inBuffer);

}

// Format: "cdrom:\\SYSTEM.CNF;1"
// Format: "bu00:\\BESLUS_WHATEVER" (no ;1)
ulong CDFirstFile(const char *fileName, struct DirEntry *inEntry) {
    register int cmd __asm__("t1") = 0x42;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((ulong(*)(const char *, struct DirEntry *))0xB0)(fileName, inEntry);
}

ulong CDSeek( uchar * msf ){
    register int cmd asm("t1") = 0x78;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    return ((int(*)(uchar *))0xA0)(msf);
}


static void KException(int code1, int code2) {
    register volatile int cmd asm("t1") = 0xA1;
    __asm__ volatile("" : "=r"(cmd) : "r"(cmd));
    ((void(*)(int, int))0xA0)(code1, code2);
}

#if DEBUG_STUFF


    char CDReadResult( char * buff, ulong maxLength ) {
        
        char lastReadVal = 0;

        CDWaitInt();
        
        while ((pCDREG0 & CDREG0_DATA_IN_RESPONSEFIFO) != 0) {
            
            lastReadVal = CDReadResponse() & 0xFF;
            *buff++ = lastReadVal;

            ClearTextBuffer();        
            Blah( "Result %x %c", lastInt, lastReadVal );
            HoldMessage();

        }
        
        CDClearInts();
        
        return lastReadVal;

    }


    void DebugResults() {
        
        char buf[0x20];

        CDReadResult( buf, 20 );

        ClearTextBuffer();                
        Blah("String : was %s\n", buf);
        Blah("Hexa   : was %x,%x,%x,%x... \n", buf[0], buf[1], buf[2], buf[3]);
        HoldMessage();

    }

    static void GetRegionString() {
        CDStartCommand();
        CDWriteParam( 0x22 );
        CDWriteCommand( CD_CMD_TEST );
        DebugResults();
    }

    static void GetServoAmpID() {
        CDStartCommand();
        CDWriteParam( 0x23 );
        CDWriteCommand( CD_CMD_TEST );
        DebugResults();
    }

    static void GetSignalProcessorID() {
        CDStartCommand();
        CDWriteParam( 0x24 );
        CDWriteCommand( CD_CMD_TEST );
        DebugResults();
    }

    static void GetFifoID() {
        CDStartCommand();
        CDWriteParam( 0x25 );
        CDWriteCommand( CD_CMD_TEST );
        DebugResults();
    }





    void CDTestCode(){
        
        GetRegionString();
        return;
        
        int returnCode = CDReadSector( 2, 0x23, cGPBUFFER );

        ClearTextBuffer();
        Blah( "RC %x\n", returnCode );
        HoldMessage();
        
        
        return;

        ulong counter = 0;
        
        
        while ( 1 ){
            ClearTextBuffer();        
            
            CDSendCommand_SetLocLBA( 0x10 );
            CDAck();
            Blah( " x %x, %x\n ", lastInt, lastResponse );

            CDSendCommand_SeekL();
            CDAck();
            Blah( " x %x, %x\n ", lastInt, lastResponse );
            CDAck();
            Blah( " x %x, %x\n ", lastInt, lastResponse );

            CDSendCommand_ReadN();
            CDAck();
            Blah( " x %x, %x\n ", lastInt, lastResponse );
            CDAck();
            Blah( " x %x, %x\n ", lastInt, lastResponse );
            
            Blah( "CD %x\n" , counter++ );
            
            ClearAndDraw();

        }


    }




#endif  // DEBUG_STUFF

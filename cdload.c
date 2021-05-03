// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

//
// CDROM File handling and disc booting routines for Unirom 8
// Low level CD stuff can be found in cd.c
//

#include "cdload.h"

#include "cd.h"
#include "drawing.h"
#include "filebrowser.h"
#include "littlelibc.h"
#include "npal.h"
#include "utility.h"
#include "ttyredirect.h"

// Using the Cop0 hook
// Games which work:
// * = NTSC-PAL switch works
// ! = NTSC-PAL works, but has issues
// x = NTSC-PAL doesn't work
//   = Haven't tested

// * 40 Winks USA
//   A Bug's Life PAL
// * A Bug's Life NTSC
// * Alone In The Dark - Jack Is Whatever USA (needs switch)
// * Ape Escape USA
// * Ape Escape PAL (forgot libcrypt tho)
// * Battle Arena Toshinden USA
// * Battle Arena Toushinden JAP (Toshinden 4)
// * Colin McRae 2 USA
// * Crash 1 USA
// * Crash 2 USA
// * Critical Depth NTSC
// * Cool Boarders 1 USA - Works, but it's still shit.
// * Cool Boarders 3 USA - Wow, still shit!
//   Croc PAL
//   Croc 2 PAL
// * Croc 2 NTSC - bit flaky.. is it my disc?
// * Dave Mira freestyle BMX
// * Dave Mira Maximum BMX Freestyle Maximum BMX FreeMix Something
// * Destruction Derby 2 NTSC - Audio's so messed up
// ! Destruction Derby RAW USA  - switchy only?
// * Dino Crisis USA
// * Discworld USA
// * Dino Crisis PAL (forgot libcrypt tho)
// ! Driver USA (toggle after loading)
// ! Driver PAL (toggle after loading)
// * Elemental Gearbolt USA
// * Excalibur 2555 USA
//   Explosive Racing PAL
// * FireBugs PAL - It's already miscentered? Works tho.
// * Gran Turismo USA
//   Gran Turismo PAL
// * Harvest Moon - Back to Nature USA
// * Harmful Park JAP (misses one frame, calling it a win)
// * Heart of Darkness USA
// * Hello Kitty Cube Frenzy USA (switch)
// * Herc's Adventure USA
// * Hooters Road Trip USA (wtf, it's not even terrible...)
// * Hot Shots Golf USA
// * IQ Final JAP
//   Jumping Flash USA (fastload forced)
//   Max Power Racing PAL
// * Medievil Euro - didn't patch libcrypt but it boots
// * Medievil NTSC
//   Metal Gear Solid PAL
// * Metal Gear Solid USA - Not even w/ switch
// * Metal Gear Solid Integral
// ? Metal Slug X JP - partial boot, disk's wrecked
// * Metal Slug X USA
// * Mission Impossbile USA
//   Monster Trucks PAL
// * N20: Nitrous Oxide USA
// * Nagano Winter Olympics 98 (USA) Holy shit this is a funny game.
// * Need for Speed 1 USA
// * Need for speed 5 USA (fixed tail lights!)
// * Oddworld - Abe's Oddysee NTSC (Rev 2 with promo)
// * Parasite Eve USA - Kinda works, CBA rewatching the intro
// * Pipe Mania PAL
// * Porche Challenge PAL
// * Porche Challenge USA
// * Psychic Detective USA
// * Rapid Racer PAL
// * Rapid Reload PAL
// * Rayman USA
// * Rayman 2 USA
// * Res Evil 1 USA ( first door used to fail without switch, should be fine now )
// * Res Evil 2 DualShock
// * Ridge Racer Type 4
// * Road & Track Presents - The Need For Speed (USA)
//   Shendo's BIOS dumper
//   Silent Hill
//   Spyro PAL
//   Spyro 2 PAL
//   Spyro 3 (w/ PDX loader)
//   Super Metroid
//   Symphony Of The Night (CastleVania) PAL
// * Symphony Of The Night (CastleVania) NTSC
// * Syphon Filter 1 USA (fastload forced)
// * Syphon Filter 2 USA (fastload forced)
// * Syphon Filter 3 USA (fastload forced)
// * Team Buddies USA (Some music stuttering)
// ! Tekken 3 USA (Hi Res / Interlacing issues?)
// * Tony Hawk's Pro Skater 2
// * Tony Hawk's Pro Skater 3 USA
// * Tomb Raider 1 PAL
// * Tomb Raider 2 USA
// * Tomb Raider 3 Rev 3 USA (timesplitters n stuff)
// * Tomb Raider 3 PAL
//   Tomb Raider 4 PAL
// * Toy Story 2 PAL
// ! Twisted Metal 2 USA (can be flaky)
// ? Vigilante 8
// * Wild Thornberries USA

// Non-working titles:
//    Tomb Raider 3 PAL
//    Destruction Derby 3 RAW

// Still to test personally
//   Mortal Kombat Mythologies
//   Rollcage 2 hangs
//   Grind Session
//   TR5
//   Fear Effect 1
//   Colony Wars 3
//   G-Police 2


// Menu doesn't take params...
void BootNormal() { CDLoad(0); }
void BootAutoNPAL() { CDLoad(CDLOAD_NPAL_AUTO | CDLOAD_PAL); }
void BootManualNPAL() { CDLoad(CDLOAD_NPAL_MANUAL | CDLOAD_PAL); }
void BootManualNTSC() { CDLoad(CDLOAD_NPAL_MANUAL); }
void BootAutoNTSC() { CDLoad(CDLOAD_NPAL_AUTO); }


static ulong GetStack( char* filename ){

    ulong defaultAddr = 0x801FFF00;

    // Remember to compare against UPPERCASE names

    NewPrintf( "Filename %s\n ", filename );

    // Destruction Derby RAW is the only game requiring
    // a parse of the SYSTEM.CNF's "stack" param
    // so there's no point building one to potentially
    // introduce errors.
    if ( NewStrcmp( filename, "cdrom:\\SCES_020.60"  ) == 0 ){
        NewPrintf( "Patch for DDRaw Euro\n" );
        return 0x80022FFC;
    }

    if ( NewStrcmp( filename, "cdrom:\\SLUS_009.12"  ) == 0 ){
        NewPrintf( "Patch for DDRaw USA\n" );
        return 0x80022FFC;
    }
    
    // The unirom bootdisc up to 8.0.bB will remove and re-add the 
    // TTY in the wrong order, causing the kernel to call
    // a remove function that may no longer exist.
    // so we'll remove TTY and let it add itsself    
    if ( NewStrcmp(  (char*)filename, "cdrom:\\UNIROM_B.EXE" ) ==0 ){        
        RemoveTTY();        
        Blah("        Removed TTY device...\n");
        ClearAndDraw();
    }

    return defaultAddr;

}


// todo: remove the old boot stuff
// jumpaddr, checksum, etc
int CDLoad(int inBootOptions) {
    // Pointer to the string containing the .exe file name
    // e.g. inside SYSTEM.CNF
    char *fileName;

    // Result byte used for many functions
    char result;

    // General purpose counter
    int i;

    // checksum to determine games which *must* use fastboot
    // note: very weak checksum, might need changing
    //ulong checksum = 0;

    // Where the .exe header will be written
    // NOTE: sys.cnf is loaded to a GPBUFFER instead
    ulong writeBuffer;

    // Values from the header
    // We'll set these before launching the .exe
    ulong jumpAddr = 0;
    ulong writeAddr = 0;
    ulong stackAddr = 0;
    ulong gpVal = 0;

    // Where does the game start on disc?
    // And what sector are we currently reading?
    ulong startSector = 0;
    ulong currentSector = 0;
    ulong async_totalSectors = 0;
    ulong async_sectorsRead = 0;
    

    if (!InitBrowser()) {
        Blah("\n        Error reading disc contents!");
        HoldMessage();
        return -1;
    }

    if (IsJAP()) {
        Blah("        Skipping nocash unlock...\n");
    } else {
        Blah("        Applied nocash unlock...\n");
    }
    ClearAndDraw();

    i = GetSector("SYSTEM.CNF;1");

    if (i == -1) {
        i == GetSector("system.cnf;1");
    }
    

    if (i != -1) {
        // =====================================
        // PARSE SYSTEM.CNF
        // =====================================

        CDReadSector(1, i, cGPBUFFER);

        Blah("        Loaded SYSTEM.CNF from sector 0x%x\n", i);
        ClearAndDraw();

        memset( (char*)CDFILENAMEBUFFER, 0x00, 0x20 );
        
        // Search for the first ":" (some CNFs have spaces)
        // note: Mr Domino has the filename at the end... 
        for (i = GPBUFFER; i < (GPBUFFER + 60); i++) {
            // filename starts after the colon
            // slashes aren't always present
            if (*(char *)i == ':') {
                // start of the filename
                fileName = ((char *)(i + 1));

                // now find the end ( either CR or LF)
                while (*(char *)i != 0x0D && *(char *)i != 0x0A) {
                    // convert to upper case.
                    if (*(char *)i >= 'a' && *(char *)i < 'z') {  // 0x61-0x7A
                        *(char *)i ^= 0x20;
                    }

                    i++;
                }
                *(char *)i = 0;
                break;
            }
        }

        // Some things will be like cdrom:\XFLASH.EXE;1 but some disks
        // have e.g. cdrom:DUMPER.EXE;1 without the slash.
        // in that case, just turn the colon into a slash
        if (*(char *)fileName != '\\') {
            fileName--;
            *(char *)fileName = '\\';
        }
        
        NewSPrintf((char *)CDFILENAMEBUFFER, "cdrom:%s", fileName);  // prepend the filename with "cdrom:" again

    } else {
        NewSPrintf((char *)CDFILENAMEBUFFER, "cdrom:\\PSX.EXE;1");
    }

    // =====================================
    // FASTLOAD - shell handles it
    // =====================================
    

    // finish at the ;1
    // including ;1 works for every game but toushinden 4
    
    for (i = CDFILENAMEBUFFER; i < CDFILENAMEBUFFER + 30; i++) {
        if (*(char *)i == ';') {
            *(char *)(i) = 0;
            break;
        }
    }
    

    Blah("        FastLoad Locating: %s\n", CDFILENAMEBUFFER);
    ClearAndDraw();


    if (inBootOptions & CDLOAD_NPAL_AUTO) {
        storeinitialswitchstate();
        installgpuhook(1, inBootOptions & CDLOAD_PAL);
    }

    if (inBootOptions & CDLOAD_NPAL_MANUAL) {
        storeinitialswitchstate();
        installgpuhook(0, inBootOptions & CDLOAD_PAL);
        //patchb0();
    }


    stackAddr = GetStack( (char*)CDFILENAMEBUFFER );
    
    // For the shell
    CDInitForShell();

    NewPrintf( "Final filename: %s\n", CDFILENAMEBUFFER );
    
    // Blank the VRAM so stray blue pixels don't mess with scalers
    BlankVRAM();

    CDLoadAndExecute((char *)CDFILENAMEBUFFER, stackAddr, 0x0);

    // Reset if something fucks up?
    goto *(ulong *)0xBFC00000;

}

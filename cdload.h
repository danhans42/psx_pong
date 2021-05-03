// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

//#define CDDEBUG 1

// Note: changing the value of CDLOAD_PAL will require some asm-level changes
// as it's expecting the value to be 1 for PAL and 0 for NTSC like Sony's Libs

#define CDLOAD_PAL 0x01
#define CDLOAD_NPAL_MANUAL 0x08
#define CDLOAD_NPAL_AUTO 0x10
#define CDLOAD_FAST 0x100

#define CDREAD_BLOCKING 0
#define CDREAD_NONBLOCKING 1

int CDLoad(int inBootOptions);

// Menu doesn't take params...
void BootNormal();
void BootAutoNPAL();
void BootManualNPAL();
void BootManualNTSC();
void BootAutoNTSC();

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define DEBUG_STUFF 0
#define CD_DEBUG 0

#define GPBUFFER 0x80080000
#define pGPBUFFER *(ulong*)GPBUFFER
#define cGPBUFFER (uchar*)GPBUFFER
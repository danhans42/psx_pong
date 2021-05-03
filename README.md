
## "PSX PONG"

http://psx0.wordpress.com/ 

http://www.github.com/danhans42/

### About

My first ever PlayStation game.

Its very simple, probably poorly written but was written quickly as a simple example of how to throw something together.

It uses one pad for both players. UP/DOWN are player 1, CIRCLE/X are player 2.

I will probably get around to adding difficulty/two pad support/hiscore/sound etc.

### Compiling the source

I am only covering Ubuntu/Mint Linux (or WSL) but should be fairly straightforward on other Linuxes etc.

1. Clone this repo 
2. grab gcc-10-mipsel-linux-gnu using  "sudo apt install gcc-10-mipsel-linux-gnu"
3. type make

I probably need to double check this..

### Running on a Console/Emulator

To run this on a console you would need to transfer the EXE file over to the PSX somehow. You can do this old school with the cheat cart & comms routes (xplorer/AR&GS), ROM replacements for the aforementioned such as Unirom/n00brom/Caetla or even a unirom running via FreePSXBoot and a serial cable. 

To run pong using unirom and a serial cable type "nops /exe psxpong.exe /dev/ttyUSB0", changing the serial port as necessary for your setup.

To run pong on an emulator use the load EXE function in your emulator.

### Thanks & Links

This was written using the helloworld_and_flappycredits Pad/GPU example by Sicklebrick as a base, 
https://github.com/JonathanDotCel/helloworld_and_flappycredits

Using the following Pong game logic article by William Doane,
https://drdoane.com/thinking-through-a-basic-pong-game-in-processing/

Shouts, thanks and greets to everyone on psxdev forums/discord

For more PSX related chat join us on the #psxdev server.


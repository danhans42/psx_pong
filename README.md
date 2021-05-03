
## PSX PONG

![Main Screen](https://github.com/danhans42/psx_pong/blob/main/images/screenshot1.png)

A very simple version of pong for the Sony Playstation.

### Info

Simple example of a making a game. It was written quickly as an example of how to throw something together on the psx quickly.

![Game Screen](https://github.com/danhans42/psx_pong/blob/main/images/screenshot2.png)

It uses one pad for both players. UP/DOWN are player 1, Triangle/X are player 2. First to 10 wins.

I will probably get around to adding difficulty/two pad support/hiscore/sound etc.

### Compiling the source

Using Ubuntu/Mint Linux.. 

1. Clone this repo 
2. grab gcc-10-mipsel-linux-gnu using  "sudo apt install gcc-10-mipsel-linux-gnu"
3. cd to psx_pong
4. type make
5. You should then have a pong.ps-exe in your folder

### Running on a Console/Emulator

To run this on a console you would need to transfer the EXE file over to the PSX somehow. You can do this old school with the cheat cart & comms routes (xplorer/AR&GS), ROM replacements for the aforementioned such as Unirom/n00brom/Caetla or even a unirom running via FreePSXBoot and a serial cable. 

To run psxpong using unirom and a serial cable type "nops /exe pong.exe COM3", changing the serial port as necessary for your setup. Note: If you are using nops on linux you will need to install mono.

To run pong on an emulator use the load EXE function in your emulator.

### Links/Credit/Thanks

This was written using the helloworld_and_flappycredits Pad/GPU example by Sicklebrick as a base :-
https://github.com/JonathanDotCel/helloworld_and_flappycredits

Using the following Pong game logic article by William Doane :-
https://drdoane.com/thinking-through-a-basic-pong-game-in-processing/

The font used was made using one of the ZX Origins fonts by damieng :- 
https://damieng.com/typography/zx-origins/ 
However I cant remember which one I used :)

pong_packed is a UPX compressed version . :- 
https://upx.github.io/

For more PSX related chat join us on the #psxdev discord server.

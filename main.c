//
// "PONG"
// by @danhans42
// http://psx0.wordpress.com/ 
// http://www.github.com/danhans42/
//
// My first ever PlayStation game Yay! Cant take all the credit though...
// 
// This was written using the helloworld_and_flappycredits Pad/GPU example 
// as a base, and using the info from the following links :-
//
// https://github.com/JonathanDotCel/helloworld_and_flappycredits
// https://drdoane.com/thinking-through-a-basic-pong-game-in-processing/
// https://github.com/grumpycoders/pcsx-redux/ 
//
// For more PSX related chat join us on the #psxdev discord
//
// ------ Original Helloworld_and_flappycredits Licence Text Below --------
//
// https://github.com/JonathanDotCel
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
// 
// Standard kinda fare though, no explicit or implied warranty.
// Any issues arising from the use of this software are your own.
// 
// https://github.com/JonathanDotCel
//
// To do List Features
// 		
// [X] Add Second Player on same pad
// [ ] Alternate ball serve direction?
// [X] Scoring
// [X] Paddle binding to court
// [ ] Sound - Gotta be old school Spectrumesque Beeps - or shitter
// [ ] Instructions/button mapping on main screen
// [ ] Arkanoid styleee breakout mode?
//


#ifndef  MAINC
#define MAINC

#include <stdarg.h>
#include <stdbool.h>

#include "main.h"
#include "littlelibc.h"
#include "utility.h"
#include "drawing.h"
#include "pads.h"
#include "gpu.h"



//Screens
void MainScreen();
void GameScreen();
void GameOver();

// Graphic Elements
void DrawGameArea();
void DrawLogo();

void DrawBall();

// Stuff
void DoStuff();
bool Collision();

int p1_score, p2_score, p1_bat;

short blockX = SCREEN_WIDTH / 2;
short blockY = SCREEN_HEIGHT / 2;

short BallX=256;
short BallY=120;
short LimitX=40;
short LimitY=34;
short WidthX=464;
short HeightY=200;
short dX = 2;
short dY = 2;

short p1_PaddleY=100;
short p1_PaddleX=46;

short p2_PaddleY=100;
short p2_PaddleX=458;


short PaddleWidth = 40;

int main(){
	// Sets shit up
	ExitCritical();
	InitBuffer();
	InitGPU();
	InitPads();
	// Main Screen
	MainScreen();

}



void DoStuff(){

	if ( Released( PADRdown ) ){
		// go to start of game
	}

	if ( Released( PADselect ) ){
		// restart via bios
		goto *(ulong*)0xBFC00000;
	}	
	
}


void DrawBall() {

	DrawTile (BallX,BallY,8,6,0xFFFFFF);

}

void DrawGameArea() {

		// Colours can be changed in main.h
		DrawTile(38,30,436,180,CLR_BORDER); 	// Draw Border
		DrawTile(40,32,432,176,CLR_COURT); 		// Draw Court
}

void DrawLogo() {
		//Draws the word PONG - 1979 Styleee - Yeeeaaaah Baby!!

		DrawTile(98,54,16,60,CLR_LOGO); 		// Letter P
		DrawTile(146,54,16,36,CLR_LOGO);
		DrawTile(114,54,48,12,CLR_LOGO);
		DrawTile(114,78,48,12,CLR_LOGO);		
		
		DrawTile(178,54,16,36,CLR_LOGO);		// Letter O
		DrawTile(178,54,50,12,CLR_LOGO);
		DrawTile(178,78,50,12,CLR_LOGO);
		DrawTile(228,54,16,36,CLR_LOGO);
		
		DrawTile(260,54,16,36,CLR_LOGO);		// Letter N
		DrawTile(260,54,50,12,CLR_LOGO);
		DrawTile(310,54,16,36,CLR_LOGO);
		
		DrawTile(390,54,16,60,CLR_LOGO);		// Letter G
		DrawTile(342,54,50,12,CLR_LOGO);
		DrawTile(342,54,16,36,CLR_LOGO);
		DrawTile(342,78,50,12,CLR_LOGO);
		DrawTile(342,102,50,12,CLR_LOGO);

	}


void MainScreen() {

	while ( 1 ){

		MonitorPads();
		ClearScreenText();
		DrawBG();
	
		Blah("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
		Blah("                      Press Start To Play\n\n\n\n\n");
		Blah("            http://www.github.com/danhans42/psx_pong \n");
		DrawGameArea();
		DrawLogo();		

		if ( Released( PADstart ) ){
			GameScreen();
		}

		if ( Released( PADselect ) ){
			// restart via bios
			goto *(ulong*)0xBFC00000;
		}	
		Draw();
	}

}

void GameScreen() {

	// Put the ball in the middle of the court
	p1_score = 0;
	p2_score = 0;
		
	while (1) {
		ClearScreenText();
		MonitorPads();
		DrawBG();
		Blah                  ("\n\n\n\n\n                              %i  %i   \n",p1_score,p2_score);
		
		DrawGameArea();

		// Draw Court Divider
		DrawTile(256,32,2,11,CLR_BORDER);
		DrawTile(256,61,2,11,CLR_BORDER);
		DrawTile(256,88,2,11,CLR_BORDER);
		DrawTile(256,115,2,11,CLR_BORDER);
		DrawTile(256,142,2,11,CLR_BORDER);
		DrawTile(256,169,2,11,CLR_BORDER);
		DrawTile(256,197,2,11,CLR_BORDER);	
		DrawBall();

		// Checks if the paddles are off the top the court area, if not clip them back in

		if (p1_PaddleY < LimitY+2 ) p1_PaddleY = LimitY;
		if (p2_PaddleY < LimitY+2 ) p2_PaddleY = LimitY;
		
	
		// Checks if the paddles are off the bottom the court area, if not clip them back in
		// (same thing essentially, but we need to take paddleX size into consideration)

		if (p1_PaddleY+PaddleWidth > HeightY ) p1_PaddleY = (HeightY+6)-PaddleWidth;
		if (p2_PaddleY+PaddleWidth > HeightY ) p2_PaddleY = (HeightY+6)-PaddleWidth;



		// Handles if the ball runs off the court on P1 side - Point for Player 2
		if (BallX < p1_PaddleX) {
			p2_score=p2_score+1;
			BallX=256;
			BallY=120;
  		}
		// Handles if the ball runs off the court on P2 side - Point for Player 1
		if (BallX > p2_PaddleX) {
			p1_score=p1_score+1;
			BallX=256;
			BallY=120;
  		}

		// Handles paddle collision
		if (Collision()) {
   			dX = -dX; // if dX == 2, it becomes -2; if dX is -2, it becomes 2
 		}


		// Handles ball bouncing off the court sides - should be self explanatory
		if (BallX > WidthX) {  
  			dX = -dX; 
 		}
		if (BallX < LimitX) {
   			dX = -dX;
  		}
		if (BallY > HeightY) {
   			dY = -dY; 
 		}
 		if (BallY < LimitY) {
    		dY = -dY; 
  		}

		// Increment the movement
		BallX = BallX + dX;
		BallY = BallY + dY;

		// Draw P1 Paddle
		DrawTile(p1_PaddleX,p1_PaddleY,8,PaddleWidth,0xffffff);

		// Draw P2 Paddle
		DrawTile(p2_PaddleX,p2_PaddleY,8,PaddleWidth,0xffffff);

		// Draw the screen
		Draw();

		//Process Input - also clips the paddle to the court area

		// move P1 Paddle Down
		if ( Released( PADLdown ) ){
			p1_PaddleY = p1_PaddleY+6;
		}
		
		// move P1 Paddle Up
		if (Released( PADLup ) ){
			p1_PaddleY = p1_PaddleY-6;
		}

		// move P2 Paddle Up
		if (Released( PADRup ) ){
			p2_PaddleY = p2_PaddleY-6;
		}

		// move P2 Paddle Down
	
		if ( Released( PADRdown ) ){
			p2_PaddleY = p2_PaddleY+6;
		}

		// Check scores to see if a player has won - first to 8

		if (p1_score >7 | p2_score >7 ) GameOver();
		
	}
}


void GameOver() {

	while(1) {
		ClearScreenText();
		DrawGameArea();
		MonitorPads();
		Blah       ("\n\n\n\n\n\n                          GAME OVER\n\n\n\n\n\n\n");


		if (p1_score > p2_score) {
			Blah("                        Player 1 WINS");
		}		

		if (p2_score > p1_score) {
			Blah("                        Player 2 WINS");
		}

		Blah ("\n\n\n\n\n\n\n\n                  Press Start to Play Again\n\n");
		Blah ("                      or O for Main Menu     \n");
		Draw();

		if ( Released( PADstart ) ){
			GameScreen();	
		}

		if ( Released( PADRright ) ){
			MainScreen();	
		}

	}
}

bool Collision() {

	bool returnValue = false;
	// player 1
  	if ((BallX >= p1_PaddleX+8) && (BallX <= p1_PaddleX +8)) {
    	if ((BallY >= p1_PaddleY) && (BallY <= p1_PaddleY + PaddleWidth)) {
      		returnValue = true;
    	}
  	}
	// player 2
  	if ((BallX >= p2_PaddleX-6) && (BallX <= p2_PaddleX-6 )) {
    	if ((BallY >= p2_PaddleY) && (BallY <= p2_PaddleY + PaddleWidth)) {
      		returnValue = true;
    	}
  	}
	return returnValue;
}

#endif 


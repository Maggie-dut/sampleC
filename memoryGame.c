/*
This was an end-of-term bonus project for a programming class.
The GUI ( continueSuperLoop(), setupDAQ() digitalWrite(), digitalRead() ) was provided by the professor; essentially it consisted of 4 input buttons, each associated with an output light
The goal of the project was to build a simple memory game.
Game rules:
A random sequence of lights is played; the user then has to copy the random series to the input keys.
The sequence starts at length 1 on round 1, for every subsequent round in the same game, the pattern increases by 1 digit. If the player succeeds for 5 rounds, they win the game.
If the player makes a mistake, then the correct light will flash rapidly then the game ends.
If the player completes LIMIT rounds without a mistake, the green light will flash 3 times slowly.
If the player loses the game, the red light will flash 3 times slowly.
Then the player is asked if they want to play again,
if yes, the game restarts with a different random pattern.
if no, program ends.
If player exists simulator at any time, an error message will appear and game will terminate.
Note: time delays are not standardised with symbolic constants as the duration of time delays vary widely within aspects of the program.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <DAQlib.h>
#include <Windows.h>
#include <time.h>

#define LIMIT 5				//limit of number of rounds in a game
#define SIMULATION 6		//which parameter is used by setupDAQ()
#define NUM_LIGHTS 4		//number of LEDs in simulator
#define SENTINEL -1			//used to break loops

#define TRUE 1
#define FALSE 0

#define RED 1				//channel for red light
#define GREEN 0				//channel for green light

int playTheGame(void);
void setRandomLights(int, int);
int readUserInput(int, int);
void lightOne(int);
void wrongLight(int);
int getLightNumber(int, int);
void endGame(int);


int main(void) {
	//seed random number generator
	srand((unsigned)time(NULL));
	int playAgain = TRUE;

	if ( setupDAQ(SIMULATION) == TRUE) {
		while (continueSuperLoop() == TRUE && playAgain == TRUE) {
			//play a game
			if (playTheGame() == TRUE) {
				printf("do you want to play again? (1 for yes, 0 for no.)\n");
				scanf("%d", &playAgain);
				//displays error message if player closed simulation
				if (continueSuperLoop() == FALSE) {
					printf("Error: simulator has been disconnected.\n");
				}
				//give player a second to prepare for next round
				Sleep(1000);
			}
		}
	}
	else {
		printf("Error starting DAQ simulation. Sorry!\n");
	}
	system("PAUSE");
	return 0;
}

/*
general outline for 1 game of TheGame.
Returns:	TRUE for DAQ simulator still running, SENTINEL for DAQ has been disconnnected
*/
int playTheGame(void) {
	//initial pattern is 1 light
	int patternLength = 1;
	//to save memory, pattern to store LEDs is base 4 (saved to a normal int) instead of an array to take less memory space
	int pattern = 0;
	int nextLight;
	int userInput;
	int winStatus = GREEN;
	//each time through the loop is 1 round
	while (patternLength <= LIMIT) {
		//left-shift pattern, generate new digit randomly, add a new digit to pattern
		pattern *= NUM_LIGHTS;
		nextLight = rand() % NUM_LIGHTS;
		pattern += nextLight;
		//display LEDs for this round
		setRandomLights(pattern, patternLength);
		//confirms that DAQ is still running.
		if (continueSuperLoop() == FALSE) {
			printf("Error: simulator has been disconnected.\n");
			return SENTINEL;
		}
		//take in input from player; end game if loss or the DAQ is no longer working
		userInput = readUserInput(pattern, patternLength);
		if (userInput == FALSE) {
			winStatus = RED;
			break;
		}
		else if (userInput == SENTINEL) {
			printf("Error: simulator has been disconnected.\n");
			return SENTINEL;
		}
		//sequence gets 1 digit longer for new round
		patternLength++;
	}
	endGame(winStatus);
	return TRUE;
}

/*		Lights up LEDs at the beginning of the round
Parameters:		pattern = quatenary (base 4) number describing random pattern of light
count = length of pattern
*/
void setRandomLights(int pattern, int count) {
	int nextLight;
	//each time through the loop is 1 light
	while (count > 0) {
		//current light number has to be extracted from the pattern
		nextLight = getLightNumber(pattern, count);
		lightOne(nextLight);
		count--;
		//confirms that DAQ is still running.
		if (continueSuperLoop() == FALSE) {
			break;
		}
	}
}

/*		reads which buttons user presses
Parameters:		pattern = quatenary number describing random pattern of light
count = length of pattern
Return value:	success = whether player won round (TRUE if yes; FALSE if no)
*/
int readUserInput(int pattern, int count) {
	int nextLight;
	int input;
	int terminal;
	int success = TRUE;
	//each time through loop is validating 1 button press
	while (count > 0) {
		nextLight = getLightNumber(pattern, count);
		input = SENTINEL;
		//check all 4 buttons (each time through loop checks 1 button)
		for (terminal = 0; terminal < NUM_LIGHTS; terminal++) {
			//read at terminal button - while loop to address sticky keys problem
			//if that button isn't being pressed, then it will never enter while loop
			while (digitalRead(terminal) == TRUE) {
				input = terminal;
			}
			//if that key is pressed (if it entered the while loop), break
			if (input != SENTINEL) {
				break;
			}
			//if none of the keys were pressed, mark as -1 (restart loop). 
			if (input == SENTINEL && terminal == NUM_LIGHTS - 1) {
				terminal = SENTINEL;
			}
			//if DAQ breaks, return error
			if (continueSuperLoop() == FALSE) {
				return SENTINEL;
			}
		}
		//check for error
		if (terminal != nextLight) {
			wrongLight(nextLight);
			success = FALSE;
			break;
		}
		count--;
	}
	return success;
}

/*
lights one light, then turns it off
Parameters:		light = which light to turn on
*/
void lightOne(int light) {
	Sleep(500);
	digitalWrite(light, TRUE);
	Sleep(1000);
	digitalWrite(light, FALSE);
}

/*
flashes the correct light if player makes a mistake
Parameter:		light = which light was the correct one and therefore flashes
*/
void wrongLight(int light) {
	int count;
	for (count = 0; count < 10;count++) {
		digitalWrite(light, TRUE);
		Sleep(50);
		digitalWrite(light, FALSE);
		Sleep(50);
	}
}

/*
determines the next light in the pattern based off of quatenary number
Parameters:		pattern = quatenary number used to store random series of lights to light up
count = the nth light in the pattern
Return value:	value of light at count value in series
*/
int getLightNumber(int pattern, int count) {
	int getLight;
	for (getLight = count; getLight > 1; getLight--) {
		pattern /= NUM_LIGHTS;
	}
	pattern %= NUM_LIGHTS;
	return pattern;
}

/*
flashes slowely 3 times at increasing speed to be distinctive from wrongLight()
GREEN for win or RED for loss depending on whether player won or lost the round
Parameter:		light = which light (GREEN or RED) to flash
*/
void endGame(int light) {
	int count;
	for (count = 1;count <= 3;count++) {
		digitalWrite(light, TRUE);
		Sleep(200 * count);
		digitalWrite(light, FALSE);
		Sleep(300);
	}
}
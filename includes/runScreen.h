/**
 * File: runScreen.h
 * @author Jace Ferguson
 *
 * Function declarations for the runScreen.c file.
 *
 * runScreen.c builds and manages the timer functionality of the
 * app.
 *
 */
#ifndef INTERVAL_RUN_SCREEN_H
#define INTERVAL_RUN_SCREEN_H

void activateRunMode();
void deinitRunScreen();
void doVibrate();
void initRunScreen();
bool isRunning();
void skipToNextInterval();
void skipToPrevInterval();
void toggleRunning();
void tick();
void updateRunTimeScreen();
void vibrate();

#endif

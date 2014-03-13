/**
 * File: timeSetScreen.h
 * @author Jace Ferguson
 *
 * Function declarations for the timeSetScreen.c file.
 *
 * timeSetScreen.c builds the screen and manages the setting of
 * the interval time for each interval set.
 *
 */
#ifndef TIME_SET_SCREEN_H
#define TIME_SET_SCREEN_H
#include "../includes/types.h"

void adjustIntervalSetTime(int8_t change);
void changeUnit();
void deinitTimeSetScreen();
void initTimeSetScreen();
void updateSetTimeScreen();
void flashUnit();

#endif

/**
 * File: intervalSetScreen.h
 * @author Jace Ferguson
 *
 * Function declarations for the intervalSetScreen.c file.
 *
 * intervalsSetScreen.c is used to build and display the screen
 * that lets you set the number of intervals in the session.
 *
 */
#ifndef INTERVAL_SET_SCREEN_H
#define INTERVAL_SET_SCREEN_H

void adjustIntervals(int8_t change);
void deinitIntervalSetScreen();
void initIntervalSetScreen();
void setIntervalCountTxt(uint16_t count);

#endif

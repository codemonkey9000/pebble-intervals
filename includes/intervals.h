/**
 * File: intervals.h
 * @author Jace Ferguson
 *
 * Function declarations for the intervals.c file.
 *
 * intervals.c runs the show. It manages interaction with
 * buttons and setting up the window.
 *
 */
#ifndef _INTERVALS_H
#define _INTERVALS_H
#include "../includes/types.h"


void button_pressed_down(ClickRecognizerRef recognizer);
void button_pressed_up(ClickRecognizerRef recognizer);
void click_provider(Window *window);
void formatTime(uint16_t time, char *timeStr, bool setHours);
SettingsState getCurrState();
uint16_t * getIntervals();
uint8_t getIntervalCount();
uint8_t getCurrIntervalSetIdx();
char * getTimeTitleStr();
void handle_deinit();
void handle_init();
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed);
void handle_timer_event();
int main();
void nextState();
void prevState();
void runApp();
void select_double_press(ClickRecognizerRef rec);
void select_long_press(ClickRecognizerRef rec);
void select_pressed(ClickRecognizerRef rec);
void setIntervalCount(uint8_t ct);

#endif

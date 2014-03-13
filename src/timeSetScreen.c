/**
 * File: timeSetScreen.c
 * @author Jace Ferguson <fjace05@gmail.com>
 *
 * Last Update March 2014'
 *
 * This "screen" is used to set the time for each interval. The interval count is
 * set in the previous step. The time for each interval is simply an integer count
 * of seconds stored in the intervals array. Each interval can only be set for a
 * maximum of 59 minutes and 59 seconds.
 *
 */
#include <pebble.h>
#include <pebble_fonts.h>
#include "../includes/types.h"
#include "../includes/intervals.h"
#include "../includes/timeSetScreen.h"

//Reference to this layer pointer from intervals.c
extern Layer *setTimeLayer;

//Visual elements for this layer
TextLayer *setTimeModeText, *setTimeTitle, *colonString, *minStr, *secStr;

//A flag to indicate what unit of time we're adjusting.
SettingsUnit setting_unit = SETTING_MINUTE;

//This flag is used to know what color to set the selected unit so it looks like
//it is flashing.
bool unitShown = true;

//Text strings used through this screen.
char minStrTxt[3] = "00";
char secStrTxt[3] = "00";
char fullText[9];

/**
 * Change the amount of seconds the currIntervalSetIdx
 * interval timer will be set for.
 *
 * @param unit - is the unit we are setting currently.
 * @param change - can be positive or negative and specifies * how much to change the interval
 */
void adjustIntervalSetTime(int8_t change)
{
	uint16_t newTime;
	uint16_t *intervals = getIntervals();
	uint8_t currIntervalSetIdx = getCurrIntervalSetIdx();

	/*
	 If we're adjusting minutes we need to multiple change by 60
	 since time is stored in the array as seconds.
	 */
	if (setting_unit)
	{
		newTime = intervals[currIntervalSetIdx] + change;
	}
	else
	{
		newTime = intervals[currIntervalSetIdx] + (change * 60);
	}

	//An arbitrary limit of one hour per timer.
	if (newTime > 0 && newTime < 3600)
	{
		intervals[currIntervalSetIdx] = newTime;
		//Update the screen.
		updateSetTimeScreen();
	}
	else if (change < 0)
	{
		//The user tried to zero out the timer.
		intervals[currIntervalSetIdx] = 0;
		//Update the screen
		updateSetTimeScreen();
	}
} //End adjustIntervalSetTime

/**
 * Change the unit we're currently setting.
 * Also make sure whatever we had been previously
 * setting gets set to black so it doesn't disappear.
 */
void changeUnit()
{
	//Make sure we don't accidentally leave one white and unreadable.
	text_layer_set_text_color(minStr, GColorBlack);
	text_layer_set_text_color(secStr, GColorBlack);
	//Change the unit
	setting_unit = !setting_unit;
} //End changeUnit

/**
 * Clean up this layer
 */
void deinitTimeSetScreen()
{
	text_layer_destroy(setTimeModeText);
	text_layer_destroy(setTimeTitle);
	text_layer_destroy(colonString);
	text_layer_destroy(minStr);
	text_layer_destroy(secStr);
}

/**
 * Make the unit flash! This function is
 * called from a timer handler.
 */
void flashUnit()
{
	GColor newColor;
	//Determine the color to set
	if (unitShown)
	{
		newColor = GColorWhite;
	}
	else
	{
		newColor = GColorBlack;
	}
	unitShown = !unitShown;

	//Determine the unit whose color should change.
	if (setting_unit == SETTING_MINUTE)
	{
		text_layer_set_text_color(minStr, newColor);
	}
	else
	{
		text_layer_set_text_color(secStr, newColor);
	}
	//Reload the timer
	app_timer_register(150, (AppTimerCallback) handle_timer_event, NULL);
}

/**
 * Build this screen.
 */
void initTimeSetScreen()
{
	layer_set_hidden(setTimeLayer, true);

	//Current layer mode text
	setTimeModeText = text_layer_create(GRect(0, 15, 144, 40));
	text_layer_set_text(setTimeModeText, "Set Mode");
	text_layer_set_font(setTimeModeText,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(setTimeModeText, GTextAlignmentCenter);
	layer_add_child(setTimeLayer, text_layer_get_layer(setTimeModeText));

	//Setting interval counter
	setTimeTitle = text_layer_create(GRect(0, 40, 144, 40));
	text_layer_set_text(setTimeTitle, "Interval 00");
	text_layer_set_font(setTimeTitle,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(setTimeTitle, GTextAlignmentCenter);
	layer_add_child(setTimeLayer, text_layer_get_layer(setTimeTitle));

	//Create the three layers for displaying time.
	//This one is for the colon
	colonString = text_layer_create(GRect(0, 80, 144, 50));
	text_layer_set_text(colonString, ":");
	text_layer_set_font(colonString,
			fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(colonString, GTextAlignmentCenter);
	layer_add_child(setTimeLayer, text_layer_get_layer(colonString));

	//Minutes
	minStr = text_layer_create(GRect(0, 80, 70, 50));
	text_layer_set_font(minStr, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(minStr, GTextAlignmentRight);
	layer_add_child(setTimeLayer, text_layer_get_layer(minStr));

	//Seconds
	secStr = text_layer_create(GRect(74, 80, 70, 50));
	text_layer_set_font(secStr, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(secStr, GTextAlignmentLeft);
	layer_add_child(setTimeLayer, text_layer_get_layer(secStr));
}

/**
 * Update the screen during time set mode. This is a weird one.
 */
void updateSetTimeScreen()
{

	uint16_t *intervals = getIntervals();
	uint8_t currIntervalSetIdx = getCurrIntervalSetIdx();
	char * setTimeTitleStr = getTimeTitleStr();


	//Get the time string for the amount of seconds for the unit we're setting
	formatTime(intervals[currIntervalSetIdx], fullText, false);

	/**
	 Because the units flash, they are actually two separate text layers.
	 So the minutes and seconds are extracted from the string filled in the formatTime function
	 and placed into other variables so they can be updated separately.
	 **/
	minStrTxt[0] = fullText[0];
	minStrTxt[1] = fullText[1];
	minStrTxt[2] = 0;

	secStrTxt[0] = fullText[3];
	secStrTxt[1] = fullText[4];
	secStrTxt[2] = 0;


	//Update the interval count label.
	if (currIntervalSetIdx + 1 < 10)
	{
		setTimeTitleStr[9] = currIntervalSetIdx + 1 + 48;
		setTimeTitleStr[10] = 0;
	}
	else
	{
		setTimeTitleStr[9] = (currIntervalSetIdx + 1) / 10 + 48;
		setTimeTitleStr[10] = (currIntervalSetIdx + 1) % 10 + 48;
		setTimeTitleStr[11] = 0;
	}

	text_layer_set_text(minStr, minStrTxt);
	text_layer_set_text(secStr, secStrTxt);
	text_layer_set_text(setTimeTitle, setTimeTitleStr);
} //End updateSetTimeScreen

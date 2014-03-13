/**
 * File: intervalSetScreen.c
 * @author Jace Ferguson <fjace05@gmail.com>
 *
 * Last Update March 2014'
 *
 * This screen lets the user pick how many intervals they want
 * to set up. The maximum is (arbitrarily) hard coded to 10. It can likely go
 * higher as long as overflow doesn't become an issue in certain parts of the app.
 *
 */
#include <pebble.h>
#include <pebble_fonts.h>
#include "../includes/intervals.h"
#include "../includes/intervalSetScreen.h"

//Get the reference to this layer from the intervals.c
extern Layer *intervalLayer;

//Visual elements needed for this screen
TextLayer *settingsTitle, *setIntervalModeText, *intervalCountString;

/**
 * Sets how many interval timers to use.
 * Change can be positive or negative.
 * Change the values in the if statement to
 * increase the number of intervals you can have.
 */
void adjustIntervals(int8_t change)
{
	uint8_t intervalCount = getIntervalCount();
	if ((intervalCount + change) <= 10 && (intervalCount + change) >= 1)
	{
		intervalCount = intervalCount + change;
		//Update the screen.
		setIntervalCount(intervalCount);
		setIntervalCountTxt(intervalCount);

	}
} //End adjustIntervals

/**
 * Cleanup this screen.
 */
void deinitIntervalSetScreen()
{
	text_layer_destroy(intervalCountString);
	text_layer_destroy(settingsTitle);
	text_layer_destroy(setIntervalModeText);
}

/**
 * Build this screen.
 */
void initIntervalSetScreen()
{
	//Hide the layer by default.
	layer_set_hidden(intervalLayer, true);

	//Set mode text
	setIntervalModeText = text_layer_create(GRect(0, 15, 144, 40));
	text_layer_set_text(setIntervalModeText, "Set Mode");
	text_layer_set_font(setIntervalModeText,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(setIntervalModeText, GTextAlignmentCenter);
	layer_add_child(intervalLayer, text_layer_get_layer(setIntervalModeText));

	//# of layers text
	settingsTitle = text_layer_create(GRect(0, 40, 144, 40));
	text_layer_set_font(settingsTitle,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(settingsTitle, GTextAlignmentCenter);
	text_layer_set_text(settingsTitle, "# of Intervals");
	layer_add_child(intervalLayer, text_layer_get_layer(settingsTitle));

	//Actual interval count text
	intervalCountString = text_layer_create(GRect(0, 80, 144, 50));
	text_layer_set_font(intervalCountString,
			fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(intervalCountString, GTextAlignmentCenter);
	layer_add_child(intervalLayer, text_layer_get_layer(intervalCountString));

	//Prefill the interval count with what it currently is
	setIntervalCountTxt(getIntervalCount());
} //End initIntervalCountScreen

/**
 * Update the interval text with the passed in
 * value.
 */
void setIntervalCountTxt(uint16_t count)
{
	static char countStr[3];
	//Convert the int value to corresonding ascii.
	if (count < 10)
	{
		countStr[0] = count + 48;
		countStr[1] = 0;
	}
	else
	{
		countStr[0] = count / 10 + 48;
		countStr[1] = count % 10 + 48;
		countStr[2] = 0;
	}
	text_layer_set_text(intervalCountString, countStr);
}

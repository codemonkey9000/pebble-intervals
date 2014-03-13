/**
 * File: runScreen.c
 * @author Jace Ferguson <fjace05@gmail.com>
 *
 * Last Update March 2014'
 *
 * runScreen.c is the last "screen" for the app. It displays the countdown
 * intervals as well as a countup clock. (Which will overflow at some point. I should
 * probably fix that...)
 *
 */
#include <pebble.h>
#include <pebble_fonts.h>
#include "../includes/types.h"
#include "../includes/intervals.h"
#include "../includes/runScreen.h"

//Reference to the pointer for this layer from intervals.c
extern Layer *runLayer;

//The pattern to use for the vibration.

const VibePattern vibeSeq =
{
//Change this value to change the length of the vibration.
		.durations = (uint32_t[]
)				{	150},
				.num_segments = 1
			};

//Visual elements for this screen.
TextLayer *runModeTitleTextLayer, *runModeIntervalTextLayer;
TextLayer *runModeIntRunTimeTextLayer, *runModeTotalRunTimeTextLayer;

//Flag to indicate if the timers are running
bool isRunningFlag = false;
//The interval that is currently active
uint8_t currRunInt = 0;
//Timer counters! (Now  32 bit for even more overflow protection!)
uint32_t currSecCount = 0, totalRunCount = 0;
//Counter for how many vibrations to pulse.
uint8_t vibeCount = 0;
char timeStringText[6];
char runTimeStringText[9];

/**
 * When run mode starts for we need to clear out
 * the acculmulator variables.
 */
void activateRunMode()
{
	//Initialize stuff
	//The countup timer
	totalRunCount = 0;
	//The interval we're currently in
	currRunInt = 0;
	//A countup counter for the seconds elapsed in this interval
	currSecCount = 0;
	//Redraw the new screen.
	updateRunTimeScreen();
}

/**
 * Clean up on shut down.
 */
void deinitRunScreen()
{
	text_layer_destroy(runModeTitleTextLayer);
	text_layer_destroy(runModeIntervalTextLayer);
	text_layer_destroy(runModeIntRunTimeTextLayer);
	text_layer_destroy(runModeTotalRunTimeTextLayer);
}

/**
 * Make it vibrate!
 */
void doVibrate()
{
	//See if we need to vibrate anymore.
	if (vibeCount > 0)
	{
		vibes_enqueue_custom_pattern(vibeSeq);
		vibeCount--;
		//Reload the timer for the next vibration
		app_timer_register(200, (AppTimerCallback) handle_timer_event, NULL);
	}
}

/**
 * Build the screen
 */
void initRunScreen()
{
	char * setTimeTitleStr = getTimeTitleStr();
	//Hide it by default.
	layer_set_hidden(runLayer, true);

	//Current layer lable
	runModeTitleTextLayer = text_layer_create(GRect(0, 15, 144, 40));
	text_layer_set_text(runModeTitleTextLayer, "Run Mode");
	text_layer_set_font(runModeTitleTextLayer,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(runModeTitleTextLayer, GTextAlignmentCenter);
	layer_add_child(runLayer, text_layer_get_layer(runModeTitleTextLayer));

	//Current interval text
	runModeIntervalTextLayer = text_layer_create(GRect(0, 40, 144, 40));
	text_layer_set_text(runModeIntervalTextLayer, setTimeTitleStr);
	text_layer_set_font(runModeIntervalTextLayer,
			fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
	text_layer_set_text_alignment(runModeIntervalTextLayer,
			GTextAlignmentCenter);
	layer_add_child(runLayer, text_layer_get_layer(runModeIntervalTextLayer));

	//Create the countdown layer
	runModeIntRunTimeTextLayer = text_layer_create(GRect(0, 80, 144, 50));
	text_layer_set_font(runModeIntRunTimeTextLayer,
			fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
	text_layer_set_text_alignment(runModeIntRunTimeTextLayer,
			GTextAlignmentCenter);
	layer_add_child(runLayer, text_layer_get_layer(runModeIntRunTimeTextLayer));

	//Create the countup layer
	runModeTotalRunTimeTextLayer = text_layer_create(GRect(0, 130, 144, 40));
	text_layer_set_font(runModeTotalRunTimeTextLayer,
			fonts_get_system_font(FONT_KEY_GOTHIC_18));
	text_layer_set_text_alignment(runModeTotalRunTimeTextLayer,
			GTextAlignmentCenter);
	layer_add_child(runLayer,
			text_layer_get_layer(runModeTotalRunTimeTextLayer));
}

/**
 * Returns true if the timer is running
 */
bool isRunning()
{
	return isRunningFlag;
}

/**
 * In run mode, this will skip whatever time is remaining
 * in the current interval and move to the next
 */
void skipToNextInterval()
{

	/*
	 To avoid race conditions with the timer handler
	 pause the timer before switching intervals.
	 */
	isRunningFlag = false;
	//Reset the countup counter for elapsed time in the current interval
	currSecCount = 0;
	//Move to the next interval
	currRunInt++;
	//Go back to the first interval if necessary
	if (currRunInt == getIntervalCount())
	{
		currRunInt = 0;
	}
	updateRunTimeScreen();
	//Restart the timer!
	isRunningFlag = true;
} //End skipToNextInterval

/**
 In run mode, this will skip whatever time is remaining
 in the current interval and move to the previous one.
 **/
void skipToPrevInterval()
{
	//Pause the timer
	isRunningFlag = false;
	currSecCount = 0;

	//Figure out which interval to go to.
	if (currRunInt == 0)
	{
		currRunInt = getIntervalCount() - 1;
	}
	else
	{
		currRunInt--;
	}
	updateRunTimeScreen();
	//Restart the time.
	isRunningFlag = true;
} //End skipToPrevInterval

/**
 * Start and stop the running timer.
 */
void toggleRunning()
{
	isRunningFlag = !isRunningFlag;
} //End toggleRunning

/**
 * Update the timers in run mode. Called every second
 * by a timer handler.
 */
void tick()
{
	uint16_t *intervals = getIntervals();
	uint8_t intervalCount = getIntervalCount();
	//We don't do any thing if the timers aren't running
	if (!isRunningFlag)
	{
		return;
	}
	//Increment the seconds elapsed in current interval counter
	currSecCount++;

	//Check to see if we have gone over the time we set for this interval
	if (currSecCount > intervals[currRunInt])
	{
		//If so, we need to go to the next interval.
		//Reset the seconds elapsed in current interval counter
		currSecCount = 0;
		//Go to the next interval
		currRunInt++;
		//This will vibrate the same number of interval we just finished
		vibeCount = currRunInt;
		//Start the vibrating
		handle_timer_event();

		//Make sure we don't need to go back to the first interval.
		if (currRunInt >= intervalCount)
		{
			currRunInt = 0;
		}
		//Update the screen
		updateRunTimeScreen();

		//We don't want to do the rest of the stuff in the function.
		return;
	}
	//Increment the elapsed time counter
	totalRunCount++;

	//Update the screen to show that a second elapsed.
	updateRunTimeScreen();
} //End tick

/**
 * Update the screen to show the change in the timers.
 */
void updateRunTimeScreen()
{

	uint16_t *intervals = getIntervals();
	uint8_t intervalCount = getIntervalCount();
	char * setTimeTitleStr = getTimeTitleStr();
	//Get the seconds remaining in this interval
	formatTime(intervals[currRunInt] - currSecCount, timeStringText, false);
	//Get the total time elapsed time string
	formatTime(totalRunCount, runTimeStringText, true);
	//Update the number of interval we're on.
	if (currRunInt + 1 < 10)
	{
		setTimeTitleStr[9] = currRunInt + 1 + 48;
		setTimeTitleStr[10] = 0;
	}
	else
	{
		setTimeTitleStr[9] = (currRunInt + 1) / 10 + 48;
		setTimeTitleStr[10] = (currRunInt + 1) % 10 + 48;
		setTimeTitleStr[11] = 0;
	}

	//Update the corresponding text layers
	text_layer_set_text(runModeIntervalTextLayer, setTimeTitleStr);
	text_layer_set_text(runModeIntRunTimeTextLayer, timeStringText);
	text_layer_set_text(runModeTotalRunTimeTextLayer, runTimeStringText);
} //End updateRunTimeScreen


/**
 * File: intervals.c
 * @author Jace Ferguson <fjace05@gmail.com>
 *
 * Last Update March 2014'
 *
 * Program for the Pebble Smartwatch.
 *
 * This app lets the user set multiple timer each with a different countdown time.
 * The app will cycle through the timers alerting the user when an interval elapses,
 * and repeats intervals once they've all elapsed.
 *
 */
#include <pebble.h>
#include "../includes/types.h"
#include "../includes/intervals.h"
#include "../includes/intervalSetScreen.h"
#include "../includes/timeSetScreen.h"
#include "../includes/runScreen.h"

//The pointer for the app window
static Window *window;
//The three layers used for each set mode.
Layer *intervalLayer, *runLayer, *setTimeLayer;

//Initial state of the app
SettingsState current_state = INTERVAL_COUNT;

//The number of intervals for the current session
uint8_t intervalCount = 1;

//The interval we are setting the time one.
uint8_t currIntervalSetIdx = 0;

//An array of 'seconds'. Each element corresponds to the time for one interval
uint16_t intervals[10] =
{ 0 };

//Initial text for the set and run screens
char setTimeTitleStr[] = "Interval 00";

/**
 * Down button pressed handler.
 */
void button_pressed_down(ClickRecognizerRef recognizer)
{

	if (current_state == INTERVAL_COUNT)
	{
		//Decrease interval count.
		adjustIntervals(-1);
	}
	else if (current_state == TIME_SET)
	{
		//Lower the currently selected 'set unit'
		adjustIntervalSetTime(-1);
	}
	else //We're in run mode here and we'll go to the previous interval.
	{
		skipToPrevInterval();
	}
} //End button_pressed_down

/**
 * Up button pressed handler.
 */
void button_pressed_up(ClickRecognizerRef recognizer)
{
	if (current_state == INTERVAL_COUNT)
	{
		//Increase interval count.
		adjustIntervals(1);
	}
	else if (current_state == TIME_SET)
	{
		//Increase the currently selected 'set unit'
		adjustIntervalSetTime(1);
	}
	else //We're in run mode so skip to the next interval.
	{
		skipToNextInterval();
	}
} //End button_pressed_up

/**
 * Connect the buttons to the handler functions
 */
void click_provider(Window *window)
{

	window_single_click_subscribe(BUTTON_ID_SELECT,
			(ClickHandler) select_pressed);
	window_long_click_subscribe(BUTTON_ID_SELECT, 0,
			(ClickHandler) select_long_press, NULL);
	window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 2, 100, true,
			(ClickHandler) select_double_press);

	window_single_repeating_click_subscribe(BUTTON_ID_UP, 200,
			(ClickHandler) button_pressed_up);
	window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 200,
			(ClickHandler) button_pressed_down);

} //End settings_click_provider

/**
 * This function is used to format a time.
 *
 * @param time - The time to format in seconds
 * @param timeStr - a character string to insert the formatted time into
 * @param setHours - whether or not to include hours in the string
 */
void formatTime(uint16_t time, char *timeStr, bool setHours)
{
	app_log(APP_LOG_LEVEL_INFO, "intervals", 101, "Passed in time %d", time);

	uint16_t hours, mins, secs;

	hours = time / 3600;
	mins = time % 3600 / 60;
	secs = time % 60;

	if (setHours)
	{
		timeStr[0] = (hours / 10) + 48;
		timeStr[1] = (hours % 10) + 48;
		timeStr[2] = ':';

		timeStr[3] = (mins / 10) + 48;
		timeStr[4] = (mins % 10) + 48;
		timeStr[5] = ':';

		timeStr[6] = (secs / 10) + 48;
		timeStr[7] = (secs % 10) + 48;
		timeStr[8] = 0;
	}
	else
	{
		timeStr[0] = (mins / 10) + 48;
		timeStr[1] = (mins % 10) + 48;
		timeStr[2] = ':';

		timeStr[3] = (secs / 10) + 48;
		timeStr[4] = (secs % 10) + 48;
		timeStr[5] = 0;
	}

} //End formatTime

/**
 * Return the current state of the app
 */
SettingsState getCurrState()
{
	return current_state;
}

/**
 * Get the array of intervals
 */
uint16_t * getIntervals()
{
	return intervals;
}

/**
 * Get the current count of intervals
 */
uint8_t getIntervalCount()
{
	return intervalCount;
}

/**
 * Get the index of the interval being set
 */
uint8_t getCurrIntervalSetIdx()
{
	return currIntervalSetIdx;
}

/**
 * Get the title string for the set and run modes
 */
char * getTimeTitleStr()
{
	return setTimeTitleStr;
}

/**
 * Cleanup on App close
 */
void handle_deinit()
{
	//Destroy each sublayer
	deinitIntervalSetScreen();
	deinitTimeSetScreen();
	deinitRunScreen();

	layer_destroy(intervalLayer);
	layer_destroy(setTimeLayer);
	layer_destroy(runLayer);

	window_destroy(window);
}

/**
 * Initializes the window and the three main "subroot" screen layers
 */
void handle_init()
{
	//Create the window.
	window = window_create();
	window_set_click_config_provider(window,
			(ClickConfigProvider) click_provider);

	//Get the window layer and bounds.
	Layer *windowLayer = window_get_root_layer(window);
	GRect windowBounds = layer_get_bounds(windowLayer);

	//Create the subroot screens
	intervalLayer = layer_create(windowBounds);
	setTimeLayer = layer_create(windowBounds);
	runLayer = layer_create(windowBounds);

	//Initialize the three separate layers
	initIntervalSetScreen();
	initTimeSetScreen();
	initRunScreen();

	//Add the layers to the window layer.
	layer_add_child(windowLayer, intervalLayer);
	layer_add_child(windowLayer, setTimeLayer);
	layer_add_child(windowLayer, runLayer);

	//Add the window to the stack
	window_stack_push(window, true);
	tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

	//RUN IT!
	runApp();

} //End handle_init

/**
 * This handler gets called every second.
 * In run mode in makes the countdown tick
 */
void handle_second_tick(struct tm *tick_time, TimeUnits units_changed)
{

	if (current_state == RUN_MODE)
	{
		tick();
	}
} //End handle_second_tick

/**
 * A timer handler. A timer is used to make the units
 * flash when setting the time. It's also used for vibrating! Yay, dual purpose!
 */
void handle_timer_event()
{
	if (current_state == TIME_SET)
	{
		flashUnit();

	}
	else if (current_state == RUN_MODE)
	{
		doVibrate();
	}

} //End handle_time_event

/**
 * Main function called when the app starts
 */
int main()
{
	//Set handlers for initialization, tick handler, and time handler
	handle_init();

	//Starts the loop
	app_event_loop();

	//Handle cleanup
	handle_deinit();
	return 0;
} //End pbl_main

/**
 * Called when the button is pressed to go to the next state.
 */
void nextState()
{
	if (current_state == INTERVAL_COUNT)
	{
		//The set time mode comes after the interval count mode
		//Change state
		current_state = TIME_SET;
		//Start the time that flashes the units
		app_timer_register(150, (AppTimerCallback) handle_timer_event, NULL);
		//Clear out the index for the interval time we're setting
		currIntervalSetIdx = 0;
		//Hide the current layer and show the next one.
		layer_set_hidden(intervalLayer, true);
		layer_set_hidden(setTimeLayer, false);
		//Update the screen to show the cleared time.
		updateSetTimeScreen();
	}
	else //In a time set stage
	{
		//if true, then all interval times have been set
		if ((currIntervalSetIdx + 1) == intervalCount)
		{
			currIntervalSetIdx++;
			//Hide the current layer and show the next.
			layer_set_hidden(setTimeLayer, true);
			layer_set_hidden(runLayer, false);
			//Set the new state
			current_state = RUN_MODE;
			activateRunMode();
		}
		else //We just need to set the next interval time
		{
			currIntervalSetIdx++;
			updateSetTimeScreen();
		}
	}

} //End nextState

/**
 * This gets called when the select button is long pressed to go to previous state
 */
void prevState()
{
	if (current_state == RUN_MODE)
	{
		//Only go to the previous state from run mode if the timer is paused
		if (!isRunning())
		{
			//Switch to time_set mode
			current_state = TIME_SET;
			//Start the timer for unit flashing
			app_timer_register(150, (AppTimerCallback) handle_timer_event,
					NULL);
			currIntervalSetIdx--;
			layer_set_hidden(runLayer, true);
			layer_set_hidden(setTimeLayer, false);
			updateSetTimeScreen();
		}
	}
	else if (current_state == INTERVAL_COUNT)
	{
		//Do nothing. Can't go back from this...
	}
	else //In a time set stage
	{
		//There are more interval to set time for.
		if ((currIntervalSetIdx - 1) >= 0)
		{
			currIntervalSetIdx--;
			updateSetTimeScreen();
		}
		else //There are no more time set screens, go back to the interval set screen
		{
			current_state = INTERVAL_COUNT;
			layer_set_hidden(setTimeLayer, true);
			layer_set_hidden(intervalLayer, false);
		}
	}

} //End prevState

void runApp()
{
	//Start the app by showing the interval layer.
	layer_set_hidden(intervalLayer, false);
}

/**
 * Double press select handler. Moves to the next state if not in the run state currently.
 */
void select_double_press(ClickRecognizerRef rec)
{
	if (current_state == TIME_SET || current_state == INTERVAL_COUNT)
	{
		nextState();
	}
}

/**
 *    The handler for when the select button is LONG PRESSED.
 *    Always attempts to revert to the previous screen.
 */
void select_long_press(ClickRecognizerRef rec)
{
	prevState();
} //End select_long_press

/**
 * The handler for when the select button is pressed (NOT HELD).
 * In RUN_MODE the timer is started or stpped.
 * In any other mode, the next state is selected.
 */
void select_pressed(ClickRecognizerRef rec)
{
	if (current_state == RUN_MODE)
	{
		toggleRunning();
	}
	else if (current_state == TIME_SET)
	{
		changeUnit();
	}
} //End selected_pressed


/**
 * Sets the number of intervals we will be using.
 */
void setIntervalCount(uint8_t ct)
{
	intervalCount = ct;
}

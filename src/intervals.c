#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

/**
intervals.c
April 2013
By Jace Ferguson

Program for the Pebble Smartwatch.

This app allows the user to set up to 5 interval timers. After setting
each timer, the app will cycle through the interval alerting the user when
time has elapsed by vibrating. Once the end of the set intervals has been 
reached, it will start back with the first interval and continue until
stopped.

**/

/*
There are three different "states" or screens in the app.
We use this type to identify which one we are in.
*/
enum SettingsState {
INTERVAL_COUNT,
TIME_SET,
RUN_MODE
};

/*
The unit of time we are setting blinks, this enum
is used to identify what is being set.
*/
enum SettingsUnit {
  SETTING_SECOND = 1,
  SETTING_MINUTE = 0
};

void adjustIntervals(int8_t change);
void adjustIntervalSetTime(enum SettingsUnit unit, int8_t change);
void button_pressed_down(ClickRecognizerRef recognizer, Window *window);
void button_pressed_up(ClickRecognizerRef recognizer, Window *window);
void changeUnit(ClickRecognizerRef rec, Window *window);
void formatTime(uint16_t time, char *timeStr, bool setHours);
void handle_init(AppContextRef ctx);
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t);
void handle_timer_event(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie);
void initIntervalCountScreen();
void initRunScreen();
void initSetTimeScreen();
void nextState();
void pbl_main(void *params);
void prevState();
void select_long_press(ClickRecognizerRef rec, Window *window);
void select_pressed(ClickRecognizerRef rec, Window *window);
void settings_click_provider(ClickConfig **config, Window *window);
void skipToNextInterval();
void skipToPrevInterval();
void tick();
void toggleRunning();
void updateIntervalCount();
void updateRunTimeScreen();
void updateSetTimeScreen();
void vibrate();

#define MY_UUID { 0x05, 0x6E, 0x0F, 0x30, 0x6D, 0xE2, 0x45, 0xE4, 0xA3, 0xE7, 0x56, 0xAE, 0xAB, 0xEB, 0xBE, 0x37 }
PBL_APP_INFO(MY_UUID,
             "Intervals", "Jace Ferguson",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

const VibePattern vibeSeq = {
  .durations = (uint32_t []) {150},
  .num_segments = 1
};

AppContextRef appCtx;

Window window;

//Each of the app 'modes' has its own layer
Layer runLayer, intervalLayer, setTimeLayer;

TextLayer minStr, secStr, titleText, colonString;

TextLayer intervalCountString, setTimeTitle, setIntervalModeText, setTimeModeText;

TextLayer runTitleText, runMinString, runSecString, runColonString, totalRunHours, totalRunMins, totalRunSecs, totalRunColons;

TextLayer runModeTitleTextLayer, runModeIntRunTimeTextLayer, runModeTotalRunTimeTextLayer, runModeIntervalTextLayer;

uint16_t intervals[5], currSecCount = 0, totalRunCount = 0;

uint8_t intervalCount = 1, currIntervalSetIdx, vibeCount = 0, currRunInt = 0;

char setTimeTitleStr[] = "Interval 0";

//Initial state of the app
enum SettingsState current_state = INTERVAL_COUNT;
enum SettingsUnit setting_unit = SETTING_MINUTE;
bool isRunning = false;
bool unitShown = true;

/**
Sets how many interval timers to use. Change
can be positive or negative. Change the values
in the if statement to increase the number of intervals
you can have.
**/
void adjustIntervals(int8_t change)
{
  uint8_t newInterval = intervalCount + change;
  if(newInterval <= 5 && newInterval >= 1)
  {
    intervalCount = newInterval;
    //Update the screen.
    updateIntervalCount();
  }
}//End adjustIntervals

/**
Change the amount of seconds the currIntervalSetIdx interval
timer will be set for. 

param unit - is the unit we are setting currently. 
param change - can be positive or negative and specifies how much to change
the interval
**/
void adjustIntervalSetTime(enum SettingsUnit unit, int8_t change)
{
  uint16_t newTime;
  
  /*
  If we're adjusting minutes we need to multiple change by 60
  since time is stored in the array as seconds.
  */
  if(unit)
  {
    newTime = intervals[currIntervalSetIdx] + change;
  }
  else
  {
    newTime = intervals[currIntervalSetIdx] + (change * 60);
  }

  //An arbitrary limit of one hour per timer.
  if(newTime > 0 && newTime < 3600)
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
}//End adjustIntervalSetTime

/**
Down button pressed handler.
**/
void button_pressed_down(ClickRecognizerRef recognizer, Window *window)
{

  if(current_state == INTERVAL_COUNT){
    adjustIntervals(-1);
  }
  else if(current_state == TIME_SET)
  {
    adjustIntervalSetTime(setting_unit, -1);
  }
  else //We're in run mode here and we'll go to the previous interval.
  {
    skipToPrevInterval();
  }
}//End button_pressed_down

/**
Up button pressed handler.
**/
void button_pressed_up(ClickRecognizerRef recognizer, Window *window)
{
  if(current_state == INTERVAL_COUNT){
    adjustIntervals(1);
  }
  else if(current_state == TIME_SET){
    adjustIntervalSetTime(setting_unit, 1);
  }
  else //We're in run mode so skip to the next interval.
  {
    skipToNextInterval();
  }
}//End button_pressed_up

/**
Change the unit we're currently setting.
Also make sure whatever we had been previously setting
gets set to black so it doesn't disappear.
**/
void changeUnit(ClickRecognizerRef rec, Window *window)
{
  //Make sure we don't accidentally leave one white and unreadable.
  text_layer_set_text_color(&minStr, GColorBlack);
  text_layer_set_text_color(&secStr, GColorBlack);
  //Change the unit
  setting_unit = !setting_unit;
}//End changeUnit

/**
This function is used to format a time.

param time - The time to format in seconds
param timeStr - a character string to insert the formatted time into
param setHours - whether or not to include hours in the string
**/
void formatTime(uint16_t time, char *timeStr, bool setHours)
{
  static uint16_t hours, mins, secs;

  hours = time / 3600;
  mins = time % 3600 / 60;
  secs = time % 60;
  
  if(setHours)
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
}//End formatTime

/**
Gets called after pbl_main to initialize the app.
**/
void handle_init(AppContextRef ctx) {
  (void)ctx;

  appCtx = ctx;
  
  //Create the window.
  window_init(&window, "Main Window");
  window_set_background_color(&window, GColorWhite);
  window_stack_push(&window, true /* Animated */);
  window_set_fullscreen(&window, true);
  window_set_click_config_provider(&window, (ClickConfigProvider) settings_click_provider);

  //Create and add the screens for each mode.
  initIntervalCountScreen();  
  layer_add_child(&window.layer, &intervalLayer);

  initSetTimeScreen();
  layer_add_child(&window.layer, &setTimeLayer);

  initRunScreen(); 
  layer_add_child(&window.layer, &runLayer);

  //shows the interval layer
  layer_set_hidden(&intervalLayer, false);
  
}//End handle_init

/**
This handler gets called every second.

In run mode in makes the countdown tick
**/
void handle_second_tick(AppContextRef ctx, PebbleTickEvent *t){

    if(current_state == RUN_MODE){
      if(isRunning)
      {
        tick();
      }
    }
}//End handle_second_tick

/**
A timer handler. A timer is used to make the units flash when setting the time.
It's also used for vibrating! Yay, dual purpose!
**/
void handle_timer_event(AppContextRef app_ctx, AppTimerHandle handle, uint32_t cookie)
{

  if(current_state == TIME_SET)
  {
      GColor newColor;
      if(unitShown)
      {
        newColor = GColorWhite;
      }
      else
      {
        newColor = GColorBlack;
      }
      unitShown = !unitShown;
      if(setting_unit == SETTING_MINUTE)
      {
        text_layer_set_text_color(&minStr, newColor); 
      }
      else
      {
        text_layer_set_text_color(&secStr, newColor); 
      }
      //Reload the timer
      app_timer_send_event(app_ctx, 250, 1);
      
    }
    else if(vibeCount > 0)
    {
      vibes_enqueue_custom_pattern(vibeSeq);
      vibeCount--;
      app_timer_send_event(appCtx, 200, 1);
    }

}//End handle_time_event

/**
Build the layer that is used to increment the counter
**/
void initIntervalCountScreen()
{
  static TextLayer settingsTitle;

  layer_init(&intervalLayer, GRect(0, 0, 144, 168));
  layer_set_hidden(&intervalLayer, true);

  //Run Text layer
  text_layer_init(&setIntervalModeText, GRect(0, 15, 144, 40));
  text_layer_set_text(&setIntervalModeText, "Set Mode");
  text_layer_set_font(&setIntervalModeText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&setIntervalModeText, GTextAlignmentCenter);
  text_layer_set_text_color(&setIntervalModeText, GColorBlack);
  text_layer_set_background_color(&setIntervalModeText, GColorWhite);
  layer_add_child(&intervalLayer, &setIntervalModeText.layer);

  //Setup the title layers
  text_layer_init(&settingsTitle, GRect(0, 40, 144, 40));
  text_layer_set_font(&settingsTitle, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&settingsTitle, GTextAlignmentCenter);
  text_layer_set_text_color(&settingsTitle, GColorBlack);
  text_layer_set_background_color(&settingsTitle, GColorWhite);
  text_layer_set_text(&settingsTitle, "# of Intervals");
  layer_add_child(&intervalLayer, &settingsTitle.layer);

  //Next the interval counter
  text_layer_init(&intervalCountString, GRect(0, 80, 144, 50));
  text_layer_set_font(&intervalCountString, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
  text_layer_set_text_alignment(&intervalCountString, GTextAlignmentCenter);
  text_layer_set_text_color(&intervalCountString, GColorBlack);
  layer_add_child(&intervalLayer, &intervalCountString.layer);

  //Prefil the interval count with what it currently is
  updateIntervalCount();
}//End initIntervalCountScreen

/**
Build the layer that is used during run mode.
**/
void initRunScreen()
{
  layer_init(&runLayer, GRect(0, 0, 144, 168));
  layer_set_hidden(&runLayer, true);  

  //Run Text layer
  text_layer_init(&runModeTitleTextLayer, GRect(0, 15, 144, 40));
  text_layer_set_text(&runModeTitleTextLayer, "Run Mode");
  text_layer_set_font(&runModeTitleTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&runModeTitleTextLayer, GTextAlignmentCenter);
  text_layer_set_text_color(&runModeTitleTextLayer, GColorBlack);
  text_layer_set_background_color(&runModeTitleTextLayer, GColorWhite);
  layer_add_child(&runLayer, &runModeTitleTextLayer.layer);
  
  //Change the title text
  text_layer_init(&runModeIntervalTextLayer, GRect(0, 40, 144, 40));
  text_layer_set_text(&runModeIntervalTextLayer, setTimeTitleStr);
  text_layer_set_font(&runModeIntervalTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&runModeIntervalTextLayer, GTextAlignmentCenter);
  text_layer_set_text_color(&runModeIntervalTextLayer, GColorBlack);
  text_layer_set_background_color(&runModeIntervalTextLayer, GColorWhite);
  layer_add_child(&runLayer, &runModeIntervalTextLayer.layer);

  //Create the countdown layer
  text_layer_init(&runModeIntRunTimeTextLayer, GRect(0, 80, 144, 50));
  text_layer_set_font(&runModeIntRunTimeTextLayer, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
  text_layer_set_text_alignment(&runModeIntRunTimeTextLayer, GTextAlignmentCenter);
  text_layer_set_text_color(&runModeIntRunTimeTextLayer, GColorBlack);
  layer_add_child(&runLayer, &runModeIntRunTimeTextLayer.layer);

  //Create the countup layer
  text_layer_init(&runModeTotalRunTimeTextLayer, GRect(0, 130, 144, 40));
  text_layer_set_font(&runModeTotalRunTimeTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(&runModeTotalRunTimeTextLayer, GColorBlack);
  text_layer_set_text_alignment(&runModeTotalRunTimeTextLayer, GTextAlignmentCenter);  
  layer_add_child(&runLayer, &runModeTotalRunTimeTextLayer.layer);
  

}//End initRunScreen

/**
Build the screen that is used when setting the time for each itnerval
**/
void initSetTimeScreen()
{  
  layer_init(&setTimeLayer, GRect(0, 0, 144, 168));
  layer_set_hidden(&setTimeLayer, true);  

  //Run Text layer
  text_layer_init(&setTimeModeText, GRect(0, 15, 144, 40));
  text_layer_set_text(&setTimeModeText, "Set Mode");
  text_layer_set_font(&setTimeModeText, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&setTimeModeText, GTextAlignmentCenter);
  text_layer_set_text_color(&setTimeModeText, GColorBlack);
  text_layer_set_background_color(&setTimeModeText, GColorWhite);
  layer_add_child(&setTimeLayer, &setTimeModeText.layer);
  
  //Change the title text
  text_layer_init(&setTimeTitle, GRect(0, 40, 144, 40));
  text_layer_set_text(&setTimeTitle, setTimeTitleStr);
  text_layer_set_font(&setTimeTitle, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(&setTimeTitle, GTextAlignmentCenter);
  text_layer_set_text_color(&setTimeTitle, GColorBlack);
  text_layer_set_background_color(&setTimeTitle, GColorWhite);
  layer_add_child(&setTimeLayer, &setTimeTitle.layer);

  //Create the three layers for displaying time.
  text_layer_init(&colonString, GRect(0, 80, 144, 50));
  text_layer_set_font(&colonString, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
  text_layer_set_text_alignment(&colonString, GTextAlignmentCenter);
  text_layer_set_text_color(&colonString, GColorBlack);
  text_layer_set_text(&colonString, ":");
  layer_add_child(&setTimeLayer, &colonString.layer);

  //Minutes
  text_layer_init(&minStr, GRect(0, 80, 70, 50));
  text_layer_set_font(&minStr, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
  text_layer_set_text_color(&minStr, GColorBlack);
  text_layer_set_text_alignment(&minStr, GTextAlignmentRight);
  layer_add_child(&setTimeLayer, &minStr.layer);

  //Seconds
  text_layer_init(&secStr, GRect(74, 80, 70, 50));
  text_layer_set_font(&secStr, fonts_get_system_font(FONT_KEY_GOTHAM_42_BOLD));
  text_layer_set_text_color(&secStr, GColorBlack);
  text_layer_set_text_alignment(&secStr, GTextAlignmentLeft);  
  layer_add_child(&setTimeLayer, &secStr.layer);

}//End initSetTimeScreen

/**
Called when the button is pressed to go to the next state.
**/
void nextState()
{
  if(current_state == INTERVAL_COUNT)
  {
    //The set time mode comes after the interval count mode
    //Start the time that flashes the units
    app_timer_send_event(appCtx, 250, 1);
    //Change state
    current_state = TIME_SET;
    //Clear out the index for the interval time we're setting
    currIntervalSetIdx = 0;
    //Hide the current layer and show the next one.
    layer_set_hidden(&intervalLayer, true);
    layer_set_hidden(&setTimeLayer, false);
    //Update the screen to show the cleared time.
    updateSetTimeScreen();
  }
  else //In a time set stage
  {
    //if true, then all interval times have been set
    if((currIntervalSetIdx + 1) == intervalCount)
    {
      currIntervalSetIdx++;
      //Hide the current layer and show the next.
      layer_set_hidden(&setTimeLayer, true);
      layer_set_hidden(&runLayer, false);
      //Set the new state
      current_state = RUN_MODE;
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
    else //We just need to set the next interval time
    {
      currIntervalSetIdx++;
      updateSetTimeScreen();
    }
  }
  
}//End nextState

/**
Main function called when the app starts
**/
void pbl_main(void *params) {
//Set handlers for initialization, tick handler, and time handler
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,

    .tick_info = {
      .tick_handler = &handle_second_tick,
      .tick_units = SECOND_UNIT
    },

    .timer_handler = &handle_timer_event
  };
  //Starts the loop
  app_event_loop(params, &handlers);
}//End pbl_main

/**
This gets called when the select button is long pressed to go to previous state
**/
void prevState()
{
  if(current_state == RUN_MODE)
  {
    //Only go to the previous state from run mode if the timer is paused
    if(!isRunning)
    {
      //Switch to time_set mode
      current_state = TIME_SET;
      //Start the timer for unit flashing
      app_timer_send_event(appCtx, 250, 1);
      currIntervalSetIdx--;
      layer_set_hidden(&runLayer, true);
      layer_set_hidden(&setTimeLayer, false);
      updateSetTimeScreen();
    }
  }
  else if(current_state == INTERVAL_COUNT)
  {
    //Do nothing. Can't go back from this...
  }
  else //In a time set stage
  {
    //There are more interval to set time for.
    if((currIntervalSetIdx - 1) >= 0)
    {
      currIntervalSetIdx--;
      updateSetTimeScreen();
    }
    else //There are no more time set screens, go back to the interval set screen
    {
      current_state = INTERVAL_COUNT;
      layer_set_hidden(&setTimeLayer, true);
      layer_set_hidden(&intervalLayer, false);
    }
  }
  
}//End prevState

/**
   The handler for when the select button is LONG PRESSED.
   Always attempts to revert to the previous screen.
**/
void select_long_press(ClickRecognizerRef rec, Window *window)
{
  prevState();
}//End select_long_press

/**
  The handler for when the select button is pressed (NOT HELD).
  In RUN_MODE the timer is started or stpped.
  In any other mode, the next state is selected.
**/
void select_pressed(ClickRecognizerRef rec, Window *window)
{
  if(current_state == RUN_MODE)
  {
    toggleRunning();
  }
  else
  {
    nextState();
  }
}//End selected_pressed

/**
  Establish all the button handlers
**/
void settings_click_provider(ClickConfig **config, Window *window){
	config[BUTTON_ID_SELECT]->click.handler = (ClickHandler) select_pressed;
	
	config[BUTTON_ID_SELECT]->long_click.handler = (ClickHandler) select_long_press;

	config[BUTTON_ID_UP]->click.handler = (ClickHandler) button_pressed_up;
	config[BUTTON_ID_UP]->click.repeat_interval_ms = 300;

	config[BUTTON_ID_DOWN]->click.handler = (ClickHandler) button_pressed_down;
	config[BUTTON_ID_DOWN]->click.repeat_interval_ms = 300;

  config[BUTTON_ID_DOWN]->multi_click.handler = (ClickHandler) changeUnit;
  config[BUTTON_ID_DOWN]->multi_click.last_click_only = true;
  config[BUTTON_ID_DOWN]->multi_click.min = 2;
  config[BUTTON_ID_DOWN]->multi_click.max = 2;
  config[BUTTON_ID_DOWN]->multi_click.timeout = 100;
  
  config[BUTTON_ID_UP]->multi_click.handler = (ClickHandler) changeUnit;
  config[BUTTON_ID_UP]->multi_click.last_click_only = true;
  config[BUTTON_ID_UP]->multi_click.min = 2;
  config[BUTTON_ID_UP]->multi_click.max = 2;
  config[BUTTON_ID_UP]->multi_click.timeout = 100;

	(void)window;
}//End settings_click_provider

/**
  In run mode, this will skip whatever time is
  remaining in the current interval and move to the next
**/
void skipToNextInterval()
{
  /* 
    To avoid race conditions with the timer handler
    pause the timer before switching intervals.
  */
  isRunning = false;
  //Reset the countup counter for elapsed time in the current interval
  currSecCount = 0;
  //Move to the next interval
  currRunInt++;
  //Go back to the first interval if necessary
  if(currRunInt == intervalCount)
  {
    currRunInt = 0;
  }
  updateRunTimeScreen();
  //Restart the timer!
  isRunning = true;
}//End skipToNextInterval

/**
  In run mode, this will skip whatever time is remaining
  in the current interval and move to the previous one.
**/
void skipToPrevInterval()
{
  //Pause the timer
  isRunning = false;
  currSecCount = 0;
  
  //Figure out which interval to go to.
  if(currRunInt == 0)
  {
    currRunInt = intervalCount - 1;
  }  
  else
  {
    currRunInt--;
  }
  updateRunTimeScreen();
  //Restart the time.
  isRunning = true;
}//End skipToPrevInterval

/**
  Update the timers in run mode.
**/
void tick()
{
  //Increment the elapsed time counter
  totalRunCount++;
  //Increment the seconds elapsed in current interval counter
  currSecCount++;
  
  //Check to see if we have gone over the time we set for this interval
  if(currSecCount > intervals[currRunInt])
  {
    //If so, we need to go to the next interval.
    //Reset the seconds elapsed in current interval counter
    currSecCount = 0;
    //Go to the next interval
    currRunInt++;
    //This will vibrate the same number of interval we just finished
    vibeCount = currRunInt;
    //Start the vibrating
    vibrate();
    
    //Make sure we don't need to go back to the first interval.
    if(currRunInt >= intervalCount)
    {
      currRunInt = 0;
    }
    //Update the screen
    updateRunTimeScreen();
    
    //We don't want to do the rest of the stuff in the function. 
    return;
  }
  
  //Update the screen to show that a second elapsed.
  updateRunTimeScreen();
}//End tick

/**
Start and stop the running timer.
**/
void toggleRunning()
{
  isRunning = !isRunning; 
}//End toggleRunning

/**
  Update the screen when the interval count changes
**/
void updateIntervalCount()
{
  static char countStr[2];
  //Convert the int value to corresonding ascii.
  countStr[0] = intervalCount + 48;
  countStr[1] = 0;
  text_layer_set_text(&intervalCountString, countStr);
}//End updateIntervalCount

/**
  Update the screen to show the change in the timers.
**/
void updateRunTimeScreen()
{
  static char timeStringText[6];
  static char runTimeStringText[9];
  
  //Get the seconds remaining in this interval
  formatTime(intervals[currRunInt] - currSecCount, timeStringText, false);
  //Get the total time elapsed time string
  formatTime(totalRunCount, runTimeStringText, true);
  //Update the number of interval we're on.
  setTimeTitleStr[9] = currRunInt + 1 + 48;

  //Update the corresponding text layers
  text_layer_set_text(&runModeIntervalTextLayer, setTimeTitleStr);
  text_layer_set_text(&runModeIntRunTimeTextLayer, timeStringText);  
  text_layer_set_text(&runModeTotalRunTimeTextLayer, runTimeStringText);
}//End updateRunTimeScreen

/**
  Update the screen during time set mode. This is a weird one.
**/
void updateSetTimeScreen()
{
  static char minStrTxt[3];
  static char secStrTxt[3];
  static char fullText [6];
    
  //Get the time string for the amount of seconds.
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
  setTimeTitleStr[9] = currIntervalSetIdx + 1 + 48;

  text_layer_set_text(&setTimeTitle, setTimeTitleStr);
  text_layer_set_text(&minStr, minStrTxt);
  text_layer_set_text(&secStr, secStrTxt);
}//End updateSetTimeScreen

/**
Start the vibration timer to vibrate the same number of times
as the interval count we just finished.
**/
void vibrate()
{
    app_timer_send_event(appCtx, 200, 1);
}//End vibrate


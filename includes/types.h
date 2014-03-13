/**
 * File: types.h
 * @author Jace Ferguson
 *
 */
#ifndef _TYPES_H
#define _TYPES_H
/**
 * There are three different "states" or screens in the app.
 * We use this type to identify which one we are in.
 */
typedef enum
{
	INTERVAL_COUNT, TIME_SET, RUN_MODE
} SettingsState;

/**
 * The unit of time we are setting blinks,
 * this enum is used to identify what is being set.
 */
typedef enum
{
	SETTING_SECOND = 1, SETTING_MINUTE = 0
} SettingsUnit;

#endif

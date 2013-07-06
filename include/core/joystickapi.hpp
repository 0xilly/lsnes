#ifndef _joystickapi__hpp__included__
#define _joystickapi__hpp__included__

//These correspond to various joystick_driver_* functions.
struct _joystick_driver
{
	void (*init)();
	void (*quit)();
	void (*thread_fn)();
	void (*signal)();
	const char* (*name)();
};

struct joystick_driver
{
	joystick_driver(_joystick_driver drv);
};

/**
 * Joystick initialization function.
 *
 * - The third initialization function to be called by window_init().
 * - The call occurs in the main thread.
 * - Implemented by the joystick plugin.
 */
void joystick_driver_init() throw();
/**
 * Joystick quit function.
 *
 * - The third last quit function to be called by window_quit().
 * - The call occurs in the main thread.
 * - Implemented by the joystick plugin.
 */
void joystick_driver_quit() throw();
/**
 * This thread becomes the joystick polling thread.
 *
 * - Called in joystick polling thread.
 */
void joystick_driver_thread_fn() throw();
/**
 * Signal the joystick thread to quit.
 */
void joystick_driver_signal() throw();
/**
 * Identification for joystick plugin.
 */
const char* joystick_driver_name();

#endif

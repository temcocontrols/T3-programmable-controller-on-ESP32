/*
 * Timer fucntions, with callback, mainly for Windows and *nix.
 *
 */

#ifndef timer_INCLUDED
#define timer_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#include <sys/time.h>
#include <signal.h>
#endif

int timer_start(int mSec, void (*timer_handler)(void)); /* returns 0 if success */
void timer_stop(void);

#ifdef __cplusplus
}
#endif

#endif

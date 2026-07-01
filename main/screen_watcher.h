#ifndef SCREEN_WATCHER_H
#define SCREEN_WATCHER_H

#include "lvglui/screens.h"   // for ScreensEnum

// Call once per tick (e.g. inside flow_tick_task), while already lvgl-locked
void screen_watcher_check(void);

// Returns the currently active screen ID (from ScreensEnum)
// Returns 0 if the active screen doesn't match any known screen object
int screen_watcher_get_current(void);

#endif
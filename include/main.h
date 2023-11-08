#ifndef MAIN_H
#define MAIN_H

#include <os_type.h>

#include "TimeChangeRules.h"
#include "daisy595.h"

#include "settings.h"

extern daisy595 disp;
extern settings appSettings;
extern os_timer_t heartbeatTimer;

extern TimeChangeRule *tcr;

#endif
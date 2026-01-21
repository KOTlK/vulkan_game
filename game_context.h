#pragma once

#include "glass.h"

struct Time {
    double  time_double;
    double  dt_double;
    float   time;
    float   dt;
};

struct Game_Context {
    Window* wnd;
    Time    time;
};
#pragma once

#include <raylib.h>

struct DungeonTheme {
    const char* name;
    Color wallFg;
    Color floorFg;
    Color secretFg;
    Color exploredFg;
};

inline constexpr int kNumThemes = 6;

inline const DungeonTheme kThemes[kNumThemes] = {
    {
        "Clásico",
        {255, 255, 255, 255},
        {255, 255, 255, 255},
        {200, 200, 200, 255},
        { 80,  80,  80, 255},
    },
    {
        "Cripta",
        {140, 160, 180, 255},
        {180, 180, 190, 255},
        {100, 110, 120, 255},
        { 30,  40,  60, 255},
    },
    {
        "Bosque",
        {100, 160,  80, 255},
        {140, 200, 100, 255},
        {100, 120,  80, 255},
        { 30,  50,  20, 255},
    },
    {
        "Volcán",
        {200,  80,  50, 255},
        {240, 160,  50, 255},
        {150,  90,  50, 255},
        { 60,  20,  10, 255},
    },
    {
        "Mágico",
        {180, 100, 220, 255},
        {220, 120, 240, 255},
        {130, 100, 150, 255},
        { 50,  20,  70, 255},
    },
    {
        "Ámbar",
        {200, 160,  80, 255},
        {220, 180, 100, 255},
        {140, 110,  60, 255},
        { 60,  40,  10, 255},
    },
};

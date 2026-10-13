#include "DayCycle.hpp"
#include <algorithm>
#include <cmath>

namespace {

struct Keyframe {
    float    hour;
    DayLight light;
};

// Recorrido de un día: noche azul profunda, amanecer rosado, día neutro,
// atardecer naranja y vuelta a la noche. Se interpola entre fotogramas.
const Keyframe kKeys[] = {
    {  0.0f, { 0.20f, 0.24f, 0.38f,  0,  0,  8, 1.00f, DayPhase::Night, 0.65f } },
    {  4.5f, { 0.20f, 0.24f, 0.38f,  0,  0,  8, 1.00f, DayPhase::Night, 0.65f } },
    {  6.0f, { 0.96f, 0.76f, 0.78f, 18,  6,  8, 0.35f, DayPhase::Dawn  } },
    {  7.5f, { 1.00f, 1.00f, 1.00f,  0,  0,  0, 0.00f, DayPhase::Day   } },
    { 17.0f, { 1.00f, 1.00f, 1.00f,  0,  0,  0, 0.00f, DayPhase::Day   } },
    { 18.5f, { 1.00f, 0.72f, 0.48f, 16,  4,  0, 0.30f, DayPhase::Dusk  } },
    { 20.0f, { 0.46f, 0.34f, 0.46f,  4,  0,  6, 0.75f, DayPhase::Dusk, 0.35f } },
    { 21.0f, { 0.20f, 0.24f, 0.38f,  0,  0,  8, 1.00f, DayPhase::Night, 0.65f } },
    { 24.0f, { 0.20f, 0.24f, 0.38f,  0,  0,  8, 1.00f, DayPhase::Night, 0.65f } },
};

float lerp(float a, float b, float t) { return a + (b - a) * t; }

} // namespace

DayLight dayLight(float hour)
{
    hour = advanceHour(hour, 0.0f);
    constexpr int n = sizeof(kKeys) / sizeof(kKeys[0]);
    for (int i = 0; i + 1 < n; i++) {
        const Keyframe& a = kKeys[i];
        const Keyframe& b = kKeys[i + 1];
        if (hour > b.hour) continue;
        float t = (b.hour > a.hour) ? (hour - a.hour) / (b.hour - a.hour) : 0.0f;
        t = t * t * (3 - 2 * t);
        DayLight l;
        l.mulR = lerp(a.light.mulR, b.light.mulR, t);
        l.mulG = lerp(a.light.mulG, b.light.mulG, t);
        l.mulB = lerp(a.light.mulB, b.light.mulB, t);
        l.addR = lerp(a.light.addR, b.light.addR, t);
        l.addG = lerp(a.light.addG, b.light.addG, t);
        l.addB = lerp(a.light.addB, b.light.addB, t);
        l.darkness = lerp(a.light.darkness, b.light.darkness, t);
        l.desat = lerp(a.light.desat, b.light.desat, t);
        l.phase = (t < 0.5f) ? a.light.phase : b.light.phase;
        l.hour = hour;
        return l;
    }
    return kKeys[0].light;
}

float incursionHours(int deepestFloor)
{
    return std::min(12.0f, 2.0f + 1.5f * std::max(1, deepestFloor));
}

float advanceHour(float hour, float hours)
{
    float h = std::fmod(hour + hours, 24.0f);
    return h < 0.0f ? h + 24.0f : h;
}

const char* dayPhaseName(DayPhase p)
{
    switch (p) {
    case DayPhase::Night: return "Noche";
    case DayPhase::Dawn:  return "Amanecer";
    case DayPhase::Day:   return "Día";
    case DayPhase::Dusk:  return "Atardecer";
    }
    return "";
}

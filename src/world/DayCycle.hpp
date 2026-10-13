#pragma once

// Ciclo día/noche de la aldea. La hora (0–24) avanza en tiempo real solo
// mientras el jugador está en la aldea y salta varias horas por cada incursión.
// Toda la iluminación del pueblo sale de dayLight(): no calcular la noche a mano.

inline constexpr float kDayLengthSeconds = 480.0f;  // un día completo en tiempo real
inline constexpr float kStartHour        = 8.0f;    // hora de una partida nueva

enum class DayPhase { Night, Dawn, Day, Dusk };

struct DayLight {
    // Color final = color * mul + add, por canal (0–1 y 0–255)
    float mulR = 1.0f, mulG = 1.0f, mulB = 1.0f;
    float addR = 0.0f, addG = 0.0f, addB = 0.0f;
    float darkness = 0.0f;  // 0 = pleno día, 1 = noche cerrada (farolas y caída de luz)
    DayPhase phase = DayPhase::Day;
    float desat = 0.0f;     // 0 = color pleno, 1 = gris (se aplica antes de mul/add)
    float hour = 12.0f;
};

DayLight dayLight(float hour);

// Horas que pasan en una incursión según el piso más profundo alcanzado
float incursionHours(int deepestFloor);

// Suma horas y normaliza a [0, 24)
float advanceHour(float hour, float hours);

const char* dayPhaseName(DayPhase p);

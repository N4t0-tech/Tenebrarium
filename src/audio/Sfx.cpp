#include "Sfx.hpp"
#include "core/Assets.hpp"
#include <raylib.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <vector>

namespace {

constexpr int   kRate = 22050;
constexpr float kPi   = 3.14159265f;

enum class Osc { Square, Saw, Sine, Noise };

// Una nota con barrido de frecuencia y envolvente de ataque/caída.
struct Tone {
    Osc   osc;
    float f0, f1;      // Hz al inicio y al final
    float dur;         // segundos
    float gain;
    float at  = 0.0f;  // desfase dentro del efecto (segundos)
    float vib = 0.0f;  // vibrato en Hz (0 = sin vibrato)
    float det = 0.0f;  // segunda voz desafinada (p. ej. 0.01 = +1%): batido inquietante
};

// echo/fb dan el eco de cueva; lp suaviza los agudos (1 = sin filtro).
struct Recipe {
    const char*       name;
    std::vector<Tone> tones;
    float             echo = 0.0f;
    float             fb   = 0.0f;
    float             lp   = 0.35f;
};

// Notas en registro grave (Hz)
constexpr float A1 = 55.0f, A2 = 110.0f, C3 = 130.8f, D3 = 146.8f, Eb3 = 155.6f, E3 = 164.8f,
                F3 = 174.6f, G3 = 196.0f, A3 = 220.0f, C4 = 261.6f, D4 = 293.7f, E4 = 329.6f,
                A4 = 440.0f, C5 = 523.3f, E5 = 659.3f;
// Parcial inarmónico de campana
constexpr float kBell = 2.76f;

std::vector<Recipe> recipes() {
    using O = Osc;
    return {
        {"menu_move",   {{O::Sine, A2, A2, 0.06f, 0.35f}, {O::Noise, 700, 300, 0.03f, 0.10f}}, 0.0f, 0.0f, 0.3f},
        {"menu_select", {{O::Sine, A3, A3, 0.45f, 0.35f}, {O::Sine, A3 * kBell, A3 * kBell, 0.25f, 0.12f}},
                        0.16f, 0.30f},
        {"menu_back",   {{O::Sine, E3, D3, 0.20f, 0.4f}, {O::Sine, E3 * kBell, D3 * kBell, 0.12f, 0.08f}}, 0.14f, 0.2f},
        {"step",        {{O::Noise, 260, 120, 0.06f, 0.18f}}, 0.0f, 0.0f, 0.25f},
        {"chest",       {{O::Saw, 95, 70, 0.35f, 0.18f, 0.0f, 22.0f},
                         {O::Sine, A3, A3, 0.35f, 0.3f, 0.30f}, {O::Sine, C4, C4, 0.35f, 0.3f, 0.42f},
                         {O::Sine, E4, E4, 0.60f, 0.3f, 0.54f, 5.0f}}, 0.18f, 0.35f},
        {"coin",        {{O::Sine, 880, 880, 0.35f, 0.3f}, {O::Sine, 880 * kBell, 880 * kBell, 0.15f, 0.12f}},
                        0.12f, 0.30f, 0.5f},
        {"stairs",      {{O::Noise, 300, 120, 0.10f, 0.40f}, {O::Noise, 260, 100, 0.10f, 0.35f, 0.22f},
                         {O::Noise, 220, 80, 0.10f, 0.30f, 0.44f},
                         {O::Saw, A1, A1 * 0.9f, 1.10f, 0.22f, 0.0f, 0.0f, 0.012f}}, 0.22f, 0.40f, 0.25f},
        {"level_up",    {{O::Sine, A3, A3, 0.30f, 0.3f}, {O::Sine, C4, C4, 0.30f, 0.3f, 0.15f},
                         {O::Sine, E4, E4, 0.30f, 0.3f, 0.30f},
                         {O::Saw, A4, A4, 0.90f, 0.15f, 0.45f, 5.0f, 0.008f},
                         {O::Sine, A2, A2, 1.30f, 0.3f, 0.0f, 0.0f, 0.005f}}, 0.20f, 0.40f},
        {"quest",       {{O::Sine, D3, D3, 0.35f, 0.3f}, {O::Sine, F3, F3, 0.35f, 0.3f, 0.18f},
                         {O::Sine, A3, A3, 0.35f, 0.3f, 0.36f}, {O::Sine, D4, D4, 0.80f, 0.3f, 0.54f, 4.0f},
                         {O::Sine, D4 * kBell, D4 * kBell, 0.40f, 0.08f, 0.54f}}, 0.20f, 0.40f},
        {"equip",       {{O::Noise, 2200, 700, 0.12f, 0.25f}, {O::Square, A2, A2 * 0.9f, 0.10f, 0.15f}}, 0.10f, 0.2f, 0.4f},
        {"buy",         {{O::Sine, 880, 880, 0.25f, 0.25f}, {O::Sine, 880 * kBell, 880 * kBell, 0.12f, 0.1f},
                         {O::Sine, 660, 660, 0.35f, 0.25f, 0.12f}, {O::Sine, 660 * kBell, 660 * kBell, 0.15f, 0.1f, 0.12f}},
                        0.12f, 0.30f, 0.5f},
        {"denied",      {{O::Saw, A2, A2, 0.25f, 0.22f}, {O::Saw, Eb3, Eb3, 0.25f, 0.22f}}, 0.0f, 0.0f, 0.25f},
        {"combat_start",{{O::Saw, A1, A1, 1.0f, 0.30f, 0.0f, 0.0f, 0.015f},
                         {O::Square, A2, A2, 0.35f, 0.2f}, {O::Square, Eb3, Eb3, 0.35f, 0.2f},
                         {O::Noise, 150, 600, 0.6f, 0.2f, 0.1f}}, 0.25f, 0.40f, 0.25f},
        {"hit",         {{O::Noise, 1200, 150, 0.14f, 0.5f}, {O::Sine, 150, 45, 0.16f, 0.5f}}, 0.08f, 0.2f},
        {"crit",        {{O::Noise, 2500, 90, 0.30f, 0.55f}, {O::Saw, 200, 40, 0.28f, 0.3f, 0.0f, 0.0f, 0.02f},
                         {O::Sine, 120, 35, 0.30f, 0.5f}}, 0.14f, 0.35f, 0.4f},
        {"player_hurt", {{O::Saw, 140, 50, 0.30f, 0.35f, 0.0f, 28.0f, 0.03f}, {O::Noise, 600, 150, 0.15f, 0.3f}},
                        0.08f, 0.2f, 0.3f},
        {"blocked",     {{O::Square, 620, 600, 0.20f, 0.15f}, {O::Square, 620 * 1.41f, 600 * 1.41f, 0.20f, 0.1f},
                         {O::Noise, 3000, 1000, 0.06f, 0.2f}}, 0.12f, 0.35f, 0.4f},
        {"enemy_die",   {{O::Saw, 160, 40, 0.70f, 0.3f, 0.0f, 6.0f, 0.02f}, {O::Noise, 800, 60, 0.50f, 0.25f}},
                        0.20f, 0.35f, 0.25f},
        {"boss_die",    {{O::Saw, 120, 25, 1.8f, 0.35f, 0.0f, 5.0f, 0.025f}, {O::Noise, 1000, 30, 1.6f, 0.4f},
                         {O::Sine, A1, 20, 1.8f, 0.5f}, {O::Square, Eb3, Eb3 * 0.5f, 1.2f, 0.12f, 0.3f}},
                        0.30f, 0.50f, 0.25f},
        {"spell",       {{O::Sine, A3, A4 * 2, 0.45f, 0.35f, 0.0f, 9.0f, 0.015f},
                         {O::Sine, Eb3, Eb3 * 4, 0.45f, 0.2f, 0.05f, 7.0f}}, 0.18f, 0.40f},
        {"buff",        {{O::Saw, A2, A2, 0.70f, 0.18f, 0.0f, 0.0f, 0.01f}, {O::Saw, E3, E3, 0.70f, 0.14f, 0.05f, 0.0f, 0.01f},
                         {O::Saw, A3, A3, 0.70f, 0.10f, 0.10f, 4.0f, 0.01f}}, 0.20f, 0.40f, 0.2f},
        {"heal",        {{O::Sine, A4, A4, 0.40f, 0.25f, 0.0f, 5.0f}, {O::Sine, C5, C5, 0.40f, 0.25f, 0.14f, 5.0f},
                         {O::Sine, E5, E5, 0.70f, 0.25f, 0.28f, 5.0f, 0.006f}}, 0.20f, 0.45f},
        {"defend",      {{O::Sine, 80, 45, 0.25f, 0.5f}, {O::Noise, 700, 200, 0.12f, 0.25f}}, 0.10f, 0.25f, 0.3f},
        {"flee",        {{O::Noise, 150, 1400, 0.45f, 0.3f}, {O::Sine, A2, A3, 0.40f, 0.15f, 0.0f, 6.0f}}, 0.15f, 0.30f, 0.3f},
        {"flee_fail",   {{O::Noise, 200, 700, 0.15f, 0.25f}, {O::Sine, 90, 40, 0.20f, 0.45f, 0.15f},
                         {O::Saw, A2, A2, 0.30f, 0.15f, 0.20f}, {O::Saw, Eb3, Eb3, 0.30f, 0.15f, 0.20f}}, 0.10f, 0.2f, 0.3f},
        {"victory",     {{O::Sine, A3, A3, 0.40f, 0.3f}, {O::Sine, C4, C4, 0.40f, 0.3f, 0.20f},
                         {O::Sine, E4, E4, 0.40f, 0.3f, 0.40f}, {O::Sine, A4, A4, 1.10f, 0.3f, 0.60f, 4.0f, 0.006f},
                         {O::Saw, A1, A1, 1.70f, 0.15f, 0.0f, 0.0f, 0.01f}}, 0.25f, 0.40f, 0.3f},
        {"defeat",      {{O::Saw, A3, A3, 0.60f, 0.2f, 0.0f, 0.0f, 0.01f}, {O::Saw, F3, F3, 0.60f, 0.2f, 0.55f, 0.0f, 0.01f},
                         {O::Saw, D3, D3, 0.60f, 0.2f, 1.10f, 0.0f, 0.01f}, {O::Saw, A2, A2 * 0.97f, 1.60f, 0.25f, 1.65f, 3.0f, 0.015f},
                         {O::Sine, A2 * kBell, A2 * kBell, 1.2f, 0.12f, 1.65f}}, 0.30f, 0.45f, 0.25f},
        {"explosion",   {{O::Noise, 900, 30, 0.90f, 0.65f}, {O::Sine, 60, 20, 0.90f, 0.6f}}, 0.22f, 0.40f, 0.3f},
        {"throw",       {{O::Noise, 250, 1100, 0.25f, 0.3f}}, 0.0f, 0.0f, 0.3f},
        {"trap",        {{O::Square, 1400, 1400, 0.02f, 0.2f}, {O::Saw, 90, 70, 0.25f, 0.15f, 0.03f, 20.0f}},
                        0.10f, 0.25f, 0.4f},
        {"key",         {{O::Noise, 3500, 1800, 0.015f, 0.35f}, {O::Sine, 190, 120, 0.04f, 0.3f}}, 0.0f, 0.0f, 0.5f},
        {"key_enter",   {{O::Noise, 2500, 900, 0.03f, 0.45f}, {O::Sine, 110, 60, 0.10f, 0.45f},
                         {O::Sine, A3, A3, 0.50f, 0.25f, 0.04f}, {O::Sine, A3 * kBell, A3 * kBell, 0.25f, 0.08f, 0.04f}},
                        0.16f, 0.30f, 0.4f},
        {"drink",       {{O::Sine, 320, 170, 0.08f, 0.4f, 0.00f, 14.0f}, {O::Sine, 300, 160, 0.08f, 0.4f, 0.13f, 14.0f},
                         {O::Sine, 280, 150, 0.09f, 0.4f, 0.26f, 14.0f},
                         {O::Sine, A3, A3, 0.45f, 0.18f, 0.38f, 5.0f}, {O::Sine, E4, E4, 0.55f, 0.15f, 0.45f, 5.0f}},
                        0.14f, 0.30f, 0.4f},
        {"mana",        {{O::Sine, 300, 170, 0.08f, 0.35f, 0.00f, 14.0f}, {O::Sine, 280, 150, 0.09f, 0.35f, 0.13f, 14.0f},
                         {O::Sine, E3, E4, 0.70f, 0.25f, 0.22f, 7.0f, 0.012f},
                         {O::Sine, E4 * kBell, E4 * kBell, 0.40f, 0.06f, 0.70f}}, 0.20f, 0.40f, 0.4f},
        {"zoom_in",     {{O::Noise, 2500, 1500, 0.015f, 0.3f}, {O::Sine, 160, 260, 0.10f, 0.3f, 0.01f}}, 0.0f, 0.0f, 0.45f},
        {"zoom_out",    {{O::Noise, 2500, 1500, 0.015f, 0.3f}, {O::Sine, 260, 160, 0.10f, 0.3f, 0.01f}}, 0.0f, 0.0f, 0.45f},
    };
}

// Volumen propio de cada efecto, relativo al general.
float baseVolume(SfxId id) {
    switch (id) {
    case SfxId::Hit: case SfxId::Crit: case SfxId::PlayerHurt: case SfxId::Blocked:
    case SfxId::EnemyDie: case SfxId::Defend: case SfxId::FleeFail:
        return 0.45f;
    case SfxId::BossDie: case SfxId::Explosion: case SfxId::CombatStart:
        return 0.6f;
    default:
        return 1.0f;
    }
}

float oscSample(Osc osc, float phase, float prevPhase, float& noise, float& smooth) {
    switch (osc) {
    case Osc::Square: return phase < 0.5f ? 1.0f : -1.0f;
    case Osc::Saw:    return 2.0f * phase - 1.0f;
    case Osc::Sine:   return std::sin(2.0f * kPi * phase);
    case Osc::Noise:
        // Ruido "sample & hold" a la frecuencia indicada, suavizado
        if (phase < prevPhase) noise = static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f;
        smooth += (noise - smooth) * 0.5f;
        return smooth;
    }
    return 0.0f;
}

Wave synthesize(const Recipe& r) {
    float total = 0.0f;
    for (const auto& t : r.tones) total = std::max(total, t.at + t.dur);
    if (r.echo > 0.0f) total += r.echo * 5.0f;
    const int n = static_cast<int>(total * kRate) + 1;
    std::vector<float> mix(n, 0.0f);

    for (const auto& t : r.tones) {
        const int start  = static_cast<int>(t.at * kRate);
        const int len    = static_cast<int>(t.dur * kRate);
        const int attack = std::max(1, std::min(len / 8, kRate / 100));
        float ph1 = 0.0f, ph2 = 0.0f, n1 = 0.0f, n2 = 0.0f, s1 = 0.0f, s2 = 0.0f;
        for (int i = 0; i < len && start + i < n; i++) {
            float p    = static_cast<float>(i) / len;
            float freq = t.f0 + (t.f1 - t.f0) * p;
            if (t.vib > 0.0f) freq *= 1.0f + 0.03f * std::sin(2.0f * kPi * t.vib * i / kRate);
            float prev1 = ph1, prev2 = ph2;
            ph1 += freq / kRate;               ph1 -= std::floor(ph1);
            ph2 += freq * (1.0f + t.det) / kRate; ph2 -= std::floor(ph2);
            float s = oscSample(t.osc, ph1, prev1, n1, s1);
            if (t.det > 0.0f) s = 0.5f * (s + oscSample(t.osc, ph2, prev2, n2, s2));
            float env = i < attack ? static_cast<float>(i) / attack : 1.0f - p;
            mix[start + i] += s * env * env * t.gain;
        }
    }

    float lp = 0.0f;
    for (float& v : mix) { lp += (v - lp) * r.lp; v = lp; }

    if (r.echo > 0.0f) {
        const int d = static_cast<int>(r.echo * kRate);
        for (int i = d; i < n; i++) mix[i] += mix[i - d] * r.fb;
    }

    float peak = 0.0f;
    for (float v : mix) peak = std::max(peak, std::fabs(v));
    float norm = peak > 0.9f ? 0.9f / peak : 1.0f;

    auto* data = static_cast<int16_t*>(MemAlloc(n * sizeof(int16_t)));
    for (int i = 0; i < n; i++)
        data[i] = static_cast<int16_t>(std::clamp(mix[i] * norm, -1.0f, 1.0f) * 32000.0f);
    Wave w{};
    w.frameCount = static_cast<unsigned int>(n);
    w.sampleRate = kRate;
    w.sampleSize = 16;
    w.channels   = 1;
    w.data       = data;
    return w;
}

struct Pending { SfxId id; double at; };

std::array<Sound, static_cast<int>(SfxId::Count)> gSounds{};
std::vector<Pending> gQueue;
std::mutex           gQueueMutex;
bool                 gReady  = false;
int                  gVolume = 75;

}

namespace Sfx {

void init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) return;
    SetMasterVolume(gVolume / 100.0f);

    auto list = recipes();
    for (int i = 0; i < static_cast<int>(SfxId::Count) && i < static_cast<int>(list.size()); i++) {
        std::string base = assetsDir() + "sfx/" + list[i].name;
        if (FileExists((base + ".wav").c_str()))
            gSounds[i] = LoadSound((base + ".wav").c_str());
        else if (FileExists((base + ".ogg").c_str()))
            gSounds[i] = LoadSound((base + ".ogg").c_str());
        if (gSounds[i].frameCount == 0) {
            Wave w = synthesize(list[i]);
            gSounds[i] = LoadSoundFromWave(w);
            UnloadWave(w);
        }
        SetSoundVolume(gSounds[i], baseVolume(static_cast<SfxId>(i)));
    }
    gReady = true;
}

void shutdown() {
    if (!gReady) return;
    for (auto& s : gSounds) UnloadSound(s);
    gReady = false;
    CloseAudioDevice();
}

void update() {
    if (!gReady) return;
    std::vector<SfxId> due;
    {
        std::lock_guard<std::mutex> lk(gQueueMutex);
        double now = GetTime();
        for (auto it = gQueue.begin(); it != gQueue.end();) {
            if (it->at <= now) { due.push_back(it->id); it = gQueue.erase(it); }
            else ++it;
        }
    }
    for (SfxId id : due) PlaySound(gSounds[static_cast<int>(id)]);
}

void play(SfxId id, float delay) {
    if (!gReady || gVolume == 0 || id == SfxId::Count) return;
    std::lock_guard<std::mutex> lk(gQueueMutex);
    gQueue.push_back({id, GetTime() + delay});
}

void setVolume(int percent) {
    gVolume = std::clamp(percent, 0, 100);
    if (gReady) SetMasterVolume(gVolume / 100.0f);
}

int volume() { return gVolume; }

}

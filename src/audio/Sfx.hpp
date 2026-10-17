#pragma once

// Efectos de sonido. Cada efecto se sintetiza al arrancar (estilo 8-bit), salvo
// que exista assets/sfx/<nombre>.wav u .ogg, en cuyo caso se usa ese archivo.
// play() es seguro desde cualquier hilo: encola y update() reproduce en el principal.

enum class SfxId {
    MenuMove, MenuSelect, MenuBack,
    Step, Chest, Coin, Stairs, LevelUp, Quest, Equip, Buy, Denied,
    CombatStart, Hit, Crit, PlayerHurt, Blocked, EnemyDie, BossDie,
    Spell, Buff, Heal, Defend, Flee, FleeFail, Victory, Defeat,
    Explosion, Throw, Trap,
    Key, KeyEnter,
    Drink, Mana, ZoomIn, ZoomOut,
    Count
};

namespace Sfx {
    void init();        // tras InitWindow()
    void shutdown();    // antes de CloseWindow()
    void update();      // una vez por frame, hilo principal
    void play(SfxId id, float delay = 0.0f);
    void setVolume(int percent);   // 0–100
    int  volume();
}

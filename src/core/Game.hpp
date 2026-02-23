#pragma once

#include "GameState.hpp"
#include "MenuPhase.hpp"
#include "entities/Player.hpp"
#include "world/Map.hpp"
#include "world/BSPDungeon.hpp"
#include "ui/HudLayout.hpp"
#include <string>
#include <memory>

// Central game class. Owns the main loop and all subsystems.
class Game {
public:
    Game();
    ~Game();

    void run();

private:
    GameState     state_;
    MenuPhase     menuPhase_;
    bool          running_;

    // Menu state
    int           menuSelection_;   // title: 0=Nueva Partida, 1=Salir
    std::string   playerName_;      // being typed in NameInput
    int           classSelection_;  // 0=Warrior, 1=Mage, 2=Ranger
    int           hudSelection_;    // 0=Sidebar, 1=Bottom

    HudLayout     hudLayout_;

    std::unique_ptr<Player>     player_;
    std::unique_ptr<Map>        map_;

    void processInput(int key);
    void update();
    void render();

    void setState(GameState newState);

    // Per-phase input handlers
    void inputTitle(int key);
    void inputNameInput(int key);
    void inputClassSelect(int key);
    void inputHudSelect(int key);
};

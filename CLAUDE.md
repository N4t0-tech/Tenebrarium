# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Tenebrarium is a DND-style RPG in C++17 with ncurses terminal UI and CMake as build system.

## Build & Run

```bash
# Configure (first time only)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./build/tenebrarium
```

## Architecture

The codebase is organized into modules under `src/`:

- **`core/`** — `Game` class owns the main loop and all game state. `GameState` enum drives what `processInput`, `update`, and `render` do each frame.
- **`entities/`** — `Entity` is the base for all combatants (stats, damage, healing). `Player` extends it with level/XP, mana, class type (`Warrior`, `Mage`, `Ranger`), and an `Inventory`.
- **`inventory/`** — Slot-based `Inventory`. Each `Item` declares how many slots it consumes.
- **`combat/`** — `CombatSystem` takes a player reference and a list of enemy pointers, resolves turns, and maintains a string log for the UI.
- **`world/`** — `Map` is a 2D tile grid with FOV. `TileType` and `Tile` are the primitives.
- **`ui/`** — `Renderer` contains all ncurses drawing code. Each `draw*` method is called from `Game::render()` after `clear()`.
- **`quests/`** — `Quest` and `QuestObjective` structs. No manager class yet.

## Key Design Decisions

- `Game` calls `Renderer::draw*()` static methods — the renderer is stateless and receives data as parameters.
- `GameState` drives which system is active. Within `MainMenu`, `MenuPhase` tracks the sub-flow: `Title → NameInput → ClassSelect`.
- ncurses color pairs are defined in `Game.cpp::initColors()`: 1=normal, 2=highlight, 3=title/cyan, 4=stats/green, 5=selected box (black on cyan).
- The title screen ASCII art is loaded at runtime from `assets/title.txt`. Edit that file freely — the renderer centers each line automatically. The assets path is injected at compile time via the `ASSETS_DIR` macro defined in `CMakeLists.txt`.
- Saves will use local JSON files in `saves/` (gitignored). No JSON library integrated yet.
- Character art (PNG/JPG) will be displayed inside ncurses box frames in `assets/art/`.
- `compile_commands.json` is exported by CMake (`CMAKE_EXPORT_COMPILE_COMMANDS ON`) for editor tooling.

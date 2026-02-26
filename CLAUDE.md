# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Tenebrarium is a DND-style RPG in C++17 with **FTXUI** terminal UI and CMake as build system.
Multiplataforma (Linux/Windows nativo), truecolor, y soporte de retratos REXPaint (.xp).

## Dependencies

- **FTXUI v5.0.0** — descargado automáticamente por CMake FetchContent
- **zlib** — descompresión de archivos .xp (gzip); instalar con el gestor de paquetes del sistema
- **CMake ≥ 3.20**, **C++17**, **git** (para FetchContent)

```bash
# Arch Linux
sudo pacman -S cmake zlib git
```

## Build & Run

```bash
# Configure (primera vez — descarga FTXUI ~1-2 min)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j$(nproc)

# Run
./build/tenebrarium
```

No test suite exists yet. Verify behavior by running the game.

## Architecture

The codebase is organized into modules under `src/`:

- **`core/`** — `Game` class owns the main loop and all game state. `GameState` enum drives what `processInput`, `update`, and `renderDocument` do each frame.
- **`entities/`** — `Entity` is the base for all combatants (stats, damage, healing). `Player` extends it with level/XP, mana, class type (`Warrior`, `Mage`, `Ranger`), and an `Inventory`.
- **`inventory/`** — Slot-based `Inventory`. Each `Item` declares how many slots it consumes.
- **`combat/`** — `CombatSystem` takes a player reference and a list of enemy pointers, resolves turns, and maintains a string log for the UI.
- **`world/`** — `Map` is a 2D tile grid with FOV. `BSPDungeon` carves rooms and corridors via binary space partitioning.
- **`ui/`** — `Renderer` devuelve `ftxui::Element` (stateless). `XpLoader` carga archivos REXPaint `.xp`.
- **`quests/`** — `Quest` and `QuestObjective` structs. No manager class yet.

## Key Design Decisions

- `Game` llama `Renderer::draw*()` estáticos que devuelven `ftxui::Element` — el renderer es stateless.
- El game loop usa `ftxui::ScreenInteractive::Fullscreen()` + `CatchEvent` en lugar de `getch()`.
- `GameState` drives which system is active. Within `MainMenu`, `MenuPhase` tracks the sub-flow: `Title → NameInput → ClassSelect → HudSelect`.
- Colores FTXUI: `Color::White` normal, `Color::Yellow` highlight, `Color::Cyan` título, `Color::Green` stats, `Color::Red` enemigos, `Color::GrayDark` muros secretos.
- The title screen ASCII art is loaded at runtime from `assets/title.txt`. The assets path is injected at compile time via the `ASSETS_DIR` macro defined in `CMakeLists.txt`.
- `compile_commands.json` is exported by CMake (`CMAKE_EXPORT_COMPILE_COMMANDS ON`) for editor tooling.
- Para salir del loop FTXUI usar `screen_.ExitLoopClosure()()` — NO `running_ = false`.

## World Entity Rendering

`Game` maintains parallel world state vectors (`worldEnemies_`, `worldChests_`) alongside the `Map`. Before calling `Renderer::drawExploration`, it builds a `std::vector<MapEntity>` from alive enemies and unopened chests. `MapEntity` holds a position, glyph, and `colorPair` int (mapeado a `Color::*` dentro del Renderer).

## XpLoader — Retratos REXPaint

- `loadXp(path)` — descomprime gzip con `gzopen`/`gzread` y parsea el binario .xp (column-major → row-major).
- `xpToElement(layer, stepX, stepY)` — renderiza glifos CP437 originales con colores truecolor.
- `xpToElementHalfBlock(layer)` — empaqueta 2 filas en 1 con `▀`, sin pérdida de píxeles verticales y aspecto cuadrado automático.
- Archivos de retratos en `assets/art/<nombre>.xp`. Registrar en `kClasses[]` en `Renderer.cpp`.
- Clase seleccionada en `drawClassSelect`: lista vertical a la izquierda + retrato a la derecha.

## Combat System

- Turn-based with an Action Point (AP) system: player gets **3 AP** per turn.
- Actions: Attack (1 AP), Heavy Attack (2 AP), Defend (1 AP), Use Art (variable), Flee (3 AP), End Turn (0 AP).
- Each class has 3 **Arts** (`Art.hpp` / `ArtEffect` enum). Arts cost AP + mana.
- `StatusEffect` (in `CombatSystem.hpp`) handles Poisoned, Frozen, AttackBoosted, Defending, DefendingHeavy, TrapPending.
- `Enemy::basePa_` controls how many attacks an enemy makes per turn.
- After combat ends, `Game` checks `playerWon()` / `playerFled()` to award XP and loot or return to exploration.

## Controls Reference

### Menú principal / Selección de clase
| Key | Action |
|-----|--------|
| `↑ ↓` | Navegar opciones |
| `← →` | Navegar (título, HUD select) |
| `Enter` | Confirmar |
| `Esc` | Volver |

### Exploration
| Key | Action |
|-----|--------|
| `W A S D` / arrows / `H J K L` | Move |
| `E` | Search adjacent secret walls |
| `Q` | Quit |

### Combat
| Key | Action | Cost |
|-----|--------|------|
| `A` | Attack | 1 PA |
| `F` | Heavy Attack | 2 PA |
| `H` | Toggle Arts menu | — |
| `D` | Defend | 1 PA |
| `Space` | End turn | — |
| `R` | Flee | 3 PA |
| `Tab` | Cycle target | — |
| `↑ ↓` | Navigate Arts | — |
| `Enter` | Confirm Art | — |
| `Esc` | Back to main menu | — |

## Pending / Not Yet Implemented

- Save/load (planned: JSON files in `saves/`, gitignored).
- Retratos para Guerrero y Ranger (solo Mago tiene `assets/art/mago.xp`).
- Quest manager class (structs exist in `quests/`).
- `drawInventory()` and `drawQuestLog()` are stub methods in `Renderer`.

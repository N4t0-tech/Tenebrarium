# Tenebrarium

RPG de mazmorras por turnos en la terminal, estilo D&D. Escrito en C++17 con [FTXUI](https://github.com/ArthurSonzogni/FTXUI) para la interfaz y retratos de personaje en truecolor cargados desde archivos REXPaint (`.xp`).

## Características

- Generación procedural de mazmorras con BSP (Binary Space Partitioning)
- Campo de visión con ray-casting
- Combate por turnos con sistema de Puntos de Acción (PA)
- 3 clases jugables: Guerrero, Mago, Ranger — cada una con 3 habilidades únicas
- Inventario por slots, cofres, llaves, puertas cerradas y salas secretas
- Retratos de clase en truecolor usando archivos REXPaint `.xp`
- 2 modos de HUD: panel lateral o barra inferior
- Multiplataforma (Linux / Windows nativo)

## Dependencias

| Dependencia | Versión | Instalación |
|---|---|---|
| CMake | ≥ 3.20 | `pacman -S cmake` |
| zlib | cualquiera | `pacman -S zlib` |
| git | cualquiera | `pacman -S git` |
| FTXUI | v5.0.0 | descargado automáticamente |

## Build & Run

```bash
# Clonar
git clone https://github.com/tu-usuario/Tenebrarium.git
cd Tenebrarium

# Configurar (descarga FTXUI la primera vez, ~1-2 min)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# Compilar
cmake --build build -j$(nproc)

# Ejecutar
./build/tenebrarium
```

## Controles

### Menú
| Tecla | Acción |
|---|---|
| `↑ ↓` | Navegar entre clases |
| `← →` | Navegar opciones de título / HUD |
| `Enter` | Confirmar |
| `Esc` | Volver |

### Exploración
| Tecla | Acción |
|---|---|
| `W A S D` / flechas / `H J K L` | Moverse |
| `E` | Buscar paredes secretas adyacentes |
| `Q` | Salir |

### Combate
| Tecla | Acción | Coste |
|---|---|---|
| `A` | Atacar | 1 PA |
| `F` | Ataque Fuerte | 2 PA |
| `H` | Abrir menú de Habilidades | — |
| `D` | Defender | 1 PA |
| `Space` | Terminar turno | — |
| `R` | Huir | 3 PA |
| `Tab` | Cambiar objetivo | — |
| `↑ ↓` | Navegar habilidades | — |
| `Enter` | Confirmar habilidad | — |
| `Esc` | Volver al menú principal | — |

## Estructura del proyecto

```
src/
  core/       Game loop, estados, input
  entities/   Player, Enemy, Entity base
  combat/     CombatSystem, Arts, StatusEffects
  inventory/  Inventory, Item
  world/      Map, BSPDungeon, FOV
  ui/         Renderer (FTXUI), XpLoader (REXPaint)
  quests/     Quest structs (WIP)
assets/
  title.txt   ASCII art del título
  art/        Retratos .xp de REXPaint por clase
```

## Retratos REXPaint

Los retratos de clase se guardan como `assets/art/<nombre>.xp`. Actualmente solo el Mago tiene retrato (`mago.xp`). Para agregar los otros:

1. Crear el retrato en REXPaint (60×60 recomendado)
2. Guardarlo en `assets/art/guerrero.xp` o `assets/art/ranger.xp`
3. Actualizar `kClasses[]` en `src/ui/Renderer.cpp` cambiando `nullptr` por el nombre del archivo

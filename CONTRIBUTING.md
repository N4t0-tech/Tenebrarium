# Contribuir a Tenebrarium

## Configurar el entorno

**Dependencias del sistema:** `cmake`, `zlib`, `git`

```bash
git clone https://github.com/tu-usuario/Tenebrarium.git
cd Tenebrarium
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j$(nproc)
./build/tenebrarium
```

Raylib 5.0 se descarga automáticamente la primera vez que ejecutas CMake.

Si CMake falla con "generator does not match", borra `build/CMakeCache.txt` y `build/CMakeFiles/`, luego reconfigura.

## Probar cambios

No hay suite de tests ni linter. Compila con `cmake --build build -j$(nproc)` y ejecuta para verificar visualmente.

## Estructura del proyecto

```
src/
├── main.cpp                         # Punto de entrada
├── core/                            # Game loop, estados, serialización
│   ├── Game.cpp/hpp                 # Orquestador principal (state machine, input, render)
│   ├── GameState.hpp                # Enum: MainMenu, Exploration, Combat, Inventory...
│   ├── MenuPhase.hpp                # Enum: Title, NameInput, ClassSelect, HudSelect...
│   ├── GameSerializer.cpp/hpp       # Guardado/carga a saves/save.dat
│   ├── ShopItem.hpp                 # Item + precio + vendido
│   └── Assets.hpp                   # assetsDir() -> ruta a assets/
├── entities/                        # Jugador, enemigos, estadísticas
│   ├── Entity.cpp/hpp               # Clase base: HP, ATK, DEF, damage/heal
│   ├── Player.cpp/hpp               # Stats por clase, nivel, equipo, inventario, artes
│   └── Enemy.cpp/hpp                # Tipos de enemigo, XP, PA
├── combat/                          # Sistema de combate por turnos
│   ├── CombatSystem.cpp/hpp         # Ataque, arte, defensa, huida, efectos, botín
│   └── Art.hpp                      # Efectos y definición de artes
├── inventory/                       # Sistema de items
│   ├── Item.hpp                     # Tipos: Weapon, Armor, Consumable, Bomb
│   ├── Inventory.cpp/hpp            # Inventario de 20 slots, apilable
├── world/                           # Generación y estado del mundo
│   ├── BSPDungeon.cpp/hpp           # Árbol BSP para mazmorras procedurales
│   ├── DungeonPopulator.cpp/hpp     # Población: enemigos, cofres, objetos, secretos
│   ├── Dungeon.cpp/hpp              # Fachada thread-safe con mutex
│   ├── Map.cpp/hpp                  # Grid de tiles, FOV (ray-cast), paredes secretas
│   └── WorldObjects.hpp             # WorldEnemy, WorldChest
├── ui/                              # Renderizado y UI
│   ├── TerminalScreen.cpp/hpp       # Buffer de celdas -> dibujado con Raylib
│   ├── Renderer.cpp/hpp             # Métodos static draw*() para cada pantalla
│   ├── XpLoader.cpp/hpp             # Carga de retratos .xp (gzip + CP437)
│   └── HudLayout.hpp                # Enum: Sidebar vs Bottom
├── quests/                          # Sistema de misiones
│   └── Quest.cpp/hpp                # Quest, QuestObjective, recompensas
└── ai/                              # IA de enemigos (hilo separado)
    ├── EnemyAI.cpp/hpp              # Loop en segundo plano, 600ms
    └── Pathfinding.cpp/hpp          # BFS para movimiento enemigo

assets/
├── art/                             # Retratos .xp (formato REXPaint)
├── fonts/mono.ttf                   # Hack Regular (3 tamaños: 18, 36, 54px)
├── shaders/crt.frag                 # Shader CRT post-proceso (GLSL)
├── title.txt                        # ASCII art del título
├── gameOverTitle.txt                # ASCII art de Game Over
└── victoryTitle.txt                 # ASCII art de victoria
```

## Arquitectura

### Pipeline de renderizado

La lógica del juego escribe en un `TerminalScreen` (grid 2D de `Cell` {codepoint, fg, bg, flags}). `TerminalScreen::render()` convierte celdas a llamadas Raylib, volcando a un `RenderTexture2D` offscreen. Luego se aplica el shader CRT (`assets/shaders/crt.frag`) como post-proceso y se presenta en pantalla.

### State machine

```
MainMenu (Title ↔ Credits, Title → NameInput → ClassSelect → HudSelect)
    ↓
Exploration  ←──────────────────────────────┐
    ├─ bump enemy   → Combat                 │
    ├─ reach shop   → Shop ──────────────────┤
    ├─ press I      → Inventory ─────────────┤
    ├─ press M      → QuestLog ──────────────┘
    ├─ press Q      → QuitDialog (ESC → Exploration)
    ├─ press P      → Use potion (Exploration)
    ├─ die          → GameOver → MainMenu
    └─ stairs @20  → GameOver (victory) → MainMenu
```

### Modelo de hilos

El loop principal corre en el hilo principal (Raylib requiere single-thread). La IA enemiga corre en `aiThread_` separado, despertando cada 600ms. El estado del mundo se protege con `dungeon_->mutex_` (`std::mutex`). Banderas atómicas (`pendingCombatEnemy_`, `pendingRedraw_`) comunican eventos entre hilos.

### Guardado

- Partida: `saves/save.dat` (texto, versionado — v3 actual, incluye `isMimic`)
- Ajustes: `saves/settings.dat` (formato `clave=valor`, ej. `mapZoom=1`)
- Auto-save al bajar de piso y al entrar a MainMenu

### Dificultad

- 20 pisos. Stats enemigos escalan: `base * (1 + 0.15 * (piso-1))`
- Pisos 1-2: Fácil, 3-4: Normal, 5-6: Difícil, 7+: Peligroso
- Jefes cada 5 pisos (5, 10, 15, 20) con stats ×2.5

## Cómo contribuir

1. Haz fork del repositorio
2. Crea una rama descriptiva: `git checkout -b feature/nueva-funcionalidad`
3. Haz tus cambios y compila para verificar que no hay errores
4. Abre un Pull Request describiendo qué cambia y por qué

## Áreas donde se acepta ayuda

### Traducción al inglés
- Textos del juego en `Renderer.cpp`, `CombatSystem.cpp`, `Game.cpp`, `DungeonPopulator.cpp`

### Nuevos enemigos
1. Añade entrada a `EnemyType` enum en `src/entities/Enemy.hpp`
2. Stats en `DungeonPopulator::makeEnemy()` en `src/world/DungeonPopulator.cpp`
3. Spawn weights en `DungeonPopulator::pickEnemyType()` (mismo archivo)
4. Glyph mapping en `glyphForEnemy()` en `src/core/Game.cpp`
5. Color mapping en `colorPairForEnemy()` en `src/core/Game.cpp`

### Nuevas clases
1. Añade a `PlayerClass` enum en `src/entities/Player.hpp`
2. Stats base en `Player::baseHp/Attack/Defense/Mana()` en `src/entities/Player.cpp`
3. Artes en `Player::getAvailableArts()` en `src/entities/Player.cpp`
4. Armas en `DungeonPopulator::pickWeapon()` en `src/world/DungeonPopulator.cpp`
5. Armaduras en `DungeonPopulator::pickArmor()` en `src/world/DungeonPopulator.cpp`
6. Retrato `classname.xp` en `assets/art/`
7. Registro en `kClasses[]` en `src/ui/Renderer.cpp` (campo `portraitPath`)

### Nuevas armas/armaduras
- Añade a `pickWeapon()` o `pickArmor()` en `src/world/DungeonPopulator.cpp`
- Cada clase tiene su pool con 2 tiers de stats
- `statBonus` escala: `+= (piso-1)/2`

### Mejoras de pulido
- Efectos visuales (animaciones, partículas)
- Balance de combate
- Variedad de objetos (nuevos consumibles, tipos de item)
- Mejoras de UI/UX
- Efectos de sonido

## Convenciones

- **C++17**, sin excepciones en rutas de renderizado
- **Los métodos de `Renderer` son estáticos** y reciben `TerminalScreen&` — no añadir estado
- **Retratos `.xp`** van en `assets/art/` y se registran en `kClasses[]` en `src/ui/Renderer.cpp`
- **Nombres de clases/tipos** en `PascalCase`
- **Miembros** con trailing underscore (`hp_`, `name_`)
- **Archivos** en `snake_case.hpp` / `snake_case.cpp`
- **No comentar código** innecesariamente — el código debe ser auto-documentado
- **No añadir dependencias** sin discusión previa

## Atajos de teclado (referencia)

**Exploración**: WASD mover, E buscar secreto/usar bomba, P usar poción, I inventario, M misiones, Q salir, +/- zoom mapa.

**Combate**: 1 ataque, 2 ataque pesado, 3 artes, 4 defender, 5 usar objeto, Espacio terminar turno, 6 huir, Tab ciclo objetivo.

## Notas de build para Windows

- `add_executable` usa `WIN32` para ocultar la consola
- Release zip incluye DLLs de MinGW: `zlib1.dll`, `libstdc++-6.dll`, `libwinpthread-1.dll`, `libgcc_s_seh-1.dll`
- Raylib requiere OpenGL 3.3; VMs pueden no soportarlo (workaround: Mesa3D `opengl32.dll`)

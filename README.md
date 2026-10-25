# Tenebrarium

RPG de mazmorras por turnos con estética TUI, ejecutable como aplicación nativa. Escrito en C++17 con Raylib.

[![Jugar en itch.io](https://img.shields.io/badge/itch.io-Jugar-fa5c5c?style=for-the-badge&logo=itch.io&logoColor=white)](https://halleynato.itch.io/tenebrarium)

![Menú principal](Tenebrarium-photos/1Menu.webp)

## Características

- Ventana nativa (Linux / Windows) con fuente monoespaciada empaquetada
- Generación procedural de mazmorras (BSP) con campo de visión
- Combate por turnos con sistema de Puntos de Acción (3 PA por turno)
- **Combate múltiple**: enemigos cercanos en área 5×5 se unen a la pelea (máx. 5)
- **Hasta 3 enemigos por sala** en pisos profundos
- **Antorchas decorativas** con luz dinámica que parpadea y respeta paredes
- 3 clases: Guerrero, Mago, Arquera — y clases secretas por descubrir
- **Árbol de habilidades** por clase: 3 habilidades base y 6 desbloqueables al subir de nivel, con 3 equipables a la vez (`H`)
- **Hechizos de mapa**: trampas de la Arquera colocables con el mouse y lanzamiento de cadáveres
- **Aldea** con tienda, forja para mejorar el equipo y sala de entrenamiento para subir los rangos de las habilidades (I–III)
- **Ciclo día/noche** con hora persistente
- Equipo con 4 rarezas: Común, Raro, Épico y Legendario
- Inventario por slots, equipo, pociones, cofres, llaves y salas secretas
- Tienda con mercader por piso y compra por cantidad
- **20 pisos** con escalado de dificultad — jefes cada 5 pisos
- Diario de misiones
- Efectos de sonido tenebrosos con volumen ajustable
- Retratos de clase en truecolor (REXPaint `.xp`)
- 2 modos de HUD: panel lateral o barra inferior
- Movimiento con WASD o flechas, configurable en ajustes
- Zoom del mapa ajustable en tiempo real (`+` / `-`), se guarda entre sesiones
- Guardado automático al bajar de piso — continúa desde el menú principal

| | | |
| --- | --- | --- |
| ![Mago](Tenebrarium-photos/2wizard.webp) | ![Guerrero](Tenebrarium-photos/3warrior.webp) | ![Ranger](Tenebrarium-photos/4ranger.webp) |

| | |
| --- | --- |
| ![Gameplay 1](Tenebrarium-photos/gameplay1.webp) | ![Gameplay 2](Tenebrarium-photos/gameplay2.webp) |

## Descarga

Descarga el ejecutable desde la [página de releases](../../releases/latest):

- **Linux** → `tenebrarium-linux.tar.gz` — extrae y ejecuta `./tenebrarium`
- **Windows** → `tenebrarium-windows.zip` — extrae y ejecuta `tenebrarium.exe`

> **Nota Windows:** requiere OpenGL 3.3. En VMs sin soporte, coloca `opengl32.dll` de [Mesa3D](https://fdossena.com/?p=mesa/index.frag) junto al `.exe`.

## Build & Run

**Dependencias del sistema:** `cmake` `zlib` `git`

```bash
# Arch Linux
sudo pacman -S cmake zlib git

# Compilar (descarga Raylib automáticamente la primera vez)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j$(nproc)

./build/tenebrarium
```

## Controles

### Exploración

| Tecla | Acción |
| --- | --- |
| `W A S D` o flechas | Moverse (se elige en Configuración) |
| `E` | Buscar paredes secretas (usa una bomba si tienes) |
| `P` | Beber poción de vida |
| `R` | Beber poción de maná (cerveza para el Guerrero) |
| `Q` | Habilidad de mapa: lanzar hechizo, o levantar y lanzar cadáveres (Guerrero) |
| `T` | Colocar trampa (Arquera) |
| `H` | Habilidades: equipar en las ranuras `1` `2` `3` |
| `I` | Inventario |
| `M` | Diario de misiones |
| `B` | Bestiario |
| `+` / `-` | Zoom del mapa (1×–3×) |
| `F` | Pantalla completa |
| `Esc` | Menú de salida |

Al apuntar un hechizo o una trampa: mueve el cursor con las teclas de movimiento o el mouse, confirma con `Enter` o clic izquierdo y cancela con `Esc` o clic derecho. Con varios hechizos, `Q` o `1`–`4` cambian entre ellos.

En la aldea basta con caminar hasta la tienda, la forja o la sala de entrenamiento para abrir su menú.

### Combate

| Tecla | Acción | Coste |
| --- | --- | --- |
| `1` | Atacar | 1 PA |
| `2` | Ataque Fuerte | 2 PA |
| `3` | Habilidades (luego `1`–`3` o `Enter`) | variable |
| `4` | Defender | 1 PA |
| `5` | Usar poción | 1 PA |
| `6` | Huir | 3 PA |
| `7` | Saquear al objetivo | 1 PA |
| `Tab` | Cambiar objetivo | — |
| `Space` | Terminar turno | — |

## Estructura

```text
src/
  core/       Game loop, estados, input, guardado
  entities/   Player, Enemy, SkillTree
  combat/     CombatSystem, Arts, hechizos de mapa
  ai/         EnemyAI, Pathfinding
  inventory/  Inventory, Item
  world/      Map, BSPDungeon, Dungeon, Village, DayCycle
  ui/         Renderer, TerminalScreen, XpLoader
  audio/      Efectos de sonido (Sfx)
  bestiary/   Datos del bestiario
  quests/     Sistema de misiones
assets/
  fonts/      mono.ttf (Hack Regular), atlas CP437
  art/        Retratos .xp por clase
  maps/       Mapa de la aldea (aldea.xp)
  shaders/    Shader CRT
```

## Créditos

- **Nato** — Ascii Forager
- **Halley** — Diseñadora de retratos independiente

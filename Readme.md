# Tenebrarium

Un juego RPG estilo D&D en la terminal, escrito en C++17 con interfaz ASCII y ncurses.

## Características

- Combate por turnos con sistema de Puntos de Acción (PA)
- Artes únicas por clase (Guerrero, Mago, Ranger)
- Inventario por espacios de objetos
- Generación procedural de mazmorras (BSP)
- Salas secretas ocultas en las paredes
- Sistema de llaves, cofres y loot
- Interfaz ASCII con HUD configurable (barra lateral o inferior)

---

## Instalación

### Requisitos generales

| Herramienta | Versión mínima |
|-------------|----------------|
| Compilador C++ | GCC 9+ / Clang 10+ |
| CMake | 3.20+ |
| ncurses | 6.x |
| Git | cualquiera |

---

### Linux

#### Ubuntu / Debian / Linux Mint

```bash
sudo apt update
sudo apt install git build-essential cmake libncurses-dev
```

#### Arch Linux / Manjaro

```bash
sudo pacman -S git base-devel cmake ncurses
```

#### Fedora / RHEL / CentOS Stream

```bash
sudo dnf install git gcc-c++ cmake ncurses-devel
```

#### openSUSE

```bash
sudo zypper install git gcc-c++ cmake ncurses-devel
```

#### Compilar y ejecutar

```bash
git clone https://github.com/N4t0-tech/Tenebrarium.git
cd Tenebrarium

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/tenebrarium
```

---

### macOS

#### 1. Instalar Xcode Command Line Tools

```bash
xcode-select --install
```

#### 2. Instalar Homebrew (si no lo tienes)

```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 3. Instalar CMake

```bash
brew install cmake
```

> ncurses ya viene incluido en macOS con las Command Line Tools, no requiere instalación adicional.

#### 4. Compilar y ejecutar

```bash
git clone https://github.com/N4t0-tech/Tenebrarium.git
cd Tenebrarium

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/tenebrarium
```

---

### Windows 10 / 11

Hay dos opciones. Se recomienda **WSL2** por ser la más sencilla y estable.

---

#### Opción A — WSL2 (recomendado)

WSL2 (Windows Subsystem for Linux) permite correr un entorno Linux completo dentro de Windows.

**1. Activar WSL2**

Abre PowerShell como Administrador y ejecuta:

```powershell
wsl --install
```

Reinicia el equipo cuando se te pida. Esto instala Ubuntu por defecto.

**2. Abrir la terminal de Ubuntu**

Busca "Ubuntu" en el menú Inicio y ábrela. La primera vez te pedirá crear un usuario.

**3. Instalar dependencias**

```bash
sudo apt update
sudo apt install git build-essential cmake libncurses-dev
```

**4. Compilar y ejecutar**

```bash
git clone https://github.com/N4t0-tech/Tenebrarium.git
cd Tenebrarium

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/tenebrarium
```

> Para mejor experiencia visual en WSL2 se recomienda usar **Windows Terminal** (disponible gratis en la Microsoft Store).

---

#### Opción B — MSYS2 (nativo Windows)

MSYS2 proporciona un entorno de compilación GCC nativo para Windows.

**1. Descargar e instalar MSYS2**

Ve a [https://www.msys2.org](https://www.msys2.org) y descarga el instalador. Sigue los pasos del instalador.

**2. Abrir la terminal MSYS2 MINGW64**

Desde el menú Inicio busca **"MSYS2 MINGW64"** (importante: usa MINGW64, no MSYS).

**3. Actualizar e instalar dependencias**

```bash
pacman -Syu
pacman -S git mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ncurses
```

**4. Compilar y ejecutar**

```bash
git clone https://github.com/N4t0-tech/Tenebrarium.git
cd Tenebrarium

cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/tenebrarium.exe
```

---

## Controles

### Exploración

| Tecla | Acción |
|-------|--------|
| `W A S D` / `↑ ← ↓ →` / `H J K L` | Moverse |
| `E` | Buscar paredes secretas adyacentes |
| `Q` | Salir del juego |

### Combate

| Tecla | Acción | Costo |
|-------|--------|-------|
| `A` | Atacar | 1 PA |
| `F` | Ataque fuerte | 2 PA |
| `H` | Abrir menú de habilidades | — |
| `D` | Defender | 1 PA |
| `Space` | Terminar turno | — |
| `R` | Huir | 3 PA |
| `Tab` | Cambiar objetivo | — |
| `↑ ↓` | Navegar habilidades | — |
| `Enter` | Confirmar habilidad | — |
| `Esc` | Volver al menú principal | — |

---

## UI/UX

- Interfaz de usuario por bloques ASCII con ncurses
- Mapa renderizado con caracteres y campo de visión LOS (Line of Sight)
- HUD configurable: barra lateral o barra inferior
- Guardado local en JSON *(próximamente)*
- Arte de personajes renderizado dentro de recuadros ASCII *(próximamente)*

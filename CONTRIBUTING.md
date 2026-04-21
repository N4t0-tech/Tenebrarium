# Contribuir a Tenebrarium

## Configurar el entorno

**Dependencias:** `cmake` `zlib` `git`

```bash
git clone https://github.com/tu-usuario/Tenebrarium.git
cd Tenebrarium
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build -j$(nproc)
./build/tenebrarium
```

Raylib 5.0 se descarga automáticamente la primera vez.

## Estructura del proyecto

Consulta el [README.md](README.md) para entender la arquitectura general del proyecto.

## Cómo contribuir

1. Haz fork del repositorio
2. Crea una rama descriptiva: `git checkout -b feature/nueva-funcionalidad`
3. Haz tus cambios y compila para verificar que no hay errores
4. Abre un Pull Request describiendo qué cambia y por qué

## Áreas donde se acepta ayuda

- Retrato de Ranger en REXPaint (`.xp`) — el retrato se crea con [REXPaint](https://www.gridsagegames.com/rexpaint/), coloca el archivo en `assets/art/ranger.xp` y regístralo en `kClasses[]` en `src/ui/Renderer.cpp`
- Nuevos tipos de enemigo — añade la entrada en `EnemyType` (`src/entities/Enemy.hpp`), sus stats en `makeEnemy()` y la tabla de spawn en `populateFloor()` (`src/core/Game.cpp`)

## Convenciones

- C++17, sin excepciones en rutas de render
- Los métodos de `Renderer` son estáticos y reciben `TerminalScreen&` — no añadir estado
- Los archivos `.xp` van en `assets/art/` y se registran en `kClasses[]` en `src/ui/Renderer.cpp`

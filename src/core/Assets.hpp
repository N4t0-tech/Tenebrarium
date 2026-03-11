#pragma once
#include <string>

// Ruta a la carpeta assets/, relativa al ejecutable.
// Se inicializa una vez en Game::run() tras InitWindow().
inline std::string& assetsDir() {
    static std::string path;
    return path;
}

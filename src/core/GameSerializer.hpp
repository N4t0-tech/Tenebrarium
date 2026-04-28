#pragma once

// GameSerializer — serializa/deserializa el estado completo del juego a disco.
// Todos los métodos son estáticos; no guarda estado propio.
// Archivo: <directorio_ejecutable>/saves/save.dat (texto plano, versión 1).
// Es friend de Game para poder acceder directamente a sus miembros privados.

#include <string>
#include <iosfwd>
#include "../inventory/Item.hpp"

class Game;

class GameSerializer {
public:
    static std::string savePath();        // ruta absoluta del archivo de save
    static bool        hasSave();         // true si el archivo existe y es legible
    static void        deleteSave();      // elimina el archivo de save (permadeath)
    static void        save(Game& g);     // sobreescribe el save con el estado actual
    static bool        load(Game& g);     // carga el save; retorna false si falla o versión incorrecta

private:
    // wstr/rstr: serialización de strings con prefijo de longitud ("5 hola ")
    // para soportar nombres con espacios y evitar ambigüedad al deserializar.
    static void wstr(std::ostream& o, const std::string& s);
    static bool rstr(std::istream& in, std::string& s);
    static void witem(std::ostream& o, const Item& item);
    static bool ritem(std::istream& in, Item& item);
};

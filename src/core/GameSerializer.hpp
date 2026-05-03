#pragma once

#include <string>
#include <iosfwd>
#include "../inventory/Item.hpp"

class Game;

class GameSerializer {
public:
    static std::string savePath();
    static bool        hasSave();
    static void        deleteSave();
    static void        save(Game& g);
    static bool        load(Game& g);

private:
    static void wstr(std::ostream& o, const std::string& s);
    static bool rstr(std::istream& in, std::string& s);
    static void witem(std::ostream& o, const Item& item);
    static bool ritem(std::istream& in, Item& item, int version);
};

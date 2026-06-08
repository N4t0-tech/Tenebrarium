#pragma once

// Estados principales de la máquina de estados del juego.
// Las transiciones están centralizadas en Game::setState() y Game::dispatchInput().
// Diagrama resumido:
//   MainMenu → Exploration → Combat → Exploration (o GameOver)
//   Exploration → Inventory / QuestLog / Shop / QuitDialog → Exploration
//   GameOver → MainMenu
enum class GameState {
    MainMenu,    // menú inicial + flujo de creación de personaje (ver MenuPhase)
    Exploration, // movimiento en el mapa, interacción con cofres/tienda/escaleras
    Combat,      // sistema de combate por turnos
    Inventory,   // pantalla de mochila y equipo
    QuestLog,    // diario de misiones
    Bestiary,    // bestiario
    GameOver,    // derrota o victoria (victory_ distingue cuál)
    Shop,        // tienda del piso
    QuitDialog,  // confirmación de salida (ESC cancela sin regenerar el mapa)
};
